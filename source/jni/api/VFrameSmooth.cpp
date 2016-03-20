#include "VFrameSmooth.h"
#include "Alg.h"

#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>			// for usleep

#include <android/sensor.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "Android/JniUtils.h"
#include "Vsync.h"
#include "Distortion.h"
#include "VrApi_Android.h"

#include "../embedded/oculus_loading_indicator.h"

#if defined( OVR_ENABLE_CAPTURE )
#include "capture/Capture.h"
#include "capture/Capture_GLES3.h"
#endif

ovrSensorState ovr_GetSensorStateInternal( double absTime );
bool ovr_ProcessLatencyTest( unsigned char rgbColorOut[3] );
const char * ovr_GetLatencyTestResult();


NV_NAMESPACE_BEGIN

static const int WARP_TESSELATION = 32;	// This is probably a bit too low, I can see some wiggle.

// async	drawn by an independent thread
// sync		drawn by the same thread that draws the eye buffers
//
// frontBuffer	drawn directly to the front buffer with danger of tearing
// swappedBuffer drawn to a swapped buffer with danger of missing a flip
//
// portrait		display scans the left eye completely before scanning the right
// landscape	display scans both eyes simultaneously (DK1, not any of the mobile displays)
//
// Note that the OpenGL buffer may be rotated by hardware, and does not
// necessarily match the scanning orientation -- a landscape Android app is still
// displayed on a portrait scanned display.

// We will need additional swapPrograms for global shutter low-persistence displays
// with the same prediction value used for all points.

// The values reported by the latency tester will tend to be
// 8 milliseconds longer than the prediction values used here,
// because the tester event pulse will happen at some random point
// during the frame previous to the sensor sampling to render
// an eye.  The IMU updates 500 or 1000 times a second, so there
// is only a millisecond or so of jitter.

// The target vsync will always go up by at least one after each
// warp, which should prevent the swapped versions from ever falling into
// triple buffering and getting an additional frame of latency.
//
// It may need to go up by more than one if warping is falling behind
// the video rate, otherwise front buffer rendering could happen
// prematurely, or swapped rendering could go into triple buffering.
//
// On entry to WarpToScreen()
// nextVsync = floor( currentVsync ) + 1.0

// If we have reliable GPU scheduling and front buffer rendering,
// try to warp each eye exactly half a frame ahead.
swapProgram_t	spAsyncFrontBufferPortrait = {
        false,	false, { 0.5, 1.0},	{ {1.0, 1.5}, {1.5, 2.0} }
};

// If we have reliable GPU scheduling, but don't have front
// buffer rendering, the warp thread should still wait until
// mid frame before rendering the second eye, reducing latency.
swapProgram_t	spAsyncSwappedBufferPortrait = {
        false,	false, { 0.0, 0.5},	{ {1.0, 1.5}, {1.5, 2.0} }
};

// If a single thread of control is doing the warping as well
// as the eye rendering, we will usually already be in the second half
// of the scanout, but we may still need to wait for it if the eye
// rendering was unusually quick.  We will then need to wait for
// vsync to start the right eye rendering.
swapProgram_t	spSyncFrontBufferPortrait = {
        true,	false, { 0.5, 1.0},	{ {1.0, 1.5}, {1.5, 2.0} }
};

// If we are drawing to a swapped buffer, we don't want to wait at all,
// for fear of missing the swap point and dropping the frame.
// The true prediction timings for android would be 3,3.5,3.5,4, but
// that is way too much prediction.
swapProgram_t	spSyncSwappedBufferPortrait = {
        true,	false, { 0.0, 0.0},	{ {2.0, 2.5}, {2.5, 3.0} }
};

// Landscape scanned displays, like Rift DK1, will
// always have identical top and bottom predictions.
// For front buffer landscape rendering, We may want to split each eye
// into a pair of renderings with a wait in the middle to avoid
// tight timing near the bottom of the frame and assumptions about
// the speed and raster order of warping.

static const char * DefaultDistortionFile = "/Oculus/defaultDistortion.bin";

//=========================================================================================

// If lensCentered, the coordinates will be square and extend past the left and
// right edges for a viewport.
//
// Otherwise, it will cover only the pixels in the correct half of the screen
// for a scissor rect.

// If the window is wider than it is tall, ie 1920x1080
void EyeRectLandscape( const hmdInfoInternal_t & hmd, const ScreenEye eye,
                       const WhichEyeRect rect, int &x, int &y, int &width, int &height )
{
    if ( rect == RECT_SCREEN )
    {
        // always scissor exactly to half the screen
        int scissorX = ( eye == 0 ) ? 0 : hmd.widthPixels / 2;
        int scissorY = 0;
        int scissorWidth = hmd.widthPixels / 2;
        int scissorHeight = hmd.heightPixels;
        x = scissorX;
        y = scissorY;
        width = scissorWidth;
        height = scissorHeight;
        return;
    }

    const float	metersToPixels = hmd.widthPixels / hmd.widthMeters;

    // Even though the lens center is shifted outwards slightly,
    // the height is still larger than the largest horizontal value.
    // TODO: check for sure on other HMD
    const int	pixelRadius = hmd.heightPixels / 2;
    const int	pixelDiameter = pixelRadius * 2;
    const float	horizontalShiftMeters = ( hmd.lensSeparation / 2 ) - ( hmd.widthMeters / 4 );
    const float	horizontalShiftPixels = horizontalShiftMeters * metersToPixels;

    // Make a viewport that is symetric, extending off the sides of the screen and into the other half.
    x = hmd.widthPixels/4 - pixelRadius + ( ( eye == 0 ) ? - horizontalShiftPixels : hmd.widthPixels/2 + horizontalShiftPixels );
    y = 0;
    width = pixelDiameter;
    height = pixelDiameter;
}

void EyeRect( const hmdInfoInternal_t & hmd, const ScreenEye eye, const WhichEyeRect rect,
              int &x, int &y, int &width, int &height )
{
    int	lx, ly, lWidth, lHeight;
    EyeRectLandscape( hmd, eye, rect, lx, ly, lWidth, lHeight );

    x = lx;
    y = ly;
    width = lWidth;
    height = lHeight;
}

Matrix4f CalculateTimeWarpMatrix2( const Quatf &inFrom, const Quatf &inTo )
{
    // FIXME: this is a horrible hack to fix a zero quaternion that's passed in
    // the night before a demo. This is coming from the sensor pose and needs to
    // be tracked down further.
    Quatf from = inFrom;
    Quatf to = inTo;

    bool fromValid = from.LengthSq() > 0.95f;
    bool toValid = to.LengthSq() > 0.95f;
    if ( !fromValid )
    {
        if ( toValid )
        {
            from = to;
        }
        else
        {
            from = Quatf( 0.0f, 0.0f, 0.0f, 1.0f ); // just force identity
        }
    }
    if ( !toValid )
    {
        if ( fromValid )
        {
            to = from;
        }
        else
        {
            to = Quatf( 0.0f, 0.0f, 0.0f, 1.0f ); // just force identity
        }
    }

    Matrix4f		lastSensorMatrix = Matrix4f( to );
    Matrix4f		lastViewMatrix = Matrix4f( from );

    return ( lastSensorMatrix.Inverted() * lastViewMatrix ).Inverted();
}

//=========================================================================================

// Shim to call a C++ object from a posix thread start.
void *VFrameSmooth::ThreadStarter( void * parm )
{
    VFrameSmooth & tw = *(VFrameSmooth *)parm;
    tw.threadFunction();
    return NULL;
}

void VFrameSmooth::threadFunction()
{
    warpThreadInit();

    // Signal the main thread to wake up and return.
    pthread_mutex_lock( &m_swapMutex );
    pthread_cond_signal( &m_swapIsLatched );
    pthread_mutex_unlock( &m_swapMutex );

    LOG( "WarpThreadLoop()" );

    bool removedSchedFifo = false;

    // Loop until we get a shutdown request
    for ( double vsync = 0; ; vsync++ )
    {
        const double current = ceil( GetFractionalVsync() );
        if ( abs( current - vsync ) > 2.0 )
        {
            LOG( "Changing vsync from %f to %f", vsync, current );
            vsync = current;
        }
        if ( m_shutdownRequest.state() )
        {
            LOG( "ShutdownRequest received" );
            break;
        }

        // The time warp thread functions as a watch dog for the calling thread.
        // If the calling thread does not call WarpSwap() in a long time then this code
        // removes SCHED_FIFO from the calling thread to keep the Android watch dog from
        // rebooting the device.
        // SCHED_FIFO is set back as soon as the calling thread calls WarpSwap() again.
        if ( m_setSchedFifoMethodId != 0 )
        {
            const double currentTime = ovr_GetTimeInSeconds();
            const double lastWarpTime = m_lastWarpSwapTimeInSeconds.state();
            if ( removedSchedFifo )
            {
                if ( lastWarpTime > currentTime - 0.1 )
                {
                    LOG( "VFrameSmooth Watchdog: restored SCHED_FIFO on tid %d", m_sStartupTid );
                    m_jni->CallStaticIntMethod( m_initParms.vrLibClass, m_setSchedFifoMethodId, m_initParms.activityObject, m_sStartupTid, SCHED_FIFO_PRIORITY_VRTHREAD );
                    if ( m_initParms.gameThreadTid )
                    {
                        LOG( "VFrameSmooth Watchdog: restored SCHED_FIFO on tid %d", m_initParms.gameThreadTid );
                        m_jni->CallStaticIntMethod( m_initParms.vrLibClass, m_setSchedFifoMethodId, m_initParms.activityObject, m_initParms.gameThreadTid, SCHED_FIFO_PRIORITY_VRTHREAD );
                    }
                    removedSchedFifo = false;
                }
            }
            else
            {
                if ( lastWarpTime < currentTime - 1.0 )
                {
                    LOG( "VFrameSmooth Watchdog: removed SCHED_FIFO from tid %d", m_sStartupTid );
                    m_jni->CallStaticIntMethod( m_initParms.vrLibClass, m_setSchedFifoMethodId, m_initParms.activityObject, m_sStartupTid, SCHED_FIFO_PRIORITY_NONE );
                    if ( m_initParms.gameThreadTid )
                    {
                        LOG( "VFrameSmooth Watchdog: removed SCHED_FIFO from tid %d", m_initParms.gameThreadTid );
                        m_jni->CallStaticIntMethod( m_initParms.vrLibClass, m_setSchedFifoMethodId, m_initParms.activityObject, m_initParms.gameThreadTid, SCHED_FIFO_PRIORITY_NONE );
                    }
                    removedSchedFifo = true;
                }
            }
        }

        warpToScreen( vsync,
                      m_screen.isFrontBuffer() ? spAsyncFrontBufferPortrait
                                               : spAsyncSwappedBufferPortrait );
    }

    warpThreadShutdown();

    LOG( "Exiting WarpThreadLoop()" );
}


// Not in extension string, unfortunately.
static bool IsContextPriorityExtensionPresent()
{
    EGLint currentPriorityLevel = -1;
    if ( !eglQueryContext( eglGetCurrentDisplay(), eglGetCurrentContext(), EGL_CONTEXT_PRIORITY_LEVEL_IMG, &currentPriorityLevel )
         || currentPriorityLevel == -1 )
    {
        // If we can't report the priority, assume the extension isn't there
        return false;
    }
    return true;
}

VFrameSmooth * VFrameSmooth::Factory( TimeWarpInitParms initParms )
{
    return new VFrameSmooth( initParms );
}

/*
* Startup()
*/
VFrameSmooth::VFrameSmooth( const TimeWarpInitParms initParms ) :
        m_untexturedMvpProgram(),
        m_debugLineProgram(),
        m_warpPrograms(),
        m_blackTexId( 0 ),
        m_defaultLoadingIconTexId( 0 ),
        m_hasEXT_sRGB_write_control( false ),
        m_sStartupTid( 0 ),
        m_jni( NULL ),
        m_setSchedFifoMethodId( 0 ),
        m_updateTexImageMethodId( 0 ),
        m_getTimestampMethodId( 0 ),
        m_eglDisplay( 0 ),
        m_eglPbufferSurface( 0 ),
        m_eglMainThreadSurface( 0 ),
        m_eglConfig( 0 ),
        m_eglClientVersion( 0 ),
        m_eglShareContext( 0 ),
        m_eglWarpContext( 0 ),
        m_contextPriority( 0 ),
        m_eyeLog(),
        m_lastEyeLog( 0 ),
        m_logEyeWarpGpuTime(),
        m_warpThread( 0 ),
        m_warpThreadTid( 0 ),
        m_lastSwapVsyncCount( 0 )
{
    // Code which auto-disable chromatic aberration expects
    // the warpProgram list to be symmetric.
    // See disableChromaticCorrection.
    OVR_COMPILER_ASSERT( ( WP_PROGRAM_MAX & 1 ) == 0 );
    OVR_COMPILER_ASSERT( ( WP_CHROMATIC - WP_SIMPLE ) ==
                         ( WP_PROGRAM_MAX - WP_CHROMATIC ) );

    m_shutdownRequest.setState( false );
    m_eyeBufferCount.setState( 0 );
    memset( m_warpSources, 0, sizeof( m_warpSources ) );
    memset( m_warpPrograms, 0, sizeof( m_warpPrograms ) );

    // set up our synchronization primitives
    pthread_mutex_init( &m_swapMutex, NULL /* default attributes */ );
    pthread_cond_init( &m_swapIsLatched, NULL /* default attributes */ );

    LOG( "-------------------- VFrameSmooth() --------------------" );

    // Only allow WarpSwap() to be called from this thread
    m_sStartupTid = gettid();

    // Keep track of the last time WarpSwap() was called.
    m_lastWarpSwapTimeInSeconds.setState( ovr_GetTimeInSeconds() );

    // If this isn't set, Shutdown() doesn't need to kill the thread
    m_warpThread = 0;

    // Save our startup parms
    m_initParms = initParms;

    // No buffers have been submitted yet
    m_eyeBufferCount.setState( 0 );

    //---------------------------------------------------------
    // OpenGL initialization that can be done on the main thread
    //---------------------------------------------------------

    // Get values for the current OpenGL context
    m_eglDisplay = eglGetCurrentDisplay();
    if ( m_eglDisplay == EGL_NO_DISPLAY )
    {
        FAIL( "EGL_NO_DISPLAY" );
    }
    m_eglMainThreadSurface = eglGetCurrentSurface( EGL_DRAW );
    if ( m_eglMainThreadSurface == EGL_NO_SURFACE )
    {
        FAIL( "EGL_NO_SURFACE" );
    }
    m_eglShareContext = eglGetCurrentContext();
    if ( m_eglShareContext == EGL_NO_CONTEXT )
    {
        FAIL( "EGL_NO_CONTEXT" );
    }
    EGLint configID;
    if ( !eglQueryContext( m_eglDisplay, m_eglShareContext, EGL_CONFIG_ID, &configID ) )
    {
        FAIL( "eglQueryContext EGL_CONFIG_ID failed" );
    }
    m_eglConfig = EglConfigForConfigID( m_eglDisplay, configID );
    if ( m_eglConfig == NULL )
    {
        FAIL( "EglConfigForConfigID failed" );
    }
    if ( !eglQueryContext( m_eglDisplay, m_eglShareContext, EGL_CONTEXT_CLIENT_VERSION, (EGLint *)&m_eglClientVersion ) )
    {
        FAIL( "eglQueryContext EGL_CONTEXT_CLIENT_VERSION failed" );
    }
    LOG( "Current EGL_CONTEXT_CLIENT_VERSION:%i", m_eglClientVersion );

    // It is wasteful for the main config to be anything but a color buffer.
    EGLint depthSize = 0;
    eglGetConfigAttrib( m_eglDisplay, m_eglConfig, EGL_DEPTH_SIZE, &depthSize );
    if ( depthSize != 0 )
    {
        LOG( "Share context eglConfig has %i depth bits -- should be 0", depthSize );
    }

    EGLint samples = 0;
    eglGetConfigAttrib( m_eglDisplay, m_eglConfig, EGL_SAMPLES, &samples );
    if ( samples != 0 )
    {
        LOG( "Share context eglConfig has %i samples -- should be 0", samples );
    }

    // See if we have sRGB_write_control extension
    m_hasEXT_sRGB_write_control = GL_ExtensionStringPresent( "GL_EXT_sRGB_write_control",
                                                             (const char *)glGetString( GL_EXTENSIONS ) );

    // Skip thread initialization if we are running synchronously
    if ( !m_initParms.asynchronousTimeWarp )
    {
        // The current thread is presumably already attached, so this
        // should just return a valid environment.
        const jint rtn = m_initParms.javaVm->AttachCurrentThread( &m_jni, 0 );
        if ( rtn != JNI_OK )
        {
            FAIL( "AttachCurrentThread() returned %i", rtn );
        }

        m_screen.initForCurrentSurface( m_jni, m_initParms.frontBuffer, m_initParms.buildVersionSDK );

        if ( m_screen.windowSurface == EGL_NO_SURFACE )
        {
            FAIL( "Screen.windowSurface == EGL_NO_SURFACE" );
        }


        // create the framework graphics on this thread
        createFrameworkGraphics();
        LOG( "Skipping thread setup because !AsynchronousTimeWarp" );
    }
    else
    {
        //---------------------------------------------------------
        // Thread initialization
        //---------------------------------------------------------

        // Make GL current on the pbuffer, because the window will be
        // used by the background thread.

        if ( IsContextPriorityExtensionPresent() )
        {
            LOG( "Requesting EGL_CONTEXT_PRIORITY_HIGH_IMG" );
            m_contextPriority = EGL_CONTEXT_PRIORITY_HIGH_IMG;
        }
        else
        {
            // If we can't report the priority, assume the extension isn't there
            LOG( "IMG_Context_Priority doesn't seem to be present." );
            m_contextPriority = EGL_CONTEXT_PRIORITY_MEDIUM_IMG;
        }

        // Detach the windowSurface from the current context, because the
        // windowSurface will be used by the context on the background thread.
        //
        // Because EGL_KHR_surfaceless_context is not widespread (Only on Tegra as of
        // September 2013), we need to create and attach a tiny pbuffer surface to the
        // current context.
        //
        // It is necessary to use a config with the same characteristics that the
        // context was created with, plus the pbuffer flag, or we will get an
        // EGL_BAD_MATCH error on the eglMakeCurrent() call.
        const EGLint attrib_list[] =
                {
                        EGL_WIDTH, 16,
                        EGL_HEIGHT, 16,
                        EGL_NONE
                };
        m_eglPbufferSurface = eglCreatePbufferSurface( m_eglDisplay, m_eglConfig, attrib_list );
        if ( m_eglPbufferSurface == EGL_NO_SURFACE )
        {
            FAIL( "eglCreatePbufferSurface failed: %s", EglErrorString() );
        }

        if ( eglMakeCurrent( m_eglDisplay, m_eglPbufferSurface, m_eglPbufferSurface,
                             m_eglShareContext ) == EGL_FALSE )
        {
            FAIL( "eglMakeCurrent: eglMakeCurrent pbuffer failed" );
        }

        // The thread will exit when this is set true.
        m_shutdownRequest.setState( false );

        // Grab the mutex before spawning the warp thread, so it won't be
        // able to exit until we do the pthread_cond_wait
        pthread_mutex_lock( &m_swapMutex );

        // spawn the warp thread
        const int createErr = pthread_create( &m_warpThread, NULL /* default attributes */, &ThreadStarter, this );
        if ( createErr != 0 )
        {
            FAIL( "pthread_create returned %i", createErr );
        }

        // Atomically unlock the mutex and block until the warp thread
        // has completed the initialization and either failed or went
        // into WarpThreadLoop()
        pthread_cond_wait( &m_swapIsLatched, &m_swapMutex );

        // Pthread_cond_wait re-locks the mutex before exit.
        pthread_mutex_unlock( &m_swapMutex );
    }

    LOG( "----------------- VFrameSmooth() End -----------------" );
}

/*
* Shutdown()
*/
VFrameSmooth::~VFrameSmooth()
{
    LOG( "---------------- ~VFrameSmooth() Start ----------------" );
    if ( m_warpThread != 0 )
    {
        // Get the background thread to kill itself.
        m_shutdownRequest.setState( true );

        LOG( "pthread_join() called");
        void * data;
        pthread_join( m_warpThread, &data );

        LOG( "pthread_join() returned");

        m_warpThread = 0;

        if ( eglGetCurrentSurface( EGL_DRAW ) != m_eglPbufferSurface )
        {
            LOG( "eglGetCurrentSurface( EGL_DRAW ) != eglPbufferSurface" );
        }

        // Attach the windowSurface to the calling context again.
        if ( eglMakeCurrent( m_eglDisplay, m_eglMainThreadSurface,
                             m_eglMainThreadSurface, m_eglShareContext ) == EGL_FALSE)
        {
            FAIL( "eglMakeCurrent to window failed: %s", EglErrorString() );
        }

        // Destroy the pbuffer surface that was attached to the calling context.
        if ( EGL_FALSE == eglDestroySurface( m_eglDisplay, m_eglPbufferSurface ) )
        {
            WARN( "Failed to destroy pbuffer." );
        }
        else
        {
            LOG( "Destroyed pbuffer." );
        }
    }
    else
    {
        // Shutdown the front buffer rendering
        m_screen.shutdown();

        // Vertex array objects can only be destroyed on the context they were created on
        // If there is no warp thread then InitParms.AsynchronousTimeWarp was false and
        // CreateFrameworkGraphics() was called from the VFrameSmooth constructor.
        destroyFrameworkGraphics();
    }

    LOG( "---------------- ~VFrameSmooth() End ----------------" );
}

/*
* WarpThreadInit()
*/
void VFrameSmooth::warpThreadInit()
{
    LOG( "WarpThreadInit()" );

    pthread_setname_np( pthread_self(), "NervGear::VFrameSmooth" );

    //SetCurrentThreadAffinityMask( 0xF0 );

    //---------------------------------------------------------
    // OpenGl initiailization
    //
    // On windows, GL context creation must be done on the thread that is
    // going to use the context, or random failures will occur.  This may
    // not be necessary on other platforms, but do it anyway.
    //---------------------------------------------------------

    // Create a new GL context on this thread, sharing it with the main thread context
    // so the render targets can be passed back and forth.

    EGLint contextAttribs[] =
            {
                    EGL_CONTEXT_CLIENT_VERSION, m_eglClientVersion,
                    EGL_NONE, EGL_NONE,
                    EGL_NONE
            };
    // Don't set EGL_CONTEXT_PRIORITY_LEVEL_IMG at all if set to EGL_CONTEXT_PRIORITY_MEDIUM_IMG,
    // It is the caller's responsibility to use that if the driver doesn't support it.
    if ( m_contextPriority != EGL_CONTEXT_PRIORITY_MEDIUM_IMG )
    {
        contextAttribs[2] = EGL_CONTEXT_PRIORITY_LEVEL_IMG;
        contextAttribs[3] = m_contextPriority;
    }

    m_eglWarpContext = eglCreateContext( m_eglDisplay, m_eglConfig, m_eglShareContext, contextAttribs );
    if ( m_eglWarpContext == EGL_NO_CONTEXT )
    {
        FAIL( "eglCreateContext failed: %s", EglErrorString() );
    }
    LOG( "eglWarpContext: %p", m_eglWarpContext );
    if ( m_contextPriority != EGL_CONTEXT_PRIORITY_MEDIUM_IMG )
    {
        // See what context priority we actually got
        EGLint actualPriorityLevel;
        eglQueryContext( m_eglDisplay, m_eglWarpContext, EGL_CONTEXT_PRIORITY_LEVEL_IMG, &actualPriorityLevel );
        switch ( actualPriorityLevel )
        {
            case EGL_CONTEXT_PRIORITY_HIGH_IMG: LOG( "Context is EGL_CONTEXT_PRIORITY_HIGH_IMG" ); break;
            case EGL_CONTEXT_PRIORITY_MEDIUM_IMG: LOG( "Context is EGL_CONTEXT_PRIORITY_MEDIUM_IMG" ); break;
            case EGL_CONTEXT_PRIORITY_LOW_IMG: LOG( "Context is EGL_CONTEXT_PRIORITY_LOW_IMG" ); break;
            default: LOG( "Context has unknown priority level" ); break;
        }
    }

    // Make the context current on the window, so no more makeCurrent calls will be needed
    LOG( "eglMakeCurrent on %p", m_eglMainThreadSurface );
    if ( eglMakeCurrent( m_eglDisplay, m_eglMainThreadSurface,
                         m_eglMainThreadSurface, m_eglWarpContext ) == EGL_FALSE )
    {
        FAIL( "eglMakeCurrent failed: %s", EglErrorString() );
    }

    // Get a jni environment for front buffer setting, SurfaceTexture updating and changing SCHED_FIFO
    const jint rtn = m_initParms.javaVm->AttachCurrentThread( &m_jni, 0 );
    if ( rtn != JNI_OK )
    {
        FAIL( "AttachCurrentThread() returned %i", rtn );
    }
    m_setSchedFifoMethodId = JniUtils::GetStaticMethodID( m_jni, m_initParms.vrLibClass, "setSchedFifoStatic", "(Landroid/app/Activity;II)I" );

    static const char * className = "android/graphics/SurfaceTexture";
    const jclass surfaceTextureClass = m_jni->FindClass(className);
    if ( surfaceTextureClass == 0 ) {
        FAIL( "FindClass( %s ) failed", className );
    }
    m_updateTexImageMethodId = m_jni->GetMethodID( surfaceTextureClass, "updateTexImage", "()V" );
    if ( !m_updateTexImageMethodId ) {
        FAIL( "couldn't get updateTexImageMethodId" );
    }
    m_getTimestampMethodId = m_jni->GetMethodID( surfaceTextureClass, "getTimestamp", "()J" );
    if ( !m_getTimestampMethodId ) {
        FAIL( "couldn't get GetTimestampMethodId" );
    }
    m_jni->DeleteLocalRef( surfaceTextureClass );

    // Make the current window into a front-buffer
    // Must be called after a context is current on the window.
    m_screen.initForCurrentSurface( m_jni, m_initParms.frontBuffer, m_initParms.buildVersionSDK );

    // create the framework graphics on this thread
    createFrameworkGraphics();

    // Get the linux tid so App can set SCHED_FIFO on it
    m_warpThreadTid = gettid();

    LOG( "WarpThreadInit() - End" );
}

/*
* WarpThreadShutdown()
*/
void VFrameSmooth::warpThreadShutdown()
{
    LOG( "WarpThreadShutdown()" );

    // Vertex array objects can only be destroyed on the context they were created on
    destroyFrameworkGraphics();

    // Destroy the sync objects
    for ( int i = 0; i < MAX_WARP_SOURCES; i++ )
    {
        warpSource_t & ws = m_warpSources[i];
        if ( ws.GpuSync )
        {
            if ( EGL_FALSE == eglDestroySyncKHR_( m_eglDisplay, ws.GpuSync ) )
            {
                LOG( "eglDestroySyncKHR returned EGL_FALSE" );
            }
            ws.GpuSync = 0;
        }
    }

    // Shutdown the front buffer rendering
    m_screen.shutdown();

    // release the window so it can be made current by another thread
    if ( eglMakeCurrent( m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE,
                         EGL_NO_CONTEXT ) == EGL_FALSE )
    {
        FAIL( "eglMakeCurrent: shutdown failed" );
    }

    if ( eglDestroyContext( m_eglDisplay, m_eglWarpContext ) == EGL_FALSE )
    {
        FAIL( "eglDestroyContext: shutdown failed" );
    }
    m_eglWarpContext = 0;

    const jint rtn = m_initParms.javaVm->DetachCurrentThread();
    if ( rtn != JNI_OK )
    {
        FAIL( "DetachCurrentThread() returned %i", rtn );
    }

    LOG( "WarpThreadShutdown() - End" );
}

const VGlShader & VFrameSmooth::programForParms( const ovrTimeWarpParms & parms, const bool disableChromaticCorrection ) const
{
    int program = Alg::Clamp( (int)parms.WarpProgram, (int)WP_SIMPLE, (int)WP_PROGRAM_MAX - 1 );

    if ( disableChromaticCorrection && program >= WP_CHROMATIC )
    {
        program -= ( WP_CHROMATIC - WP_SIMPLE );
    }
    return m_warpPrograms[program];
}

void VFrameSmooth::setWarpState( const warpSource_t & currentWarpSource ) const
{
    glDepthMask( GL_FALSE );	// don't write to depth, even if Unity has depth on window
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_CULL_FACE );
    glDisable( GL_BLEND );
    glEnable( GL_SCISSOR_TEST );

    // sRGB conversion on write will only happen if BOTH the window
    // surface was created with EGL_GL_COLORSPACE_KHR,  EGL_GL_COLORSPACE_SRGB_KHR
    // and GL_FRAMEBUFFER_SRGB_EXT is enabled.
    if ( m_hasEXT_sRGB_write_control )
    {
        if ( currentWarpSource.WarpParms.WarpOptions & SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER )
        {
            glDisable( GL_FRAMEBUFFER_SRGB_EXT );
        }
        else
        {
            glEnable( GL_FRAMEBUFFER_SRGB_EXT );
        }
    }
    GL_CheckErrors( "SetWarpState" );
}

void VFrameSmooth::bindWarpProgram( const warpSource_t & currentWarpSource,
                                     const Matrix4f timeWarps[2][2], const Matrix4f rollingWarp,
                                     const int eye, const double vsyncBase /* for spinner */ ) const
{
    // TODO: bake this into the buffer objects
    const Matrix4f landscapeOrientationMatrix(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f );

    // Select the warp program.
    const VGlShader & warpProg = programForParms( currentWarpSource.WarpParms, currentWarpSource.disableChromaticCorrection );
    glUseProgram( warpProg.program );

    // Set the shader parameters.
    glUniform1f( warpProg.uniformColor, currentWarpSource.WarpParms.ProgramParms[0] );

    glUniformMatrix4fv( warpProg.uniformModelViewProMatrix, 1, GL_FALSE, landscapeOrientationMatrix.Transposed().M[0] );
    glUniformMatrix4fv( warpProg.uniformTexMatrix, 1, GL_FALSE, timeWarps[0][0].Transposed().M[0] );
    glUniformMatrix4fv( warpProg.uniformTexMatrix2, 1, GL_FALSE, timeWarps[0][1].Transposed().M[0] );
    if ( warpProg.uniformTexMatrix3 > 0 )
    {
        glUniformMatrix4fv( warpProg.uniformTexMatrix3, 1, GL_FALSE, timeWarps[1][0].Transposed().M[0] );
        glUniformMatrix4fv( warpProg.uniformTexMatrix4, 1, GL_FALSE, timeWarps[1][1].Transposed().M[0] );
    }
    if ( warpProg.uniformTexMatrix5 > 0 )
    {
        glUniformMatrix4fv( warpProg.uniformTexMatrix5, 1, GL_FALSE, rollingWarp.Transposed().M[0] );
    }
    if ( warpProg.uniformTexClamp > 0 )
    {
        // split screen clamping for UE4
        const Vector2f clamp( eye * 0.5f, (eye+1)* 0.5f );
        glUniform2fv( warpProg.uniformTexClamp, 1, &clamp.x );
    }
    if ( warpProg.uniformRotateScale > 0 )
    {
        const float angle = FramePointTimeInSeconds( vsyncBase ) * M_PI * currentWarpSource.WarpParms.ProgramParms[0];
        const Vector4f RotateScale( sinf( angle ), cosf( angle ), currentWarpSource.WarpParms.ProgramParms[1], 1.0f );
        glUniform4fv( warpProg.uniformRotateScale, 1, &RotateScale[0] );
    }
}

void VFrameSmooth::bindCursorProgram() const
{
    // TODO: bake this into the buffer objects
    const Matrix4f landscapeOrientationMatrix(
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f );

    // Select the warp program.
    const VGlShader & warpProg = m_warpPrograms[ WP_SIMPLE ];
    glUseProgram( warpProg.program );

    // Set the shader parameters.
    glUniform1f( warpProg.uniformColor, 1.0f );

    glUniformMatrix4fv( warpProg.uniformModelViewProMatrix, 1, GL_FALSE, landscapeOrientationMatrix.Transposed().M[0] );
    glUniformMatrix4fv( warpProg.uniformTexMatrix, 1, GL_FALSE, Matrix4f::Identity().M[0] );
    glUniformMatrix4fv( warpProg.uniformTexMatrix2, 1, GL_FALSE, Matrix4f::Identity().M[0] );
}

int CameraTimeWarpLatency = 4;
bool CameraTimeWarpPause;

static void UpdateSurfaceTexture( JNIEnv * Jni, jobject surfaceTexture, jmethodID UpdateTexImageMethodId,
                                  jmethodID GetTimestampMethodId, ovrPoseStatef & topPose, ovrPoseStatef & bottomPose )
{
    if ( !surfaceTexture )
    {
        return;
    }

    // Assume this is called at 120 hz
    static const int NUM_POSES = 16;
    static ovrPoseStatef loggedPose[NUM_POSES];
    static int poseNum;
    poseNum++;
    loggedPose[poseNum&(NUM_POSES-1)] = ovr_GetSensorStateInternal( ovr_GetTimeInSeconds() ).Predicted;

    // We might need to discard a frame if more than one
    // is in the queue.
    static long long prevTimeStamp;
    static ovrPoseStatef	prevTopPose;
    static ovrPoseStatef	prevBottomPose;
    while( !CameraTimeWarpPause )
    {
        // Allow the Java SurfaceTexture object to swap in an updated image.
        Jni->CallVoidMethod( surfaceTexture, UpdateTexImageMethodId );
        const long long nanoTimeStamp = Jni->CallLongMethod( surfaceTexture, GetTimestampMethodId );

        if ( prevTimeStamp == nanoTimeStamp )
        {
            break;
        }
        prevTimeStamp = nanoTimeStamp;
        prevTopPose = loggedPose[(poseNum - CameraTimeWarpLatency)&(NUM_POSES-1)];
        prevBottomPose = loggedPose[(poseNum - CameraTimeWarpLatency+1)&(NUM_POSES-1)];
    }
    topPose = prevTopPose;
    bottomPose = prevBottomPose;
}

static void BindEyeTextures( const warpSource_t & currentWarpSource, const ScreenEye eye )
{
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, currentWarpSource.WarpParms.Images[eye][0].TexId );
    if ( HasEXT_sRGB_texture_decode )
    {
        if ( currentWarpSource.WarpParms.WarpOptions & SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER )
        {
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT );
        }
        else
        {
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT, GL_DECODE_EXT );
        }
    }

    if ( currentWarpSource.WarpParms.WarpProgram == WP_MASKED_PLANE
         || currentWarpSource.WarpParms.WarpProgram == WP_CHROMATIC_MASKED_PLANE
         || currentWarpSource.WarpParms.WarpProgram == WP_OVERLAY_PLANE
         || currentWarpSource.WarpParms.WarpProgram == WP_CHROMATIC_OVERLAY_PLANE
         || currentWarpSource.WarpParms.WarpProgram == WP_OVERLAY_PLANE_SHOW_LOD
         || currentWarpSource.WarpParms.WarpProgram == WP_CHROMATIC_OVERLAY_PLANE_SHOW_LOD
            )
    {
        glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_2D, currentWarpSource.WarpParms.Images[eye][1].TexId );
        if ( HasEXT_sRGB_texture_decode )
        {
            if ( currentWarpSource.WarpParms.WarpOptions & SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER )
            {
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT );
            }
            else
            {
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT, GL_DECODE_EXT );
            }
        }
    }
    if ( currentWarpSource.WarpParms.WarpProgram == WP_MASKED_PLANE_EXTERNAL
         || currentWarpSource.WarpParms.WarpProgram == WP_CHROMATIC_MASKED_PLANE_EXTERNAL
         || currentWarpSource.WarpParms.WarpProgram == WP_CAMERA
         || currentWarpSource.WarpParms.WarpProgram == WP_CHROMATIC_CAMERA )
    {
        glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_EXTERNAL_OES, currentWarpSource.WarpParms.Images[eye][1].TexId );
        if ( HasEXT_sRGB_texture_decode )
        {
            if ( currentWarpSource.WarpParms.WarpOptions & SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER )
            {
                glTexParameteri( GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT );
            }
            else
            {
                glTexParameteri( GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_SRGB_DECODE_EXT, GL_DECODE_EXT );
            }
        }
    }
    if ( currentWarpSource.WarpParms.WarpProgram == WP_MASKED_CUBE || currentWarpSource.WarpParms.WarpProgram == WP_CHROMATIC_MASKED_CUBE )
    {
        glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_CUBE_MAP, currentWarpSource.WarpParms.Images[eye][1].TexId );
        if ( HasEXT_sRGB_texture_decode )
        {
            if ( currentWarpSource.WarpParms.WarpOptions & SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER )
            {
                glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT );
            }
            else
            {
                glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_SRGB_DECODE_EXT, GL_DECODE_EXT );
            }
        }
    }

    if ( currentWarpSource.WarpParms.WarpProgram == WP_CUBE || currentWarpSource.WarpParms.WarpProgram == WP_CHROMATIC_CUBE )
    {
        for ( int i = 0; i < 3; i++ )
        {
            glActiveTexture( GL_TEXTURE1 + i );
            glBindTexture( GL_TEXTURE_CUBE_MAP, currentWarpSource.WarpParms.Images[eye][1].PlanarTexId[i] );
            if ( HasEXT_sRGB_texture_decode )
            {
                if ( currentWarpSource.WarpParms.WarpOptions & SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER )
                {
                    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT );
                }
                else
                {
                    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_SRGB_DECODE_EXT, GL_DECODE_EXT );
                }
            }
        }
    }

    if ( currentWarpSource.WarpParms.WarpProgram == WP_LOADING_ICON || currentWarpSource.WarpParms.WarpProgram == WP_CHROMATIC_LOADING_ICON )
    {
        glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_2D, currentWarpSource.WarpParms.Images[eye][1].TexId );
        if ( HasEXT_sRGB_texture_decode )
        {
            if ( currentWarpSource.WarpParms.WarpOptions & SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER )
            {
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT );
            }
            else
            {
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_SRGB_DECODE_EXT, GL_DECODE_EXT );
            }
        }
    }
}

static void UnbindEyeTextures()
{
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, 0 );

    glActiveTexture( GL_TEXTURE1 );
    glBindTexture( GL_TEXTURE_2D, 0 );
    glBindTexture( GL_TEXTURE_EXTERNAL_OES, 0 );
    glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );

    for ( int i = 1; i < 3; i++ )
    {
        glActiveTexture( GL_TEXTURE1 + i );
        glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );
    }
}

/*
 * WarpToScreen
 *
 * Can be called by either the dedicated warp thread, or by the
 * application thread at swap time if running synchronously.
 *
 * Wait for sync point, read sensor, Warp both eyes and returns the next vsyncBase
 *
 * Calls GetFractionalVsync() multiple times, but this only calls kernel time functions, not java
 * Calls SleepUntilTimePoint() for each eye.
 * May write to the log
 * Writes eyeLog[]
 * Reads warpSources
 * Reads eyeBufferCount
 * May lock and unlock swapMutex
 * May signal swapIsLatched
 * Writes SwapVsync
 *
 */
void VFrameSmooth::warpToScreen( const double vsyncBase_, const swapProgram_t & swap )
{
#if defined( OVR_ENABLE_CAPTURE )
    OVR_CAPTURE_CPU_ZONE( warpToScreen );
#endif
    static double lastReportTime = 0;
    const double timeNow = floor( ovr_GetTimeInSeconds() );
    if ( timeNow > lastReportTime )
    {
        LOG( "Warp GPU time: %3.1f ms", m_logEyeWarpGpuTime.GetTotalTime() );
        lastReportTime = timeNow;
    }

    const warpSource_t & latestWarpSource = m_warpSources[m_eyeBufferCount.state()%MAX_WARP_SOURCES];

    // switch to sliced rendering
    if ( latestWarpSource.WarpParms.WarpOptions & SWAP_OPTION_USE_SLICED_WARP )
    {
        warpToScreenSliced( vsyncBase_, swap );
        return;
    }

    const double vsyncBase = vsyncBase_;

    // Build new line verts if timing graph is enabled
    updateTimingGraphVerts( latestWarpSource.WarpParms.DebugGraphMode, latestWarpSource.WarpParms.DebugGraphValue );

    // This will only be updated in SCREENEYE_LEFT
    warpSource_t currentWarpSource = {};

    // The mesh covers the full screen, but we only draw part of it at a time
    int screenWidth, screenHeight;
    m_screen.getScreenResolution( screenWidth, screenHeight );
    glViewport( 0, 0, screenWidth, screenHeight );
    glScissor( 0, 0, screenWidth, screenHeight );

    // Warp each eye to the display surface
    for ( ScreenEye eye = SCREENEYE_LEFT; eye <= SCREENEYE_RIGHT; eye = (ScreenEye)((int)eye+1) )
    {
#if defined( OVR_ENABLE_CAPTURE )
        OVR_CAPTURE_CPU_ZONE( eye );
#endif
        //LOG( "Eye %i: now=%f  sleepTo=%f", eye, GetFractionalVsync(), vsyncBase + swap.deltaVsync[eye] );

        // Sleep until we are in the correct half of the screen for
        // rendering this eye.  If we are running single threaded,
        // the first eye will probably already be past the sleep point,
        // so only the second eye will be at a dependable time.
        const double sleepTargetVsync = vsyncBase + swap.deltaVsync[eye];
        const double sleepTargetTime = FramePointTimeInSeconds( sleepTargetVsync );
        const float secondsToSleep = SleepUntilTimePoint( sleepTargetTime, false );
        const double preFinish = ovr_GetTimeInSeconds();

        //LOG( "Vsync %f:%i sleep %f", vsyncBase, eye, secondsToSleep );

        // Check for availability of updated eye renderings
        // now that we are about to render.
        long long thisEyeBufferNum = 0;
        int	back;

        if ( eye == SCREENEYE_LEFT )
        {
            const long long latestEyeBufferNum = m_eyeBufferCount.state();
            for ( back = 0; back < MAX_WARP_SOURCES - 1; back++ )
            {
                thisEyeBufferNum = latestEyeBufferNum - back;
                if ( thisEyeBufferNum <= 0 )
                {
                    // just starting, and we don't have any eye buffers to use
                    LOG( "WarpToScreen: No valid Eye Buffers" );
                    break;
                }
                warpSource_t & testWarpSource = m_warpSources[thisEyeBufferNum % MAX_WARP_SOURCES];
                if ( testWarpSource.MinimumVsync > vsyncBase )
                {
                    // a full frame got completed in less time than a single eye; don't use it to avoid stuttering
                    continue;
                }
                if ( testWarpSource.GpuSync == 0 )
                {
                    LOG( "thisEyeBufferNum %lli had 0 sync", thisEyeBufferNum );
                    break;
                }

                if ( Quatf( testWarpSource.WarpParms.Images[eye][0].Pose.Pose.Orientation ).LengthSq() < 1e-18f )
                {
                    LOG( "Bad Pose.Orientation in bufferNum %lli!", thisEyeBufferNum );
                    break;
                }

                const EGLint wait = eglClientWaitSyncKHR_( m_eglDisplay, testWarpSource.GpuSync,
                                                           EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, 0 );
                if ( wait == EGL_TIMEOUT_EXPIRED_KHR )
                {
                    continue;
                }
                if ( wait == EGL_FALSE )
                {
                    LOG( "eglClientWaitSyncKHR returned EGL_FALSE" );
                }

                // This buffer set is good to use
                if ( testWarpSource.FirstDisplayedVsync[eye] == 0 )
                {
                    testWarpSource.FirstDisplayedVsync[eye] = (long long)vsyncBase;
                }
                currentWarpSource = testWarpSource;
                break;
            }

            // Save this sensor state for the next application rendering frame.
            // It is important that this always be done, even if we wind up
            // not rendering anything because there are no current eye buffers.
            {
                SwapState	state;
                state.VsyncCount = (long long)vsyncBase;
                state.EyeBufferCount = thisEyeBufferNum;
                m_swapVsync.setState( state );

                // Wake the VR thread up if it is blocked on us.
                // If the other thread happened to be scheduled out right
                // after locking the mutex, but before waiting on the condition,
                // we would rather it sleep for another frame than potentially
                // miss a raster point here in the time warp thread, so use
                // a trylock() instead of a lock().
                if ( !pthread_mutex_trylock( &m_swapMutex ) )
                {
                    pthread_cond_signal( &m_swapIsLatched );
                    pthread_mutex_unlock( &m_swapMutex );
                }
            }

            if ( m_screen.windowSurface == EGL_NO_SURFACE )
            {
                static int logCount = 0;
                if ( ( logCount++ & 31 ) == 0 )
                {
                    LOG( "WarpToScreen: no valid window surface" );
                }
                return;
            }

            if ( currentWarpSource.WarpParms.Images[eye][0].TexId == 0 )
            {
                // We don't have anything valid to draw, so just sleep until
                // the next time point and check again.
                LOG( "WarpToScreen: Nothing valid to draw" );
                SleepUntilTimePoint( FramePointTimeInSeconds( sleepTargetVsync + 1.0f ), false );
                break;
            }
        }

        // Build up the external velocity transform
        Matrix4f velocity;
        const int velocitySteps = NervGear::Alg::Min( 3, (int)((long long)vsyncBase - currentWarpSource.MinimumVsync) );
        for ( int i = 0; i < velocitySteps; i++ )
        {
            velocity = velocity * currentWarpSource.WarpParms.ExternalVelocity;
        }

        // If we have a second image layer, we will need to calculate
        // a second set of time warps and use a different program.
        const bool dualLayer = ( currentWarpSource.WarpParms.Images[eye][1].TexId > 0 );

        // Calculate predicted poses for the start and end of this eye's
        // raster scanout, so we can warp the best image we have to it.
        //
        // These prediction points will always be in the future, because we
        // need time to do the drawing before the scanout starts.
        //
        // In a portrait scanned display, it is beneficial to have the time warp calculated
        // independently for each eye, giving them the same latency profile.
        Matrix4f timeWarps[2][2];
        ovrSensorState sensor[2];
        for ( int scan = 0; scan < 2; scan++ )
        {
            const double vsyncPoint = vsyncBase + swap.predictionPoints[eye][scan];
            const double timePoint = FramePointTimeInSeconds( vsyncPoint );
            sensor[scan] = ovr_GetSensorStateInternal( timePoint );
            const Matrix4f warp = CalculateTimeWarpMatrix2(
                    currentWarpSource.WarpParms.Images[eye][0].Pose.Pose.Orientation,
                    sensor[scan].Predicted.Pose.Orientation ) * velocity;
            timeWarps[0][scan] = Matrix4f( currentWarpSource.WarpParms.Images[eye][0].TexCoordsFromTanAngles ) * warp;
            if ( dualLayer )
            {
                if ( currentWarpSource.WarpParms.WarpOptions & SWAP_OPTION_FIXED_OVERLAY )
                {	// locked-to-face HUD
                    timeWarps[1][scan] = Matrix4f( currentWarpSource.WarpParms.Images[eye][1].TexCoordsFromTanAngles );
                }
                else
                {	// locked-to-world surface
                    const Matrix4f warp2 = CalculateTimeWarpMatrix2(
                            currentWarpSource.WarpParms.Images[eye][1].Pose.Pose.Orientation,
                            sensor[scan].Predicted.Pose.Orientation ) * velocity;
                    timeWarps[1][scan] = Matrix4f( currentWarpSource.WarpParms.Images[eye][1].TexCoordsFromTanAngles ) * warp2;
                }
            }
        }

        // Update the SurfaceTexture before calculating VFrameSmooth
        if ( currentWarpSource.WarpParms.SurfaceTextureObject )
        {
            ovrPoseStatef	pose[2];
            UpdateSurfaceTexture( m_jni,
                                  (jobject)currentWarpSource.WarpParms.SurfaceTextureObject,
                    m_updateTexImageMethodId, m_getTimestampMethodId, pose[0], pose[1] );
            // Both time warp matrix are from the time at the start of this eye.
            // The rollingWarp matrix will first transform the vector to account
            // for movement in the current eye display roll.
            for ( int scan = 0 ; scan < 2 ; scan++ )
            {
                const Matrix4f warp2 = CalculateTimeWarpMatrix2(
                        pose[scan].Pose.Orientation,
                        sensor[0].Predicted.Pose.Orientation
                );
                timeWarps[1][scan] = Matrix4f( currentWarpSource.WarpParms.Images[eye][1].TexCoordsFromTanAngles ) * warp2;
            }
        }

        // The pass through camera support needs to know the warping from the head motion
        // across the display scan independent of any layers, which may drop frames.
        const Matrix4f rollingWarp = CalculateTimeWarpMatrix2(
                sensor[0].Predicted.Pose.Orientation,
                sensor[1].Predicted.Pose.Orientation );

        //---------------------------------------------------------
        // Warp a latched buffer to the screen
        //---------------------------------------------------------

        m_logEyeWarpGpuTime.Begin( eye );
        m_logEyeWarpGpuTime.PrintTime( eye, "GPU time for eye time warp" );

        setWarpState( currentWarpSource );

        bindWarpProgram( currentWarpSource, timeWarps, rollingWarp, eye, vsyncBase );

        BindEyeTextures( currentWarpSource, eye );

        m_screen.beginDirectRendering( eye * screenWidth/2, 0, screenWidth/2, screenHeight );

        // Draw the warp triangles.
        glBindVertexArrayOES_( m_warpMesh.vertexArrayObject );
        const int indexCount = m_warpMesh.indexCount / 2;
        const int indexOffset = eye * indexCount;
        glDrawElements( GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, (void *)(indexOffset * 2 ) );

        // If the gaze cursor is enabled, render those subsets of triangles
        if ( currentWarpSource.WarpParms.WarpOptions & SWAP_OPTION_SHOW_CURSOR )
        {
            bindCursorProgram();
            glEnable( GL_BLEND );
            glBindVertexArrayOES_( m_cursorMesh.vertexArrayObject );
            const int indexCount = m_cursorMesh.indexCount / 2;
            const int indexOffset = eye * indexCount;
            glDrawElements( GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, (void *)(indexOffset * 2 ) );
            glDisable( GL_BLEND );
        }

        // Draw the screen vignette, calibration grid, and debug graphs.
        // The grid will be based on the orientation that the warpSource
        // was rendered at, so you can see the amount of interpolated time warp applied
        // to it as a delta from the red grid to the greed or blue grid that was drawn directly
        // into the texture.
        drawFrameworkGraphicsToWindow( eye, currentWarpSource.WarpParms.WarpOptions,
                                       currentWarpSource.WarpParms.DebugGraphMode != DEBUG_PERF_OFF );

        m_screen.endDirectRendering();

        m_logEyeWarpGpuTime.End( eye );

        const double justBeforeFinish = ovr_GetTimeInSeconds();
        if ( m_screen.isFrontBuffer() )
        {
            GL_Finish();
        }
        const double postFinish = ovr_GetTimeInSeconds();

        const float latency = postFinish - justBeforeFinish;
        if ( latency > 0.008f )
        {
            LOG( "Frame %i Eye %i latency %5.3f", (int)vsyncBase, eye, latency );
        }

        if ( 0 )
        {
            LOG( "eye %i sleep %5.3f fin %5.3f buf %lli (%i back):%i %i",
                 eye, secondsToSleep,
                 postFinish - preFinish,
                 thisEyeBufferNum, back,
                 currentWarpSource.WarpParms.Images[0][0].TexId,
                 currentWarpSource.WarpParms.Images[1][0].TexId );
        }

        // Update debug graph data
        if ( currentWarpSource.WarpParms.DebugGraphMode != DEBUG_PERF_FROZEN )
        {
            const int logIndex = (int)m_lastEyeLog & (EYE_LOG_COUNT-1);
            eyeLog_t & thisLog = m_eyeLog[logIndex];
            thisLog.skipped = false;
            thisLog.bufferNum = thisEyeBufferNum;
            thisLog.issueFinish = preFinish - sleepTargetTime;
            thisLog.completeFinish = postFinish - sleepTargetTime;
            m_lastEyeLog++;
        }

    }	// for eye

    UnbindEyeTextures();

    glUseProgram( 0 );

    glBindVertexArrayOES_( 0 );

    if ( !m_screen.isFrontBuffer() )
    {
        m_screen.swapBuffers();
    }
}

void VFrameSmooth::warpToScreenSliced( const double vsyncBase, const swapProgram_t & swap )
{
    // Fetch vsync timing information once, so we don't have to worry
    // about it changing slightly inside a given frame.
    const VsyncState vsyncState = UpdatedVsyncState.state();
    if ( vsyncState.vsyncBaseNano == 0 )
    {
        return;
    }

    // all necessary time points can now be calculated

    // Because there are blanking lines at the bottom, there will always be a longer
    // sleep for the first slice than the remainder.
    double	sliceTimes[NUM_SLICES_PER_SCREEN+1];

    static const double startBias = 0.0; // 8.0/1920.0/60.0;	// about 8 pixels into a 1920 screen at 60 hz
    static const double activeFraction = 112.0 / 135;			// the remainder are blanking lines
    for ( int i = 0; i <= NUM_SLICES_PER_SCREEN; i++ )
    {
        const double framePoint = vsyncBase + activeFraction * (float)i / NUM_SLICES_PER_SCREEN;
        sliceTimes[i] = ( vsyncState.vsyncBaseNano +
                          ( framePoint - vsyncState.vsyncCount ) * vsyncState.vsyncPeriodNano )
                        * 0.000000001 + startBias;
    }

    int screenWide, screenTall;
    m_screen.getScreenResolution( screenWide, screenTall );
    glViewport( 0, 0, screenWide, screenTall );
    glScissor( 0, 0, screenWide, screenTall );

    // This must be long enough to cover CPU scheduling delays, GPU in-flight commands,
    // and the actual drawing of this slice.
    const warpSource_t & latestWarpSource = m_warpSources[m_eyeBufferCount.state()%MAX_WARP_SOURCES];
    const double schedulingCushion = latestWarpSource.WarpParms.PreScheduleSeconds;

    //LOG( "now %fv(%i) %f cush %f", GetFractionalVsync(), (int)vsyncBase, ovr_GetTimeInSeconds(), schedulingCushion );

    // Warp each slice to the display surface
    warpSource_t currentWarpSource = {};
    int	back = 0;	// frame back from most recent
    long long thisEyeBufferNum = 0;
    for ( int screenSlice = 0; screenSlice < NUM_SLICES_PER_SCREEN; screenSlice++ )
    {
        const ScreenEye	eye = (ScreenEye)( screenSlice / NUM_SLICES_PER_EYE );

        // Sleep until we are in the correct part of the screen for
        // rendering this slice.
        const double sleepTargetTime = sliceTimes[ screenSlice ] - schedulingCushion;
        const float secondsToSleep = SleepUntilTimePoint( sleepTargetTime, false );
        const double preFinish = ovr_GetTimeInSeconds();

        //LOG( "slice %i targ %f slept %f", screenSlice, sleepTargetTime, secondsToSleep );

        // Check for availability of updated eye renderings now that we are about to render.
        if ( screenSlice == 0 )
        {
            const long long latestEyeBufferNum = m_eyeBufferCount.state();
            for ( back = 0; back < MAX_WARP_SOURCES - 1; back++ )
            {
                thisEyeBufferNum = latestEyeBufferNum - back;
                if ( thisEyeBufferNum <= 0 )
                {
                    LOG( "WarpToScreen: No valid Eye Buffers" );
                    // just starting, and we don't have any eye buffers to use
                    break;
                }

                warpSource_t & testWarpSource = m_warpSources[thisEyeBufferNum % MAX_WARP_SOURCES];
                if ( testWarpSource.MinimumVsync > vsyncBase )
                {
                    // a full frame got completed in less time than a single eye; don't use it to avoid stuttering
                    continue;
                }

                if ( testWarpSource.GpuSync == 0 )
                {
                    LOG( "thisEyeBufferNum %lli had 0 sync", thisEyeBufferNum );
                    break;
                }

                if ( Quatf( testWarpSource.WarpParms.Images[eye][0].Pose.Pose.Orientation ).LengthSq() < 1e-18f )
                {
                    LOG( "Bad Predicted.Pose.Orientation!" );
                    continue;
                }

                const EGLint wait = eglClientWaitSyncKHR_( m_eglDisplay, testWarpSource.GpuSync,
                                                           EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, 0 );
                if ( wait == EGL_TIMEOUT_EXPIRED_KHR )
                {
                    continue;
                }
                if ( wait == EGL_FALSE )
                {
                    LOG( "eglClientWaitSyncKHR returned EGL_FALSE" );
                }

                // This buffer set is good to use
                if ( testWarpSource.FirstDisplayedVsync[eye] == 0 )
                {
                    testWarpSource.FirstDisplayedVsync[eye] = (long long)vsyncBase;
                }
                currentWarpSource = testWarpSource;
                break;
            }

            // Release the VR thread if it is blocking on a frame being completed.
            // It is important that this always be done, even if we wind up
            // not rendering anything because there are no current eye buffers.
            if ( screenSlice == 0 )
            {
                SwapState	state;
                state.VsyncCount = (long long)vsyncBase;
                state.EyeBufferCount = thisEyeBufferNum;
                m_swapVsync.setState( state );

                // Wake the VR thread up if it is blocked on us.
                // If the other thread happened to be scheduled out right
                // after locking the mutex, but before waiting on the condition,
                // we would rather it sleep for another frame than potentially
                // miss a raster point here in the time warp thread, so use
                // a trylock() instead of a lock().
                if ( !pthread_mutex_trylock( &m_swapMutex ) )
                {
                    pthread_cond_signal( &m_swapIsLatched );
                    pthread_mutex_unlock( &m_swapMutex );
                }
            }

            if ( m_screen.windowSurface == EGL_NO_SURFACE )
            {
                static int logCount = 0;
                if ( ( logCount++ & 31 ) == 0 )
                {
                    LOG( "WarpToScreen: no valid window surface" );
                }
                return;
            }

            if ( currentWarpSource.WarpParms.Images[eye][0].TexId == 0 )
            {
                // We don't have anything valid to draw, so just sleep until
                // the next time point and check again.
                LOG( "WarpToScreen: Nothing valid to draw" );
                SleepUntilTimePoint( FramePointTimeInSeconds( vsyncBase + 1.0f ), false );
                break;
            }
        }

        // Build up the external velocity transform
        Matrix4f velocity;
        const int velocitySteps = NervGear::Alg::Min( 3, (int)((long long)vsyncBase - currentWarpSource.MinimumVsync) );
        for ( int i = 0; i < velocitySteps; i++ )
        {
            velocity = velocity * currentWarpSource.WarpParms.ExternalVelocity;
        }

        // If we have a second image layer, we will need to calculate
        // a second set of time warps and use a different program.
        const bool dualLayer = ( currentWarpSource.WarpParms.Images[eye][1].TexId > 0 );

        // Calculate predicted poses for the start and end of this eye's
        // raster scanout, so we can warp the best image we have to it.
        //
        // These prediction points will always be in the future, because we
        // need time to do the drawing before the scanout starts.
        //
        // In a portrait scanned display, it is beneficial to have the time warp calculated
        // independently for each eye, giving them the same latency profile.
        Matrix4f timeWarps[2][2];
        static ovrSensorState sensor[2];
        for ( int scan = 0; scan < 2; scan++ )
        {
            // We always make a new prediciton for the end of the slice,
            // but we only make a new one for the start of the slice when a
            // new eye has just started, otherwise we could get a visible
            // seam at the slice boundary when the prediction changed.
            static Matrix4f	warp;
            if ( scan == 1 || screenSlice == 0 || screenSlice == NUM_SLICES_PER_EYE )
            {
                // SliceTimes should be the actual time the pixels hit the screen,
                // but we may want a slight adjustment on the prediction time.
                const double timePoint = sliceTimes[screenSlice + scan];
                sensor[scan] = ovr_GetSensorStateInternal( timePoint );
                warp = CalculateTimeWarpMatrix2(
                        currentWarpSource.WarpParms.Images[eye][0].Pose.Pose.Orientation,
                        sensor[scan].Predicted.Pose.Orientation ) * velocity;
            }
            timeWarps[0][scan] = Matrix4f( currentWarpSource.WarpParms.Images[eye][0].TexCoordsFromTanAngles ) * warp;
            if ( dualLayer )
            {
                if ( currentWarpSource.WarpParms.WarpOptions & SWAP_OPTION_FIXED_OVERLAY )
                {	// locked-to-face HUD
                    timeWarps[1][scan] = Matrix4f( currentWarpSource.WarpParms.Images[eye][1].TexCoordsFromTanAngles );
                }
                else
                {	// locked-to-world surface
                    const Matrix4f warp2 = CalculateTimeWarpMatrix2(
                            currentWarpSource.WarpParms.Images[eye][1].Pose.Pose.Orientation,
                            sensor[scan].Predicted.Pose.Orientation ) * velocity;
                    timeWarps[1][scan] = Matrix4f( currentWarpSource.WarpParms.Images[eye][1].TexCoordsFromTanAngles ) * warp2;
                }
            }
        }

        // Update the SurfaceTexture before calculating VFrameSmooth
        if ( currentWarpSource.WarpParms.SurfaceTextureObject )
        {
            ovrPoseStatef pose[2];
            UpdateSurfaceTexture( m_jni,
                                  (jobject)currentWarpSource.WarpParms.SurfaceTextureObject,
                    m_updateTexImageMethodId, m_getTimestampMethodId, pose[0], pose[1] );
            // Both time warp matrix are from the time at the start of this eye.
            // The rollingWarp matrix will first transform the vector to account
            // for movement in the current eye display roll.
            for ( int scan = 0; scan < 2; scan++ )
            {
                const Matrix4f warp2 = CalculateTimeWarpMatrix2(
                        pose[scan].Pose.Orientation,
                        sensor[0].Predicted.Pose.Orientation
                );
                timeWarps[1][scan] = Matrix4f( currentWarpSource.WarpParms.Images[eye][1].TexCoordsFromTanAngles ) * warp2;
            }
        }

        // The pass through camera support needs to know the warping from the head motion
        // across the display scan independent of any layers, which may drop frames.
        const Matrix4f rollingWarp = CalculateTimeWarpMatrix2(
                sensor[0].Predicted.Pose.Orientation,
                sensor[1].Predicted.Pose.Orientation );

        //---------------------------------------------------------
        // Warp a latched buffer to the screen
        //---------------------------------------------------------

        m_logEyeWarpGpuTime.Begin( screenSlice );
        m_logEyeWarpGpuTime.PrintTime( screenSlice, "GPU time for eye time warp" );

        setWarpState( currentWarpSource );

        bindWarpProgram( currentWarpSource, timeWarps, rollingWarp, eye, vsyncBase );

        if ( screenSlice == 0 || screenSlice == NUM_SLICES_PER_EYE )
        {
            BindEyeTextures( currentWarpSource, eye );
        }

        const int sliceSize = screenWide / NUM_SLICES_PER_SCREEN;

        m_screen.beginDirectRendering( sliceSize*screenSlice, 0, sliceSize, screenTall );

        // Draw the warp triangles.
        const VGlGeometry & mesh = m_sliceMesh;
        glBindVertexArrayOES_( mesh.vertexArrayObject );
        const int indexCount = mesh.indexCount / NUM_SLICES_PER_SCREEN;
        const int indexOffset = screenSlice * indexCount;
        glDrawElements( GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, (void *)(indexOffset * 2 ) );

        if ( 0 )
        {	// solid color flashing test to see if slices are rendering in the right place
            const int cycleColor = (int)vsyncBase + screenSlice;
            glClearColor( cycleColor & 1, ( cycleColor >> 1 ) & 1, ( cycleColor >> 2 ) & 1, 1 );
            glClear( GL_COLOR_BUFFER_BIT );
        }

        // Draw the screen vignette, calibration grid, and debug graphs.
        // The grid will be based on the orientation that the warpSource
        // was rendered at, so you can see the amount of interpolated time warp applied
        // to it as a delta from the red grid to the greed or blue grid that was drawn directly
        // into the texture.
        drawFrameworkGraphicsToWindow( eye, currentWarpSource.WarpParms.WarpOptions,
                                       currentWarpSource.WarpParms.DebugGraphMode != DEBUG_PERF_OFF );

        m_screen.endDirectRendering();

        m_logEyeWarpGpuTime.End( screenSlice );

        const double justBeforeFinish = ovr_GetTimeInSeconds();
        if ( m_screen.isFrontBuffer() &&
             ( screenSlice == 1 * NUM_SLICES_PER_EYE - 1 || screenSlice == 2 * NUM_SLICES_PER_EYE - 1 ) )
        {
            GL_Finish();
        }
        const double postFinish = ovr_GetTimeInSeconds();

        const float latency = postFinish - justBeforeFinish;
        if ( latency > 0.008f )
        {
            LOG( "Frame %i Eye %i latency %5.3f", (int)vsyncBase, eye, latency );
        }

        if ( 0 )
        {
            LOG( "slice %i sleep %7.4f fin %6.4f buf %lli (%i back)",
                 screenSlice, secondsToSleep,
                 postFinish - preFinish,
                 thisEyeBufferNum, back );
        }

        // Update debug graph data
        if ( currentWarpSource.WarpParms.DebugGraphMode != DEBUG_PERF_FROZEN )
        {
            const int logIndex = (int)m_lastEyeLog & (EYE_LOG_COUNT-1);
            eyeLog_t & thisLog = m_eyeLog[logIndex];
            thisLog.skipped = false;
            thisLog.bufferNum = thisEyeBufferNum;
            thisLog.issueFinish = preFinish - sleepTargetTime;
            thisLog.completeFinish = postFinish - sleepTargetTime;
            m_lastEyeLog++;
        }
    }	// for screenSlice

    UnbindEyeTextures();

    glUseProgram( 0 );

    glBindVertexArrayOES_( 0 );

    if ( !m_screen.isFrontBuffer() )
    {
        m_screen.swapBuffers();
    }
}

static uint64_t GetNanoSecondsUint64()
{
    struct timespec now;
    clock_gettime( CLOCK_MONOTONIC, &now );
    return (uint64_t) now.tv_sec * 1000000000LL + now.tv_nsec;
}

void VFrameSmooth::warpSwapInternal( const ovrTimeWarpParms & parms )
{
#if defined( OVR_ENABLE_CAPTURE )
    OVR_CAPTURE_CPU_ZONE( WarpSwapInternal );
#endif

    if ( gettid() != m_sStartupTid )
    {
        FAIL( "WarpSwap: Called with tid %i instead of %i", gettid(), m_sStartupTid );
    }

    if ( m_screen.windowSurface == EGL_NO_SURFACE )
    {
        FAIL( "WarpSwap: no valid window surface" );
    }

    // Keep track of the last time WarpSwap() was called.
    m_lastWarpSwapTimeInSeconds.setState( ovr_GetTimeInSeconds() );

    // Explicitly bind the framebuffer back to the default, so the
    // eye targets will not be bound while they might be used as warp sources.
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );

    // If we are running the image server, let it add commands for resampling
    // the eye buffer to a transfer buffer.
#if defined( OVR_ENABLE_CAPTURE )
    if( Capture::IsConnected() )
{
    Capture::FrameBufferGLES3( parms.Images[0][0].TexId );
}
#endif
    const int minimumVsyncs = ( ovr_GetPowerLevelStateThrottled() ) ? 2 : parms.MinimumVsyncs;

    // Prepare to pass the new eye buffers to the background thread if we are running multi-threaded.
    const long long lastBufferCount = m_eyeBufferCount.state();
    warpSource_t & ws = m_warpSources[ ( lastBufferCount + 1 ) % MAX_WARP_SOURCES ];
    ws.MinimumVsync = m_lastSwapVsyncCount + 2 * minimumVsyncs;	// don't use it if from same frame to avoid problems with very fast frames
    ws.FirstDisplayedVsync[0] = 0;			// will be set when it becomes the currentSource
    ws.FirstDisplayedVsync[1] = 0;			// will be set when it becomes the currentSource
    ws.disableChromaticCorrection = ( ovr_GetPowerLevelStateThrottled() || ( EglGetGpuType() & NervGear::GPU_TYPE_MALI_T760_EXYNOS_5433 ) != 0 );
    ws.WarpParms = parms;

    // Default images.
    if ( ( parms.WarpOptions & SWAP_OPTION_DEFAULT_IMAGES ) != 0 )
    {
        for ( int eye = 0; eye < MAX_WARP_EYES; eye++ )
        {
            if ( parms.Images[eye][0].TexId == 0 )
            {
                ws.WarpParms.Images[eye][0].TexId = m_blackTexId;
            }
            if ( parms.Images[eye][1].TexId == 0 )
            {
                ws.WarpParms.Images[eye][1].TexId = m_defaultLoadingIconTexId;
            }
        }
    }

    // Destroy the sync object that was created for this buffer set.
    if ( ws.GpuSync != EGL_NO_SYNC_KHR )
    {
        if ( EGL_FALSE == eglDestroySyncKHR_( m_eglDisplay, ws.GpuSync ) )
        {
            LOG( "eglDestroySyncKHR returned EGL_FALSE" );
        }
    }

    // Add a sync object for this buffer set.
    ws.GpuSync = eglCreateSyncKHR_( m_eglDisplay, EGL_SYNC_FENCE_KHR, NULL );
    if ( ws.GpuSync == EGL_NO_SYNC_KHR )
    {
        FAIL( "eglCreateSyncKHR_():EGL_NO_SYNC_KHR" );
    }

    // Force it to flush the commands
    if ( EGL_FALSE == eglClientWaitSyncKHR_( m_eglDisplay, ws.GpuSync,
                                             EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, 0 ) )
    {
        LOG( "eglClientWaitSyncKHR returned EGL_FALSE" );
    }

    // Submit this buffer set for use by the VFrameSmooth thread
//	LOG( "submitting bufferNum %lli: %i %i", lastBufferCount+1,
//			ws.WarpParms.Images[0][0].TexId, ws.WarpParms.Images[1][0].TexId );
    m_eyeBufferCount.setState( lastBufferCount + 1 );

    // If we are running synchronously instead of using a background
    // thread, call WarpToScreen() directly.
    if ( !m_initParms.asynchronousTimeWarp )
    {
        // Make sure all eye drawing is completed before we warp the drawing
        // to the display buffer.
        GL_Finish();

        swapProgram_t * swapProg;
        if ( m_screen.isFrontBuffer() )
        {
            swapProg = &spSyncFrontBufferPortrait;
        }
        else
        {
            swapProg = &spSyncSwappedBufferPortrait;
        }

        warpToScreen( floor( GetFractionalVsync() ), *swapProg );

        const SwapState state = m_swapVsync.state();
        m_lastSwapVsyncCount = state.VsyncCount;

        return;
    }

    for ( ; ; )
    {
        const uint64_t startSuspendNanoSeconds = GetNanoSecondsUint64();

        // block until the next vsync
        pthread_mutex_lock( &m_swapMutex );

        // Atomically unlock the mutex and block until the warp thread
        // has completed a warp and sampled the sensors, updating SwapVsync.
        pthread_cond_wait( &m_swapIsLatched, &m_swapMutex );

        // Pthread_cond_wait re-locks the mutex before exit.
        pthread_mutex_unlock( &m_swapMutex );

        const uint64_t endSuspendNanoSeconds = GetNanoSecondsUint64();

        const SwapState state = m_swapVsync.state();
        if ( state.EyeBufferCount >= lastBufferCount )
        {
            // If MinimumVsyncs was increased dynamically, it is necessary
            // to skip one or more vsyncs just as the change happens.
            m_lastSwapVsyncCount = Alg::Max( state.VsyncCount, m_lastSwapVsyncCount + minimumVsyncs );

            // Sleep for at least one millisecond to make sure the main VR thread
            // cannot completely deny the Android watchdog from getting a time slice.
            const uint64_t suspendNanoSeconds = endSuspendNanoSeconds - startSuspendNanoSeconds;
            if ( suspendNanoSeconds < 1000 * 1000 )
            {
                const uint64_t suspendMicroSeconds = ( 1000 * 1000 - suspendNanoSeconds ) / 1000;
                LOG( "WarpSwap: usleep( %lld )", suspendMicroSeconds );
                usleep( suspendMicroSeconds );
            }
            return;
        }
    }
}

/*
* Called by the application thread to have a pair eye buffers
* time warped to the screen.
*
* Blocks until it is time to render a new pair of eye buffers.
*
*/
void VFrameSmooth::warpSwap( const ovrTimeWarpParms & parms )
{
    const int count = ( ( parms.WarpOptions & SWAP_OPTION_FLUSH ) != 0 ) ? 3 : 1;
    for ( int i = 0; i < count; i++ )
    {
        warpSwapInternal( parms );
    }
}

/*
* VisualizeTiming
*
* Draw graphs of the latency and frame drops.
*/
struct lineVert_t
{
    unsigned short	x, y;
    unsigned int	color;
};

int ColorAsInt( const int r, const int g, const int b, const int a )
{
    return r | (g<<8) | (b<<16) | (a<<24);
}

VGlGeometry CreateTimingGraphGeometry( const int lineVertCount )
{
    VGlGeometry geo;

    glGenVertexArraysOES_( 1, &geo.vertexArrayObject );
    glBindVertexArrayOES_( geo.vertexArrayObject );

    lineVert_t	* verts = new lineVert_t[lineVertCount];
    const int byteCount = lineVertCount * sizeof( verts[0] );
    memset( verts, 0, byteCount );

    glGenBuffers( 1, &geo.vertexBuffer );
    glBindBuffer( GL_ARRAY_BUFFER, geo.vertexBuffer );
    glBufferData( GL_ARRAY_BUFFER, byteCount, (void *) verts, GL_DYNAMIC_DRAW );
    glEnableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_POSITION );
    glVertexAttribPointer( SHADER_ATTRIBUTE_LOCATION_POSITION, 2, GL_SHORT, false, sizeof( lineVert_t ), (void *)0 );

    glEnableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_COLOR );
    glVertexAttribPointer( SHADER_ATTRIBUTE_LOCATION_COLOR, 4, GL_UNSIGNED_BYTE, true, sizeof( lineVert_t ), (void *)4 );
    delete[] verts;

    // these will be drawn with DrawArrays, so no index buffer is needed

    geo.indexCount = lineVertCount;

    glBindVertexArrayOES_( 0 );

    return geo;
}

void VFrameSmooth::updateTimingGraphVerts( const ovrTimeWarpDebugPerfMode debugPerfMode, const ovrTimeWarpDebugPerfValue debugValue )
{
    if ( debugPerfMode == DEBUG_PERF_OFF )
    {
        return;
    }

    // Draw graph markers every five milliseconds
    lineVert_t	verts[EYE_LOG_COUNT*2+10];
    int			numVerts = 0;
    const int	colorRed = ColorAsInt( 255, 0, 0, 255 );
    const int	colorGreen = ColorAsInt( 0, 255, 0, 255 );
    // const int	colorYellow = ColorAsInt( 255, 255, 0, 255 );
    const int	colorWhite = ColorAsInt( 255, 255, 255, 255 );

    const float base = 250;
    const int drawLogs = 256; // VSYNC_LOG_COUNT;
    for ( int i = 0; i < drawLogs; i++ )
    {
        const int logIndex = ( m_lastEyeLog - 1 - i ) & ( EYE_LOG_COUNT - 1 );
        const eyeLog_t & log = m_eyeLog[ logIndex ];
        const int y = i*4;
        if ( log.skipped )
        {
            verts[i*2+0].y = verts[i*2+1].y = y;
            verts[i*2+0].x = base;
            verts[i*2+1].x = base + 150;
            verts[i*2+0].color = verts[i*2+1].color =  colorRed;
        }
        else
        {
            if ( debugValue == DEBUG_VALUE_LATENCY )
            {
                verts[i*2+0].y = verts[i*2+1].y = y;
                verts[i*2+0].x = base;
                verts[i*2+1].x = base + 10000 * log.poseLatencySeconds;
                verts[i*2+0].color = verts[i*2+1].color =  colorGreen;
            }
            else
            {
                verts[i*2+0].y = verts[i*2+1].y = y;
                verts[i*2+0].x = base + 10000 * log.issueFinish;
                verts[i*2+1].x = base + 10000 * log.completeFinish;
                if ( m_eyeLog[ ( logIndex - 2 ) & ( EYE_LOG_COUNT-1) ].bufferNum != log.bufferNum )
                {	// green = fresh buffer
                    verts[i*2+0].color = verts[i*2+1].color =  colorGreen;
                }
                else
                {	// red = stale buffer reprojected
                    verts[i*2+0].color = verts[i*2+1].color =  colorRed;
                }
            }
        }
    }
    numVerts = drawLogs*2;

    // markers every 5 milliseconds
    if ( debugValue == DEBUG_VALUE_LATENCY )
    {
        for ( int t = 0; t <= 64; t += 16 )
        {
            const float dt = 0.001f * t;
            const int x = base + 10000 * dt;

            verts[numVerts+0].y = 0;
            verts[numVerts+1].y = drawLogs * 4;
            verts[numVerts+0].x = x;
            verts[numVerts+1].x = x;
            verts[numVerts+0].color = verts[numVerts+1].color = colorWhite;
            numVerts += 2;
        }
    }
    else
    {
        for ( int t = 0; t <= 1; t++ )
        {
            const float dt = 1.0f/120.0f * t;
            const int x = base + 10000 * dt;

            verts[numVerts+0].y = 0;
            verts[numVerts+1].y = drawLogs * 4;
            verts[numVerts+0].x = x;
            verts[numVerts+1].x = x;
            verts[numVerts+0].color = verts[numVerts+1].color = colorWhite;
            numVerts += 2;
        }
    }

    // Update the vertex buffer.
    // We are updating buffer objects inside a vertex array object instead
    // of using client arrays to avoid messing with Unity's attribute arrays.

    // NOTE: vertex array objects do NOT include the GL_ARRAY_BUFFER_BINDING state, and
    // binding a VAO does not change GL_ARRAY_BUFFER, so we do need to track the buffer
    // in the geometry if we want to update it, or do a GetVertexAttrib( SHADER_ATTRIB_ARRAY_BUFFER_BINDING

    // For reasons that I do not understand, if I don't bind the VAO, then all updates after the
    // first one produce no additional changes.
    glBindVertexArrayOES_( m_timingGraph.vertexArrayObject );
    glBindBuffer( GL_ARRAY_BUFFER, m_timingGraph.vertexBuffer );
    glBufferSubData( GL_ARRAY_BUFFER, 0, numVerts * sizeof( verts[0] ), (void *) verts );
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindVertexArrayOES_( 0 );

    m_timingGraph.indexCount = numVerts;

    GL_CheckErrors( "After UpdateTimingGraph" );
}

void VFrameSmooth::drawTimingGraph( const ScreenEye eye )
{
    // Use the same rect for viewport and scissor
    // FIXME: this is probably bad for convergence
    int	rectX, rectY, rectWidth, rectHeight;
    EyeRect( m_initParms.hmdInfo, eye, RECT_SCREEN,
             rectX, rectY, rectWidth, rectHeight );

    glViewport( rectX, rectY, rectWidth, rectHeight );
    glScissor( rectX, rectY, rectWidth, rectHeight );

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_BLEND );
    glLineWidth( 2.0f );
    glUseProgram( m_debugLineProgram.program );

    // pixel identity matrix
    // Map the rectWidth / rectHeight dimensions to -1 - 1 range
    float scale_x = 2.0f / (float)rectWidth;
    float scale_y = 2.0f / (float)rectHeight;
    const Matrix4f landscapePixelMatrix(
            0, scale_x, 0.0f, -1.0f,
            scale_y, 0, 0.0f, -1.0f,
            0.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f );


    glUniformMatrix4fv( m_debugLineProgram.uniformModelViewProMatrix, 1, GL_FALSE, /* not transposed */
                        landscapePixelMatrix.Transposed().M[0] );

    glBindVertexArrayOES_( m_timingGraph.vertexArrayObject );
    glDrawArrays( GL_LINES, 0, m_timingGraph.indexCount );
    glBindVertexArrayOES_( 0 );

    glViewport( 0, 0, rectWidth * 2, rectHeight );
    glScissor( 0, 0, rectWidth * 2, rectHeight );

    GL_CheckErrors( "DrawTimingGraph" );
}

float calibrateFovScale = 1.0f;	// for interactive tweaking

// VAO (and FBO) are not shared across different contexts, so this needs
// to be called by the thread that will be drawing the warp.
void VFrameSmooth::createFrameworkGraphics()
{
    // Texture to display a black screen.
    unsigned char blackData[4] = {};
    glGenTextures( 1, &m_blackTexId );
    glBindTexture( GL_TEXTURE_2D, m_blackTexId );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, blackData );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glBindTexture( GL_TEXTURE_2D, 0 );

    // Default loading icon.
    glGenTextures( 1, &m_defaultLoadingIconTexId );
    glBindTexture( GL_TEXTURE_2D, m_defaultLoadingIconTexId );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, oculus_loading_indicator_width, oculus_loading_indicator_height,
                  0, GL_RGBA, GL_UNSIGNED_BYTE, oculus_loading_indicator_bufferData );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glBindTexture( GL_TEXTURE_2D, 0 );

    // Decide where we get our distortion mesh from
    MemBuffer buf;
    if ( m_initParms.distortionFileName )
    {	// If we have an explicit distortion file request, use that.
        MemBufferFile explicitFile( m_initParms.distortionFileName );
        if ( explicitFile.length > 0 )
        {
            buf = explicitFile.toMemBuffer();
        }
        if ( buf.buffer == NULL )
        {
            FAIL( "Distortion file '%s' failed to load", m_initParms.distortionFileName );
        }
    }
    else
    {	// Look for the default file
        VString fullPath = m_initParms.externalStorageDirectory + DefaultDistortionFile;
        LOG( "Loading distortion file: %s", fullPath.toCString() );
        MemBufferFile defaultFile( fullPath.toCString() );
        if ( defaultFile.length > 0 )
        {
            buf = defaultFile.toMemBuffer();
        }

        if ( buf.buffer == NULL )
        {	// Synthesize the standard distortion
            buf = BuildDistortionBuffer( m_initParms.hmdInfo, 32, 32 );
        }
    }

    // single slice mesh for the normal rendering
    m_warpMesh = VGlGeometryFactory::LoadMeshFromMemory( buf, 1, calibrateFovScale, false );

    // multi-slice mesh for sliced rendering
    m_sliceMesh = VGlGeometryFactory::LoadMeshFromMemory( buf, NUM_SLICES_PER_EYE, calibrateFovScale, false );

    // small subset cursor mesh
    m_cursorMesh = VGlGeometryFactory::LoadMeshFromMemory( buf, 1, calibrateFovScale, true );

    if ( m_warpMesh.indexCount == 0 || m_sliceMesh.indexCount == 0 )
    {
        FAIL( "WarpMesh failed to load");
    }

    buf.freeData();

    // Vertexes and indexes for debug graph, the verts will be updated
    // dynamically each frame.
    m_timingGraph = CreateTimingGraphGeometry( (256+10)*2 );

    // simple cross to draw to screen
    m_calibrationLines2 = VGlGeometryFactory::CreateCalibrationLines2( 0, false );

    // FPS and graph text
    m_untexturedMvpProgram.initShader(
            "uniform mat4 Mvpm;\n"
                    "attribute vec4 Position;\n"
                    "uniform mediump vec4 UniformColor;\n"
                    "varying  lowp vec4 oColor;\n"
                    "void main()\n"
                    "{\n"
                    "   gl_Position = Mvpm * Position;\n"
                    "   oColor = UniformColor;\n"
                    "}\n"
            ,
            "varying lowp vec4	oColor;\n"
                    "void main()\n"
                    "{\n"
                    "	gl_FragColor = oColor;\n"
                    "}\n"
    );

    m_debugLineProgram.initShader(
            "uniform mediump mat4 Mvpm;\n"
                    "attribute vec4 Position;\n"
                    "attribute vec4 VertexColor;\n"
                    "varying  vec4 oColor;\n"
                    "void main()\n"
                    "{\n"
                    "   gl_Position = Mvpm * Position;\n"
                    "   oColor = VertexColor;\n"
                    "}\n"
            ,
            "varying lowp vec4 oColor;\n"
                    "void main()\n"
                    "{\n"
                    "	gl_FragColor = oColor;\n"
                    "}\n"
    );

    // Build our warp render programs
    buildWarpProgs();
}

void VFrameSmooth::destroyFrameworkGraphics()
{
    glDeleteTextures( 1, &m_blackTexId );
    glDeleteTextures( 1, &m_defaultLoadingIconTexId );

    m_calibrationLines2.Free();
    m_warpMesh.Free();
    m_sliceMesh.Free();
    m_cursorMesh.Free();
    m_timingGraph.Free();

    m_untexturedMvpProgram.destroy();
    m_debugLineProgram.destroy();

    for ( int i = 0; i < WP_PROGRAM_MAX; i++ )
    {
        m_warpPrograms[i].destroy();
    }
}

// Assumes viewport and scissor is set for the eye already.
// Assumes there is no depth buffer for the window.
void VFrameSmooth::drawFrameworkGraphicsToWindow( const ScreenEye eye,
                                                   const int swapOptions, const bool drawTimingGraph )
{
    // Latency tester support.
    unsigned char latencyTesterColorToDisplay[3];

    if ( ovr_ProcessLatencyTest( latencyTesterColorToDisplay ) )
    {
        glClearColor(
                latencyTesterColorToDisplay[0] / 255.0f,
                latencyTesterColorToDisplay[1] / 255.0f,
                latencyTesterColorToDisplay[2] / 255.0f,
                1.0f );
        glClear( GL_COLOR_BUFFER_BIT );
    }

    // Report latency tester results to the log.
    const char * results = ovr_GetLatencyTestResult();
    if ( results != NULL )
    {
        LOG( "LATENCY TESTER: %s", results );
    }

    // optionally draw the calibration lines
    if ( swapOptions & SWAP_OPTION_DRAW_CALIBRATION_LINES )
    {
        const float znear = 0.5f;
        const float zfar = 150.0f;
        // flipped for portrait mode
        const Matrix4f projectionMatrix(
                0, 1, 0, 0,
                -1, 0, 0, 0,
                0, 0, zfar / (znear - zfar), (zfar * znear) / (znear - zfar),
                0, 0, -1, 0 );
        glUseProgram( m_untexturedMvpProgram.program );
        glLineWidth( 2.0f );
        glUniform4f( m_untexturedMvpProgram.uniformColor, 1, 0, 0, 1 );
        glUniformMatrix4fv( m_untexturedMvpProgram.uniformModelViewProMatrix, 1, GL_FALSE,  // not transposed
                            projectionMatrix.Transposed().M[0] );
        glBindVertexArrayOES_( m_calibrationLines2.vertexArrayObject );

        int width, height;
        m_screen.getScreenResolution( width, height );
        glViewport( width/2 * (int)eye, 0, width/2, height );
        glDrawElements( GL_LINES, m_calibrationLines2.indexCount, GL_UNSIGNED_SHORT, NULL );
        glViewport( 0, 0, width, height );
    }

    // Draw the timing graph if it is enabled.
    if ( drawTimingGraph )
    {
        this->drawTimingGraph( eye );
    }
}

void VFrameSmooth::buildWarpProgPair( ovrTimeWarpProgram simpleIndex,
                                       const char * simpleVertex, const char * simpleFragment,
                                       const char * chromaticVertex, const char * chromaticFragment
)
{
    m_warpPrograms[ simpleIndex ] = VGlShader( simpleVertex, simpleFragment );
    m_warpPrograms[ simpleIndex + ( WP_CHROMATIC - WP_SIMPLE ) ] = VGlShader( chromaticVertex, chromaticFragment );
}

void VFrameSmooth::buildWarpProgMatchedPair( ovrTimeWarpProgram simpleIndex,
                                              const char * simpleVertex, const char * simpleFragment
)
{
    m_warpPrograms[ simpleIndex ] = VGlShader( simpleVertex, simpleFragment );
    m_warpPrograms[ simpleIndex + ( WP_CHROMATIC - WP_SIMPLE ) ] = VGlShader( simpleVertex, simpleFragment );
}


void VFrameSmooth::buildWarpProgs()
{
    buildWarpProgPair( WP_SIMPLE,
            // low quality
                       "uniform mediump mat4 Mvpm;\n"
                               "uniform mediump mat4 Texm;\n"
                               "uniform mediump mat4 Texm2;\n"

                               "attribute vec4 Position;\n"
                               "attribute vec2 TexCoord;\n"
                               "attribute vec2 TexCoord1;\n"
                               "varying  vec2 oTexCoord;\n"
                               "void main()\n"
                               "{\n"
                               "   gl_Position = Mvpm * Position;\n"
                               "   vec3 left = vec3( Texm * vec4(TexCoord,-1,1) );\n"
                               "   vec3 right = vec3( Texm2 * vec4(TexCoord,-1,1) );\n"
                               "   vec3 proj = mix( left, right, TexCoord1.x );\n"
                               "	float projIZ = 1.0 / max( proj.z, 0.00001 );\n"
                               "	oTexCoord = vec2( proj.x * projIZ, proj.y * projIZ );\n"
                               "}\n"
            ,
                       "uniform sampler2D Texture0;\n"
                               "varying highp vec2 oTexCoord;\n"
                               "void main()\n"
                               "{\n"
                               "	gl_FragColor = texture2D(Texture0, oTexCoord);\n"
                               "}\n"
            ,
            // high quality
                       "uniform mediump mat4 Mvpm;\n"
                               "uniform mediump mat4 Texm;\n"
                               "uniform mediump mat4 Texm2;\n"
                               "uniform mediump mat4 Texm3;\n"
                               "uniform mediump mat4 Texm4;\n"

                               "attribute vec4 Position;\n"
                               "attribute vec2 TexCoord;\n"	// green
                               "attribute vec2 TexCoord1;\n"	// .x = interpolated warp frac, .y = intensity scale
                               "attribute vec2 Normal;\n"		// red
                               "attribute vec2 Tangent;\n"		// blue
                               "varying  vec2 oTexCoord1r;\n"
                               "varying  vec2 oTexCoord1g;\n"
                               "varying  vec2 oTexCoord1b;\n"
                               "void main()\n"
                               "{\n"
                               "   gl_Position = Mvpm * Position;\n"
                               "	vec3 proj;\n"
                               "	float projIZ;\n"
                               ""
                               "   proj = mix( vec3( Texm * vec4(Normal,-1,1) ), vec3( Texm2 * vec4(Normal,-1,1) ), TexCoord1.x );\n"
                               "	projIZ = 1.0 / max( proj.z, 0.00001 );\n"
                               "	oTexCoord1r = vec2( proj.x * projIZ, proj.y * projIZ );\n"
                               ""
                               "   proj = mix( vec3( Texm * vec4(TexCoord,-1,1) ), vec3( Texm2 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                               "	projIZ = 1.0 / max( proj.z, 0.00001 );\n"
                               "	oTexCoord1g = vec2( proj.x * projIZ, proj.y * projIZ );\n"
                               ""
                               "   proj = mix( vec3( Texm * vec4(Tangent,-1,1) ), vec3( Texm2 * vec4(Tangent,-1,1) ), TexCoord1.x );\n"
                               "	projIZ = 1.0 / max( proj.z, 0.00001 );\n"
                               "	oTexCoord1b = vec2( proj.x * projIZ, proj.y * projIZ );\n"
                               ""
                               "}\n"
            ,
                       "uniform sampler2D Texture0;\n"
                               "varying highp vec2 oTexCoord1r;\n"
                               "varying highp vec2 oTexCoord1g;\n"
                               "varying highp vec2 oTexCoord1b;\n"
                               "void main()\n"
                               "{\n"
                               "	lowp vec4 color1r = texture2D(Texture0, oTexCoord1r);\n"
                               "	lowp vec4 color1g = texture2D(Texture0, oTexCoord1g);\n"
                               "	lowp vec4 color1b = texture2D(Texture0, oTexCoord1b);\n"
                               "	lowp vec4 color1 = vec4( color1r.x, color1g.y, color1b.z, 1.0 );\n"
                               "	gl_FragColor = color1;\n"
                               "}\n"
    );

    buildWarpProgPair( WP_MASKED_PLANE,
            // low quality
                       "uniform mediump mat4 Mvpm;\n"
                               "uniform mediump mat4 Texm;\n"
                               "uniform mediump mat4 Texm2;\n"
                               "uniform mediump mat4 Texm3;\n"
                               "uniform mediump mat4 Texm4;\n"

                               "attribute vec4 Position;\n"
                               "attribute vec2 TexCoord;\n"
                               "attribute vec2 TexCoord1;\n"
                               "varying  vec2 oTexCoord;\n"
                               "varying  vec3 oTexCoord2;\n"	// Must do the proj in fragment shader or you get wiggles when you view the plane at even modest angles.
                               "void main()\n"
                               "{\n"
                               "   gl_Position = Mvpm * Position;\n"
                               "	vec3 proj;\n"
                               "	float projIZ;\n"
                               ""
                               "   proj = mix( vec3( Texm * vec4(TexCoord,-1,1) ), vec3( Texm2 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                               "	projIZ = 1.0 / max( proj.z, 0.00001 );\n"
                               "	oTexCoord = vec2( proj.x * projIZ, proj.y * projIZ );\n"
                               ""
                               "   oTexCoord2 = mix( vec3( Texm3 * vec4(TexCoord,-1,1) ), vec3( Texm4 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                               ""
                               "}\n"
            ,
                       "uniform sampler2D Texture0;\n"
                               "uniform sampler2D Texture1;\n"
                               "varying highp vec2 oTexCoord;\n"
                               "varying highp vec3 oTexCoord2;\n"
                               "void main()\n"
                               "{\n"
                               "	lowp vec4 color0 = texture2D(Texture0, oTexCoord);\n"
                               "	{\n"
                               "		lowp vec4 color1 = vec4( texture2DProj(Texture1, oTexCoord2).xyz, 1.0 );\n"
                               "		gl_FragColor = mix( color1, color0, color0.w );\n"	// pass through destination alpha
                               "	}\n"
                               "}\n"
            ,
            // high quality
                       "uniform mediump mat4 Mvpm;\n"
                               "uniform mediump mat4 Texm;\n"
                               "uniform mediump mat4 Texm2;\n"
                               "uniform mediump mat4 Texm3;\n"
                               "uniform mediump mat4 Texm4;\n"

                               "attribute vec4 Position;\n"
                               "attribute vec2 TexCoord;\n"	// green
                               "attribute vec2 TexCoord1;\n"
                               "attribute vec2 Normal;\n"		// red
                               "attribute vec2 Tangent;\n"		// blue
                               "varying  vec2 oTexCoord;\n"
                               "varying  vec3 oTexCoord2r;\n"	// These must do the proj in fragment shader or you
                               "varying  vec3 oTexCoord2g;\n"	// get wiggles when you view the plane at even
                               "varying  vec3 oTexCoord2b;\n"	// modest angles.
                               "void main()\n"
                               "{\n"
                               "   gl_Position = Mvpm * Position;\n"
                               "	vec3 proj;\n"
                               "	float projIZ;\n"
                               ""
                               "   proj = mix( vec3( Texm * vec4(TexCoord,-1,1) ), vec3( Texm2 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                               "	projIZ = 1.0 / max( proj.z, 0.00001 );\n"
                               "	oTexCoord = vec2( proj.x * projIZ, proj.y * projIZ );\n"
                               ""
                               "   oTexCoord2r = mix( vec3( Texm3 * vec4(Normal,-1,1) ), vec3( Texm4 * vec4(Normal,-1,1) ), TexCoord1.x );\n"
                               "   oTexCoord2g = mix( vec3( Texm3 * vec4(TexCoord,-1,1) ), vec3( Texm4 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                               "   oTexCoord2b = mix( vec3( Texm3 * vec4(Tangent,-1,1) ), vec3( Texm4 * vec4(Tangent,-1,1) ), TexCoord1.x );\n"
                               ""
                               "}\n"
            ,
                       "uniform sampler2D Texture0;\n"
                               "uniform sampler2D Texture1;\n"
                               "varying highp vec2 oTexCoord;\n"
                               "varying highp vec3 oTexCoord2r;\n"
                               "varying highp vec3 oTexCoord2g;\n"
                               "varying highp vec3 oTexCoord2b;\n"
                               "void main()\n"
                               "{\n"
                               "	lowp vec4 color0 = texture2D(Texture0, oTexCoord);\n"
                               "	{\n"
                               "		lowp vec4 color1r = texture2DProj(Texture1, oTexCoord2r);\n"
                               "		lowp vec4 color1g = texture2DProj(Texture1, oTexCoord2g);\n"
                               "		lowp vec4 color1b = texture2DProj(Texture1, oTexCoord2b);\n"
                               "		lowp vec4 color1 = vec4( color1r.x, color1g.y, color1b.z, 1.0 );\n"
                               "		gl_FragColor = mix( color1, color0, color0.w );\n"	// pass through destination alpha
                               "	}\n"
                               "}\n"
    );
    buildWarpProgPair( WP_MASKED_PLANE_EXTERNAL,
            // low quality
                       "uniform mediump mat4 Mvpm;\n"
                               "uniform mediump mat4 Texm;\n"
                               "uniform mediump mat4 Texm2;\n"
                               "uniform mediump mat4 Texm3;\n"
                               "uniform mediump mat4 Texm4;\n"

                               "attribute vec4 Position;\n"
                               "attribute vec2 TexCoord;\n"
                               "attribute vec2 TexCoord1;\n"
                               "varying  vec2 oTexCoord;\n"
                               "varying  vec3 oTexCoord2;\n"	// Must do the proj in fragment shader or you get wiggles when you view the plane at even modest angles.
                               "void main()\n"
                               "{\n"
                               "   gl_Position = Mvpm * Position;\n"
                               "	vec3 proj;\n"
                               "	float projIZ;\n"
                               ""
                               "   proj = mix( vec3( Texm * vec4(TexCoord,-1,1) ), vec3( Texm2 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                               "	projIZ = 1.0 / max( proj.z, 0.00001 );\n"
                               "	oTexCoord = vec2( proj.x * projIZ, proj.y * projIZ );\n"
                               ""
                               "   oTexCoord2 = mix( vec3( Texm3 * vec4(TexCoord,-1,1) ), vec3( Texm4 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                               ""
                               "}\n"
            ,
                       "#extension GL_OES_EGL_image_external : require\n"
                               "uniform sampler2D Texture0;\n"
                               "uniform samplerExternalOES Texture1;\n"
                               "varying highp vec2 oTexCoord;\n"
                               "varying highp vec3 oTexCoord2;\n"
                               "void main()\n"
                               "{\n"
                               "	lowp vec4 color0 = texture2D(Texture0, oTexCoord);\n"
                               "	{\n"
                               "		lowp vec4 color1 = vec4( texture2DProj(Texture1, oTexCoord2).xyz, 1.0 );\n"
                               "		gl_FragColor = mix( color1, color0, color0.w );\n"	// pass through destination alpha
                               "	}\n"
                               "}\n"
            ,
            // high quality
                       "uniform mediump mat4 Mvpm;\n"
                               "uniform mediump mat4 Texm;\n"
                               "uniform mediump mat4 Texm2;\n"
                               "uniform mediump mat4 Texm3;\n"
                               "uniform mediump mat4 Texm4;\n"

                               "attribute vec4 Position;\n"
                               "attribute vec2 TexCoord;\n"	// green
                               "attribute vec2 TexCoord1;\n"
                               "attribute vec2 Normal;\n"		// red
                               "attribute vec2 Tangent;\n"		// blue
                               "varying  vec2 oTexCoord;\n"
                               "varying  vec3 oTexCoord2r;\n"	// These must do the proj in fragment shader or you
                               "varying  vec3 oTexCoord2g;\n"	// get wiggles when you view the plane at even
                               "varying  vec3 oTexCoord2b;\n"	// modest angles.
                               "void main()\n"
                               "{\n"
                               "   gl_Position = Mvpm * Position;\n"
                               "	vec3 proj;\n"
                               "	float projIZ;\n"
                               ""
                               "   proj = mix( vec3( Texm * vec4(TexCoord,-1,1) ), vec3( Texm2 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                               "	projIZ = 1.0 / max( proj.z, 0.00001 );\n"
                               "	oTexCoord = vec2( proj.x * projIZ, proj.y * projIZ );\n"
                               ""
                               "   oTexCoord2r = mix( vec3( Texm3 * vec4(Normal,-1,1) ), vec3( Texm4 * vec4(Normal,-1,1) ), TexCoord1.x );\n"
                               "   oTexCoord2g = mix( vec3( Texm3 * vec4(TexCoord,-1,1) ), vec3( Texm4 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                               "   oTexCoord2b = mix( vec3( Texm3 * vec4(Tangent,-1,1) ), vec3( Texm4 * vec4(Tangent,-1,1) ), TexCoord1.x );\n"
                               ""
                               "}\n"
            ,
                       "#extension GL_OES_EGL_image_external : require\n"
                               "uniform sampler2D Texture0;\n"
                               "uniform samplerExternalOES Texture1;\n"
                               "varying highp vec2 oTexCoord;\n"
                               "varying highp vec3 oTexCoord2r;\n"
                               "varying highp vec3 oTexCoord2g;\n"
                               "varying highp vec3 oTexCoord2b;\n"
                               "void main()\n"
                               "{\n"
                               "	lowp vec4 color0 = texture2D(Texture0, oTexCoord);\n"
                               "	{\n"
                               "		lowp vec4 color1r = texture2DProj(Texture1, oTexCoord2r);\n"
                               "		lowp vec4 color1g = texture2DProj(Texture1, oTexCoord2g);\n"
                               "		lowp vec4 color1b = texture2DProj(Texture1, oTexCoord2b);\n"
                               "		lowp vec4 color1 = vec4( color1r.x, color1g.y, color1b.z, 1.0 );\n"
                               "		gl_FragColor = mix( color1, color0, color0.w );\n"	// pass through destination alpha
                               "	}\n"
                               "}\n"
    );
    buildWarpProgPair( WP_MASKED_CUBE,
            // low quality
                       "uniform mediump mat4 Mvpm;\n"
                               "uniform mediump mat4 Texm;\n"
                               "uniform mediump mat4 Texm2;\n"
                               "uniform mediump mat4 Texm3;\n"
                               "uniform mediump mat4 Texm4;\n"
                               "uniform mediump vec2 FrameNum;\n"

                               "attribute vec4 Position;\n"
                               "attribute vec2 TexCoord;\n"
                               "attribute vec2 TexCoord1;\n"
                               "varying  vec2 oTexCoord;\n"
                               "varying  vec3 oTexCoord2;\n"
                               "void main()\n"
                               "{\n"
                               "   gl_Position = Mvpm * Position;\n"
                               "	vec3 proj;\n"
                               "	float projIZ;\n"
                               ""
                               "   proj = mix( vec3( Texm * vec4(TexCoord,-1,1) ), vec3( Texm2 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                               "	projIZ = 1.0 / max( proj.z, 0.00001 );\n"
                               "	oTexCoord = vec2( proj.x * projIZ, proj.y * projIZ );\n"
                               ""
                               "   oTexCoord2 = mix( vec3( Texm3 * vec4(TexCoord,-1,1) ), vec3( Texm4 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                               ""
                               "}\n"
            ,
                       "uniform sampler2D Texture0;\n"
                               "uniform samplerCube Texture1;\n"
                               "uniform lowp float UniformColor;\n"
                               "varying highp vec2 oTexCoord;\n"
                               "varying highp vec3 oTexCoord2;\n"
                               "void main()\n"
                               "{\n"
                               "	lowp vec4 color0 = texture2D(Texture0, oTexCoord);\n"
                               "	lowp vec4 color1 = textureCube(Texture1, oTexCoord2) * UniformColor;\n"
                               "	gl_FragColor = vec4( mix( color1.xyz, color0.xyz, color0.w ), 1.0);\n"	// pass through destination alpha
                               "}\n"
            ,
            // high quality
                       "uniform mediump mat4 Mvpm;\n"
                               "uniform mediump mat4 Texm;\n"
                               "uniform mediump mat4 Texm2;\n"
                               "uniform mediump mat4 Texm3;\n"
                               "uniform mediump mat4 Texm4;\n"
                               "uniform mediump vec2 FrameNum;\n"

                               "attribute vec4 Position;\n"
                               "attribute vec2 TexCoord;\n"	// green
                               "attribute vec2 TexCoord1;\n"
                               "attribute vec2 Normal;\n"		// red
                               "attribute vec2 Tangent;\n"		// blue
                               "varying  vec2 oTexCoord;\n"
                               "varying  vec3 oTexCoord2r;\n"
                               "varying  vec3 oTexCoord2g;\n"
                               "varying  vec3 oTexCoord2b;\n"
                               "void main()\n"
                               "{\n"
                               "   gl_Position = Mvpm * Position;\n"
                               "	vec3 proj;\n"
                               "	float projIZ;\n"
                               ""
                               "   proj = mix( vec3( Texm * vec4(TexCoord,-1,1) ), vec3( Texm2 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                               "	projIZ = 1.0 / max( proj.z, 0.00001 );\n"
                               "	oTexCoord = vec2( proj.x * projIZ, proj.y * projIZ );\n"
                               ""
                               "   oTexCoord2r = mix( vec3( Texm3 * vec4(Normal,-1,1) ), vec3( Texm4 * vec4(Normal,-1,1) ), TexCoord1.x );\n"
                               "   oTexCoord2g = mix( vec3( Texm3 * vec4(TexCoord,-1,1) ), vec3( Texm4 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                               "   oTexCoord2b = mix( vec3( Texm3 * vec4(Tangent,-1,1) ), vec3( Texm4 * vec4(Tangent,-1,1) ), TexCoord1.x );\n"
                               ""
                               "}\n"
            ,
                       "uniform sampler2D Texture0;\n"
                               "uniform samplerCube Texture1;\n"
                               "uniform lowp float UniformColor;\n"
                               "varying highp vec2 oTexCoord;\n"
                               "varying highp vec3 oTexCoord2r;\n"
                               "varying highp vec3 oTexCoord2g;\n"
                               "varying highp vec3 oTexCoord2b;\n"
                               "void main()\n"
                               "{\n"
                               "	lowp vec4 color0 = texture2D(Texture0, oTexCoord);\n"
                               "	lowp vec4 color1r = textureCube(Texture1, oTexCoord2r);\n"
                               "	lowp vec4 color1g = textureCube(Texture1, oTexCoord2g);\n"
                               "	lowp vec4 color1b = textureCube(Texture1, oTexCoord2b);\n"
                               "	lowp vec3 color1 = vec3( color1r.x, color1g.y, color1b.z ) * UniformColor;\n"
                               "	gl_FragColor = vec4( mix( color1, color0.xyz, color0.w ), 1.0);\n"	// pass through destination alpha
                               "}\n"
    );
    buildWarpProgPair( WP_CUBE,
            // low quality
                       "uniform mediump mat4 Mvpm;\n"
                               "uniform mediump mat4 Texm;\n"
                               "uniform mediump mat4 Texm2;\n"
                               "uniform mediump mat4 Texm3;\n"
                               "uniform mediump mat4 Texm4;\n"
                               "uniform mediump vec2 FrameNum;\n"

                               "attribute vec4 Position;\n"
                               "attribute vec2 TexCoord;\n"
                               "attribute vec2 TexCoord1;\n"
                               "varying  vec3 oTexCoord2;\n"
                               "void main()\n"
                               "{\n"
                               "   gl_Position = Mvpm * Position;\n"
                               "   oTexCoord2 = mix( vec3( Texm3 * vec4(TexCoord,-1,1) ), vec3( Texm4 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                               "}\n"
            ,
                       "uniform samplerCube Texture1;\n"
                               "uniform samplerCube Texture2;\n"
                               "uniform samplerCube Texture3;\n"
                               "varying highp vec3 oTexCoord2;\n"
                               "void main()\n"
                               "{\n"
                               "	lowp vec4 color1 = vec4( textureCube(Texture2, oTexCoord2).xyz, 1.0 );\n"
                               "	gl_FragColor = color1;\n"
                               "}\n"
            ,
            // high quality
                       "uniform mediump mat4 Mvpm;\n"
                               "uniform mediump mat4 Texm;\n"
                               "uniform mediump mat4 Texm2;\n"
                               "uniform mediump mat4 Texm3;\n"
                               "uniform mediump mat4 Texm4;\n"
                               "uniform mediump vec2 FrameNum;\n"

                               "attribute vec4 Position;\n"
                               "attribute vec2 TexCoord;\n"	// green
                               "attribute vec2 TexCoord1;\n"
                               "attribute vec2 Normal;\n"		// red
                               "attribute vec2 Tangent;\n"		// blue
                               "varying  vec3 oTexCoord2r;\n"
                               "varying  vec3 oTexCoord2g;\n"
                               "varying  vec3 oTexCoord2b;\n"
                               "void main()\n"
                               "{\n"
                               "   gl_Position = Mvpm * Position;\n"
                               "   oTexCoord2r = mix( vec3( Texm3 * vec4(Normal,-1,1) ), vec3( Texm4 * vec4(Normal,-1,1) ), TexCoord1.x );\n"
                               "   oTexCoord2g = mix( vec3( Texm3 * vec4(TexCoord,-1,1) ), vec3( Texm4 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                               "   oTexCoord2b = mix( vec3( Texm3 * vec4(Tangent,-1,1) ), vec3( Texm4 * vec4(Tangent,-1,1) ), TexCoord1.x );\n"
                               "}\n"
            ,
                       "uniform samplerCube Texture1;\n"
                               "uniform samplerCube Texture2;\n"
                               "uniform samplerCube Texture3;\n"
                               "varying highp vec3 oTexCoord2r;\n"
                               "varying highp vec3 oTexCoord2g;\n"
                               "varying highp vec3 oTexCoord2b;\n"
                               "void main()\n"
                               "{\n"
                               "	lowp float color1r = textureCube(Texture1, oTexCoord2r).x;\n"
                               "	lowp float color1g = textureCube(Texture2, oTexCoord2g).x;\n"
                               "	lowp float color1b = textureCube(Texture3, oTexCoord2b).x;\n"
                               "	gl_FragColor = vec4( color1r, color1g, color1b, 1.0);\n"
                               "}\n"
    );
    buildWarpProgPair( WP_LOADING_ICON,
            // low quality
                       "uniform mediump mat4 Mvpm;\n"
                               "uniform mediump mat4 Texm;\n"
                               "uniform mediump mat4 Texm2;\n"

                               "attribute vec4 Position;\n"
                               "attribute vec2 TexCoord;\n"
                               "attribute vec2 TexCoord1;\n"
                               "varying  vec2 oTexCoord;\n"
                               "void main()\n"
                               "{\n"
                               "   gl_Position = Mvpm * Position;\n"
                               "   vec3 left = vec3( Texm * vec4(TexCoord,-1,1) );\n"
                               "   vec3 right = vec3( Texm2 * vec4(TexCoord,-1,1) );\n"
                               "   vec3 proj = mix( left, right, TexCoord1.x );\n"
                               "	float projIZ = 1.0 / max( proj.z, 0.00001 );\n"
                               "	oTexCoord = vec2( proj.x * projIZ, proj.y * projIZ );\n"
                               "}\n"
            ,
                       "uniform sampler2D Texture0;\n"
                               "uniform sampler2D Texture1;\n"
                               "uniform highp vec4 RotateScale;\n"
                               "varying highp vec2 oTexCoord;\n"
                               "void main()\n"
                               "{\n"
                               "	lowp vec4 color = texture2D(Texture0, oTexCoord);\n"
                               "	highp vec2 iconCenter = vec2( 0.5, 0.5 );\n"
                               "	highp vec2 localCoords = oTexCoord - iconCenter;\n"
                               "	highp vec2 iconCoords = vec2(	( localCoords.x * RotateScale.y - localCoords.y * RotateScale.x ) * RotateScale.z + iconCenter.x,\n"
                               "								( localCoords.x * RotateScale.x + localCoords.y * RotateScale.y ) * -RotateScale.z + iconCenter.x );\n"
                               "	if ( iconCoords.x > 0.0 && iconCoords.x < 1.0 && iconCoords.y > 0.0 && iconCoords.y < 1.0 )\n"
                               "	{\n"
                               "		lowp vec4 iconColor = texture2D(Texture1, iconCoords);"
                               "		color.rgb = ( 1.0 - iconColor.a ) * color.rgb + ( iconColor.a ) * iconColor.rgb;\n"
                               "	}\n"
                               "	gl_FragColor = color;\n"
                               "}\n"
            ,
            // high quality
                       "uniform mediump mat4 Mvpm;\n"
                               "uniform mediump mat4 Texm;\n"
                               "uniform mediump mat4 Texm2;\n"

                               "attribute vec4 Position;\n"
                               "attribute vec2 TexCoord;\n"
                               "attribute vec2 TexCoord1;\n"
                               "varying  vec2 oTexCoord;\n"
                               "void main()\n"
                               "{\n"
                               "   gl_Position = Mvpm * Position;\n"
                               "   vec3 left = vec3( Texm * vec4(TexCoord,-1,1) );\n"
                               "   vec3 right = vec3( Texm2 * vec4(TexCoord,-1,1) );\n"
                               "   vec3 proj = mix( left, right, TexCoord1.x );\n"
                               "	float projIZ = 1.0 / max( proj.z, 0.00001 );\n"
                               "	oTexCoord = vec2( proj.x * projIZ, proj.y * projIZ );\n"
                               "}\n"
            ,
                       "uniform sampler2D Texture0;\n"
                               "uniform sampler2D Texture1;\n"
                               "uniform highp vec4 RotateScale;\n"
                               "varying highp vec2 oTexCoord;\n"
                               "void main()\n"
                               "{\n"
                               "	lowp vec4 color = texture2D(Texture0, oTexCoord);\n"
                               "	highp vec2 iconCenter = vec2( 0.5, 0.5 );\n"
                               "	highp vec2 localCoords = oTexCoord - iconCenter;\n"
                               "	highp vec2 iconCoords = vec2(	( localCoords.x * RotateScale.y - localCoords.y * RotateScale.x ) * RotateScale.z + iconCenter.x,\n"
                               "								( localCoords.x * RotateScale.x + localCoords.y * RotateScale.y ) * -RotateScale.z + iconCenter.x );\n"
                               "	if ( iconCoords.x > 0.0 && iconCoords.x < 1.0 && iconCoords.y > 0.0 && iconCoords.y < 1.0 )\n"
                               "	{\n"
                               "		lowp vec4 iconColor = texture2D(Texture1, iconCoords);"
                               "		color.rgb = ( 1.0 - iconColor.a ) * color.rgb + ( iconColor.a ) * iconColor.rgb;\n"
                               "	}\n"
                               "	gl_FragColor = color;\n"
                               "}\n"
    );
    buildWarpProgPair( WP_MIDDLE_CLAMP,
            // low quality
                       "uniform mediump mat4 Mvpm;\n"
                               "uniform mediump mat4 Texm;\n"
                               "uniform mediump mat4 Texm2;\n"

                               "attribute vec4 Position;\n"
                               "attribute vec2 TexCoord;\n"
                               "attribute vec2 TexCoord1;\n"
                               "varying  vec2 oTexCoord;\n"
                               "void main()\n"
                               "{\n"
                               "   gl_Position = Mvpm * Position;\n"
                               "   vec3 left = vec3( Texm * vec4(TexCoord,-1,1) );\n"
                               "   vec3 right = vec3( Texm2 * vec4(TexCoord,-1,1) );\n"
                               "   vec3 proj = mix( left, right, TexCoord1.x );\n"
                               "	float projIZ = 1.0 / max( proj.z, 0.00001 );\n"
                               "	oTexCoord = vec2( proj.x * projIZ, proj.y * projIZ );\n"
                               "}\n"
            ,
                       "uniform sampler2D Texture0;\n"
                               "uniform highp vec2 TexClamp;\n"
                               "varying highp vec2 oTexCoord;\n"
                               "void main()\n"
                               "{\n"
                               "	gl_FragColor = texture2D(Texture0, vec2( clamp( oTexCoord.x, TexClamp.x, TexClamp.y ), oTexCoord.y ) );\n"
                               "}\n"
            ,
            // high quality
                       "uniform mediump mat4 Mvpm;\n"
                               "uniform mediump mat4 Texm;\n"
                               "uniform mediump mat4 Texm2;\n"

                               "attribute vec4 Position;\n"
                               "attribute vec2 TexCoord;\n"
                               "attribute vec2 TexCoord1;\n"
                               "varying  vec2 oTexCoord;\n"
                               "void main()\n"
                               "{\n"
                               "   gl_Position = Mvpm * Position;\n"
                               "   vec3 left = vec3( Texm * vec4(TexCoord,-1,1) );\n"
                               "   vec3 right = vec3( Texm2 * vec4(TexCoord,-1,1) );\n"
                               "   vec3 proj = mix( left, right, TexCoord1.x );\n"
                               "	float projIZ = 1.0 / max( proj.z, 0.00001 );\n"
                               "	oTexCoord = vec2( proj.x * projIZ, proj.y * projIZ );\n"
                               "}\n"
            ,
                       "uniform sampler2D Texture0;\n"
                               "uniform highp vec2 TexClamp;\n"
                               "varying highp vec2 oTexCoord;\n"
                               "void main()\n"
                               "{\n"
                               "	gl_FragColor = texture2D(Texture0, vec2( clamp( oTexCoord.x, TexClamp.x, TexClamp.y ), oTexCoord.y ) );\n"
                               "}\n"
    );

    buildWarpProgPair( WP_OVERLAY_PLANE,
            // low quality
                       "uniform mediump mat4 Mvpm;\n"
                               "uniform mediump mat4 Texm;\n"
                               "uniform mediump mat4 Texm2;\n"
                               "uniform mediump mat4 Texm3;\n"
                               "uniform mediump mat4 Texm4;\n"

                               "attribute vec4 Position;\n"
                               "attribute vec2 TexCoord;\n"
                               "attribute vec2 TexCoord1;\n"
                               "varying  vec2 oTexCoord;\n"
                               "varying  vec3 oTexCoord2;\n"	// Must do the proj in fragment shader or you get wiggles when you view the plane at even modest angles.
                               "varying  float clampVal;\n"
                               "void main()\n"
                               "{\n"
                               "   gl_Position = Mvpm * Position;\n"
                               "	vec3 proj;\n"
                               "	float projIZ;\n"
                               ""
                               "   proj = mix( vec3( Texm * vec4(TexCoord,-1,1) ), vec3( Texm2 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                               "	projIZ = 1.0 / max( proj.z, 0.00001 );\n"
                               "	oTexCoord = vec2( proj.x * projIZ, proj.y * projIZ );\n"
                               ""
                               "   oTexCoord2 = mix( vec3( Texm3 * vec4(TexCoord,-1,1) ), vec3( Texm4 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                               ""
                               // We need to clamp the projected texcoords to keep from getting a mirror
                               // image behind the view, and mip mapped edge clamp (I wish we had CLAMP_TO_BORDER)
                               // issues far off to the sides.
                               "	vec2 clampXY = oTexCoord2.xy / oTexCoord2.z;\n"
// this is backwards on Stratum    		"	clampVal = ( oTexCoord2.z > -0.01 || clampXY.x < -0.1 || clampXY.y < -0.1 || clampXY.x > 1.1 || clampXY.y > 1.1 ) ? 1.0 : 0.0;\n"
                    "	clampVal = ( oTexCoord2.z < -0.01 || clampXY.x < -0.1 || clampXY.y < -0.1 || clampXY.x > 1.1 || clampXY.y > 1.1 ) ? 1.0 : 0.0;\n"
                    "}\n"
            ,
                       "uniform sampler2D Texture0;\n"
                               "uniform sampler2D Texture1;\n"
                               "varying lowp float clampVal;\n"
                               "varying highp vec2 oTexCoord;\n"
                               "varying highp vec3 oTexCoord2;\n"
                               "void main()\n"
                               "{\n"
                               "	lowp vec4 color0 = texture2D(Texture0, oTexCoord);\n"
                               "	if ( clampVal == 1.0 )\n"
                               "	{\n"
                               "		gl_FragColor = color0;\n"
                               "	} else {\n"
                               "		lowp vec4 color1 = texture2DProj(Texture1, oTexCoord2);\n"
                               "		gl_FragColor = mix( color0, color1, color1.w );\n"
                               "	}\n"
                               "}\n"
            ,
            // high quality
                       "uniform mediump mat4 Mvpm;\n"
                               "uniform mediump mat4 Texm;\n"
                               "uniform mediump mat4 Texm2;\n"
                               "uniform mediump mat4 Texm3;\n"
                               "uniform mediump mat4 Texm4;\n"

                               "attribute vec4 Position;\n"
                               "attribute vec2 TexCoord;\n"	// green
                               "attribute vec2 TexCoord1;\n"
                               "attribute vec2 Normal;\n"		// red
                               "attribute vec2 Tangent;\n"		// blue
                               "varying  vec2 oTexCoord;\n"
                               "varying  vec3 oTexCoord2r;\n"	// These must do the proj in fragment shader or you
                               "varying  vec3 oTexCoord2g;\n"	// get wiggles when you view the plane at even
                               "varying  vec3 oTexCoord2b;\n"	// modest angles.
                               "varying  float clampVal;\n"
                               "void main()\n"
                               "{\n"
                               "   gl_Position = Mvpm * Position;\n"
                               "	vec3 proj;\n"
                               "	float projIZ;\n"
                               ""
                               "   proj = mix( vec3( Texm * vec4(TexCoord,-1,1) ), vec3( Texm2 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                               "	projIZ = 1.0 / max( proj.z, 0.00001 );\n"
                               "	oTexCoord = vec2( proj.x * projIZ, proj.y * projIZ );\n"
                               ""
                               "   oTexCoord2r = mix( vec3( Texm3 * vec4(Normal,-1,1) ), vec3( Texm4 * vec4(Normal,-1,1) ), TexCoord1.x );\n"
                               "   oTexCoord2g = mix( vec3( Texm3 * vec4(TexCoord,-1,1) ), vec3( Texm4 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                               "   oTexCoord2b = mix( vec3( Texm3 * vec4(Tangent,-1,1) ), vec3( Texm4 * vec4(Tangent,-1,1) ), TexCoord1.x );\n"
                               ""
                               // We need to clamp the projected texcoords to keep from getting a mirror
                               // image behind the view, and mip mapped edge clamp (I wish we had CLAMP_TO_BORDER)
                               // issues far off to the sides.
                               "	vec2 clampXY = oTexCoord2r.xy / oTexCoord2r.z;\n"
                               "	clampVal = ( oTexCoord2r.z > -0.01 || clampXY.x < -0.1 || clampXY.y < -0.1 || clampXY.x > 1.1 || clampXY.y > 1.1 ) ? 1.0 : 0.0;\n"
                               "}\n"
            ,
                       "uniform sampler2D Texture0;\n"
                               "uniform sampler2D Texture1;\n"
                               "varying lowp float clampVal;\n"
                               "varying highp vec2 oTexCoord;\n"
                               "varying highp vec3 oTexCoord2r;\n"
                               "varying highp vec3 oTexCoord2g;\n"
                               "varying highp vec3 oTexCoord2b;\n"
                               "void main()\n"
                               "{\n"
                               "	lowp vec4 color0 = texture2D(Texture0, oTexCoord);\n"
                               "	if ( clampVal == 1.0 )\n"
                               "	{\n"
                               "		gl_FragColor = color0;\n"
                               "	} else {\n"
                               "		lowp vec4 color1r = texture2DProj(Texture1, oTexCoord2r);\n"
                               "		lowp vec4 color1g = texture2DProj(Texture1, oTexCoord2g);\n"
                               "		lowp vec4 color1b = texture2DProj(Texture1, oTexCoord2b);\n"
                               "		lowp vec4 color1 = vec4( color1r.x, color1g.y, color1b.z, 1.0 );\n"
                               "		gl_FragColor = mix( color0, color1, vec4( color1r.w, color1g.w, color1b.w, 1.0 ) );\n"
                               "	}\n"
                               "}\n"
    );


    // Debug program to color tint the overlay for LOD visualization
    buildWarpProgMatchedPair( WP_OVERLAY_PLANE_SHOW_LOD,
                              "#version 300 es\n"
                                      "uniform mediump mat4 Mvpm;\n"
                                      "uniform mediump mat4 Texm;\n"
                                      "uniform mediump mat4 Texm2;\n"
                                      "uniform mediump mat4 Texm3;\n"
                                      "uniform mediump mat4 Texm4;\n"

                                      "in vec4 Position;\n"
                                      "in vec2 TexCoord;\n"
                                      "in vec2 TexCoord1;\n"
                                      "out vec2 oTexCoord;\n"
                                      "out vec3 oTexCoord2;\n"	// Must do the proj in fragment shader or you get wiggles when you view the plane at even modest angles.
                                      "out float clampVal;\n"
                                      "void main()\n"
                                      "{\n"
                                      "   gl_Position = Mvpm * Position;\n"
                                      "	vec3 proj;\n"
                                      "	float projIZ;\n"
                                      ""
                                      "   proj = mix( vec3( Texm * vec4(TexCoord,-1,1) ), vec3( Texm2 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                                      "	projIZ = 1.0 / max( proj.z, 0.00001 );\n"
                                      "	oTexCoord = vec2( proj.x * projIZ, proj.y * projIZ );\n"
                                      ""
                                      "   oTexCoord2 = mix( vec3( Texm3 * vec4(TexCoord,-1,1) ), vec3( Texm4 * vec4(TexCoord,-1,1) ), TexCoord1.x );\n"
                                      ""
                                      // We need to clamp the projected texcoords to keep from getting a mirror
                                      // image behind the view, and mip mapped edge clamp (I wish we had CLAMP_TO_BORDER)
                                      // issues far off to the sides.
                                      "	vec2 clampXY = oTexCoord2.xy / oTexCoord2.z;\n"
                                      "	clampVal = ( oTexCoord2.z > -0.01 || clampXY.x < -0.1 || clampXY.y < -0.1 || clampXY.x > 1.1 || clampXY.y > 1.1 ) ? 1.0 : 0.0;\n"
                                      "}\n"
            ,
                              "#version 300 es\n"
                                      "uniform sampler2D Texture0;\n"
                                      "uniform sampler2D Texture1;\n"
                                      "in lowp float clampVal;\n"
                                      "in highp vec2 oTexCoord;\n"
                                      "in highp vec3 oTexCoord2;\n"
                                      "out mediump vec4 fragColor;\n"
                                      "void main()\n"
                                      "{\n"
                                      "	lowp vec4 color0 = texture(Texture0, oTexCoord);\n"
                                      "	if ( clampVal == 1.0 )\n"
                                      "	{\n"
                                      "		fragColor = color0;\n"
                                      "	} else {\n"
                                      "		highp vec2 proj = vec2( oTexCoord2.x, oTexCoord2.y ) / oTexCoord2.z;\n"
                                      "		lowp vec4 color1 = texture(Texture1, proj);\n"
                                      "		mediump vec2 stepVal = fwidth( proj ) * vec2( textureSize( Texture1, 0 ) );\n"
                                      "		mediump float w = max( stepVal.x, stepVal.y );\n"
                                      "		if ( w < 1.0 ) { color1 = mix( color1, vec4( 0.0, 1.0, 0.0, 1.0 ), min( 1.0, 2.0 * ( 1.0 - w ) ) ); }\n"
                                      "		else { color1 = mix( color1, vec4( 1.0, 0.0, 0.0, 1.0 ), min( 1.0, w - 1.0 ) ); }\n"
                                      "		fragColor = mix( color0, color1, color1.w );\n"
                                      "	}\n"
                                      "}\n"
    );

    buildWarpProgMatchedPair( WP_CAMERA,
            // low quality
                              "uniform mediump mat4 Mvpm;\n"
                                      "uniform mediump mat4 Texm;\n"
                                      "uniform mediump mat4 Texm2;\n"
                                      "uniform mediump mat4 Texm3;\n"
                                      "uniform mediump mat4 Texm4;\n"
                                      "uniform mediump mat4 Texm5;\n"

                                      "attribute vec4 Position;\n"
                                      "attribute vec2 TexCoord;\n"
                                      "attribute vec2 TexCoord1;\n"
                                      "varying  vec2 oTexCoord;\n"
                                      "varying  vec2 oTexCoord2;\n"
                                      "void main()\n"
                                      "{\n"
                                      "   gl_Position = Mvpm * Position;\n"

                                      "   vec4 lens = vec4(TexCoord,-1.0,1.0);"
                                      "	vec3 proj;\n"
                                      "	float projIZ;\n"
                                      ""
                                      "   proj = mix( vec3( Texm * lens ), vec3( Texm2 * lens ), TexCoord1.x );\n"
                                      "	projIZ = 1.0 / max( proj.z, 0.00001 );\n"
                                      "	oTexCoord = vec2( proj.x * projIZ, proj.y * projIZ );\n"
                                      ""
                                      "   vec4 dir = mix( lens, Texm2 * lens, TexCoord1.x );\n"
                                      " dir.xy /= dir.z*-1.0;\n"
                                      " dir.z = -1.0;\n"
                                      " dir.w = 1.0;\n"
                                      "	float rolling = Position.y * -1.5 + 0.5;\n"	// roughly 0 = top of camera, 1 = bottom of camera
                                      "   proj = mix( vec3( Texm3 * lens ), vec3( Texm4 * lens ), rolling );\n"
                                      "	projIZ = 1.0 / max( proj.z, 0.00001 );\n"
                                      "	oTexCoord2 = vec2( proj.x * projIZ, proj.y * projIZ );\n"
                                      ""
                                      "}\n"
            ,
                              "#extension GL_OES_EGL_image_external : require\n"
                                      "uniform sampler2D Texture0;\n"
                                      "uniform samplerExternalOES Texture1;\n"
                                      "varying highp vec2 oTexCoord;\n"
                                      "varying highp vec2 oTexCoord2;\n"
                                      "void main()\n"
                                      "{\n"
                                      "	lowp vec4 color0 = texture2D(Texture0, oTexCoord);\n"
                                      "		lowp vec4 color1 = vec4( texture2D(Texture1, oTexCoord2).xyz, 1.0 );\n"
                                      "		gl_FragColor = mix( color1, color0, color0.w );\n"	// pass through destination alpha
//		" gl_FragColor = color1;"
                    "}\n"
    );

}

NV_NAMESPACE_END
