#include <pthread.h>
#include "VFrameSmooth.h"
#include "VAlgorithm.h"
#include <errno.h>
#include <math.h>
#include <sched.h>
#include <unistd.h>			// for usleep

#include <android/sensor.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "VFrameSmooth.h"
#include "android/JniUtils.h"
#include "VLensDistortion.h"
#include "../core/VString.h"
#include "VTimer.h"
#include "VRotationSensor.h"

#include "../core/VLockless.h"
#include "VGlGeometry.h"
#include "VGlShader.h"
#include "VKernel.h"
#include "VDirectRender.h"
#include <android/native_window_jni.h>

NV_NAMESPACE_BEGIN

class VsyncState
{
public:
    long long vsyncCount;
    double	vsyncPeriodNano;
    double	vsyncBaseNano;
};

namespace {
    VLockless<VsyncState>	UpdatedVsyncState;
}

extern "C"
{

    void Java_com_vrseen_VrLib_nativeVsync(JNIEnv *, jclass, jlong frameTimeNanos)
    {

        VsyncState state = UpdatedVsyncState.state();

        // Round this, because different phone models have slightly different periods.
        state.vsyncCount += floor( 0.5 + ( frameTimeNanos - state.vsyncBaseNano ) / state.vsyncPeriodNano );
        state.vsyncPeriodNano = 1e9 / 60.0;
        state.vsyncBaseNano = frameTimeNanos;

        UpdatedVsyncState.setState( state );
    }
}

struct warpSource_t
{
    longlong			MinimumVsync;				// Never pick up a source if it is from the current vsync.
    longlong			FirstDisplayedVsync[2];		// External velocity is added after this vsync.
    bool				disableChromaticCorrection;	// Disable correction for chromatic aberration.
    EGLSyncKHR			GpuSync;					// When this sync completes, the textures are done rendering.
    ovrTimeWarpParms	WarpParms;					// passed into WarpSwap()

    warpSource_t()
        : MinimumVsync(0)
        , FirstDisplayedVsync{0, 0}
        , disableChromaticCorrection(false)
        , GpuSync(nullptr)
    {
    }
};

struct swapProgram_t
{
    // When a single thread is doing both the eye rendering and
    // the warping, we will want to do the sensor read for the
    // next frame on the second eye instead of the first.
    bool	singleThread;

    // The eye 0 texture will be used for both window eyes.
    bool	dualMonoDisplay;

    // Ensure that at least these Fractions of a frame have scanned
    // before starting the eye warps.
    float	deltaVsync[2];

    // Use prediction values in frames from the same
    // base vsync as deltaVsync, so they can be
    // scaled by frame times to get milliseconds, allowing
    // 60 / 90 / 120 hz displays.
    //
    // For a global shutter low persistence display, all of these
    // values should be the same.
    //
    // If the single thread rendering rate can drop below the vsync
    // rate, all of the values should also be the same, because it
    // would stay on screen without changing.
    //
    // For an incrementally displayed landscape scanned display,
    // The left and right values will be the same.
    float	predictionPoints[2][2];	// [left/right][start/stop]
};

// This is communicated from the VFrameSmooth thread to the VrThread at
// vsync time.
struct SwapState
{
    SwapState() : VsyncCount(0),EyeBufferCount(0) {}
    long long		VsyncCount;
    long long		EyeBufferCount;
};

struct eyeLog_t
{
    // If we dropped an entire frame, this will be true for both eyes.
    bool		skipped;

    // if this hasn't changed from the previous frame, the main thread
    // dropped a frame.
    int			bufferNum;

    // Time relative to the sleep point for this eye.  Both should
    // be below 0.008 to avoid tearing.
    float		issueFinish;
    float		completeFinish;

    // The delta from the time used to calculate the eye rendering pose
    // to the top of this frame.  Should be one frame period plus sensor jitter
    // if running synchronously at the video frame rate.
    float		poseLatencySeconds;
};

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



ANativeWindow* GetNativeWindow( JNIEnv * jni, jobject activity )
{
	vInfo("Test do GetNativeWindow!");

//	if(!plugin.isInitialized)
//	{
//		LogError("svrapi not initialized yet!");
//		return;
//	}

	jclass activityClass = jni->GetObjectClass(activity);
	if (activityClass == NULL)
	{
		vInfo("activityClass == NULL!");
		return NULL;
	}
	vInfo("Test do GetNativeWindow Get mUnityPlayer fid");

	jfieldID fid = jni->GetFieldID(activityClass, "mUnityPlayer", "Lcom/unity3d/player/UnityPlayer;");
	if (fid == NULL)
	{
		vInfo("mUnityPlayer not found!");
		return NULL;
	}

	vInfo("Test do GetNativeWindow ");
	jobject unityPlayerObj = jni->GetObjectField(activity, fid);
	if(unityPlayerObj == NULL)
	{
		vInfo("unityPlayer object not found!");
		return NULL;
	}

	vInfo("Test do GetNativeWindow! 3");
	jclass unityPlayerClass = jni->GetObjectClass(unityPlayerObj);
	if (unityPlayerClass == NULL)
	{
		vInfo("unityPlayer class not found!");
		return NULL;
	}

	vInfo("Test do GetNativeWindow! 4");
	jmethodID mid = jni->GetMethodID(unityPlayerClass, "getChildAt", "(I)Landroidiew/View;");
	if (mid == NULL)
	{
		vInfo("getChildAt methodID not found!");
		return NULL;
	}

	vInfo("Test do GetNativeWindow! 5");
	jboolean param = 0;
	jobject surfaceViewObj = jni->CallObjectMethod( unityPlayerObj, mid, param);
	if (surfaceViewObj == NULL)
	{
		vInfo("surfaceView object not found!");
		return NULL;
	}

	vInfo("Test do GetNativeWindow! 6");
	jclass surfaceViewClass = jni->GetObjectClass(surfaceViewObj);
	mid = jni->GetMethodID(surfaceViewClass, "getHolder", "()Landroidiew/SurfaceHolder;");
	if (mid == NULL)
	{
		vInfo("getHolder methodID not found!");
		return NULL;
	}

	vInfo("Test do GetNativeWindow! 7");
	jobject surfaceHolderObj = jni->CallObjectMethod( surfaceViewObj, mid);
	if (surfaceHolderObj == NULL)
	{
		vInfo("surfaceHolder object not found!");
		return NULL;
	}

	vInfo("Test do GetNativeWindow! 8");
	jclass surfaceHolderClass = jni->GetObjectClass(surfaceHolderObj);
	mid = jni->GetMethodID(surfaceHolderClass, "getSurface", "()Landroidiew/Surface;");
	if (mid == NULL)
	{
		vInfo("getSurface methodID not found!");
		return NULL;
	}

	vInfo("GetNativeWindow success!!");
	jobject surface = jni->CallObjectMethod( surfaceHolderObj, mid);
	//LOG("EGLTEST nativeWindowSurface2 = %p", surface);
	ANativeWindow* nativeWindow = ANativeWindow_fromSurface(jni, surface);
	return nativeWindow;
}


//=========================================================================================

// If lensCentered, the coordinates will be square and extend past the left and
// right edges for a viewport.
//
// Otherwise, it will cover only the pixels in the correct half of the screen
// for a scissor rect.

// If the window is wider than it is tall, ie 1920x1080
void EyeRectLandscape( const VDevice *device,int eye,int &x, int &y, int &width, int &height )
{
    // always scissor exactly to half the screen
    int scissorX = ( eye == 0 ) ? 0 : device->widthbyPixels / 2;
    int scissorY = 0;
    int scissorWidth = device->widthbyPixels / 2;
    int scissorHeight = device->heightbyPixels;
    x = scissorX;
    y = scissorY;
    width = scissorWidth;
    height = scissorHeight;
    return;

    const float	metersToPixels = device->widthbyPixels / device->widthbyMeters;

    // Even though the lens center is shifted outwards slightly,
    // the height is still larger than the largest horizontal value.
    // TODO: check for sure on other HMD
    const int	pixelRadius = device->heightbyPixels / 2;
    const int	pixelDiameter = pixelRadius * 2;
    const float	horizontalShiftMeters = ( device->lensDistance / 2 ) - ( device->widthbyMeters / 4 );
    const float	horizontalShiftPixels = horizontalShiftMeters * metersToPixels;

    // Make a viewport that is symetric, extending off the sides of the screen and into the other half.
    x = device->widthbyPixels/4 - pixelRadius + ( ( eye == 0 ) ? - horizontalShiftPixels : device->widthbyPixels/2 + horizontalShiftPixels );
    y = 0;
    width = pixelDiameter;
    height = pixelDiameter;
}

void EyeRect( const VDevice *device,const int eye,
              int &x, int &y, int &width, int &height )
{
    int	lx, ly, lWidth, lHeight;
    EyeRectLandscape( device, eye, lx, ly, lWidth, lHeight );

    x = lx;
    y = ly;
    width = lWidth;
    height = lHeight;
}

VMatrix4f CalculateTimeWarpMatrix2( const VQuatf &inFrom, const VQuatf &inTo )
{
    // FIXME: this is a horrible hack to fix a zero quaternion that's passed in
    // the night before a demo. This is coming from the sensor pose and needs to
    // be tracked down further.
    VQuatf from = inFrom;
    VQuatf to = inTo;

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
            from = VQuatf( 0.0f, 0.0f, 0.0f, 1.0f ); // just force identity
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
            to = VQuatf( 0.0f, 0.0f, 0.0f, 1.0f ); // just force identity
        }
    }

    VMatrix4f		lastSensorMatrix = VMatrix4f( to );
    VMatrix4f		lastViewMatrix = VMatrix4f( from );

    return ( lastSensorMatrix.inverted() * lastViewMatrix ).inverted();
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

//=========================================================================================

struct VFrameSmooth::Private
{
    Private(bool async,bool wantSingleBuffer):
            m_untexturedMvpProgram(),
            m_debugLineProgram(),
            m_warpPrograms(),
            m_blackTexId( 0 ),
            m_defaultLoadingIconTexId( 0 ),
            m_wantSingleBuffer(wantSingleBuffer),
            m_hasEXT_sRGB_write_control( false ),
            m_sStartupTid( 0 ),
            m_jni( NULL ),
//            m_eglDisplay( 0 ),
//            m_eglPbufferSurface( 0 ),
            m_eglMainThreadSurface( 0 ),
//            m_eglConfig( 0 ),
            m_eglClientVersion( 0 ),
            m_eglShareContext( 0 ),
//            m_eglWarpContext( 0 ),
            m_contextPriority( 0 ),
            m_eyeLog(),
            m_lastEyeLog( 0 ),
            m_warpThread( 0 ),
            m_warpThreadTid( 0 ),
            m_lastSwapVsyncCount( 0 )
    {
        // Code which auto-disable chromatic aberration expects
        // the warpProgram list to be symmetric.
        // See disableChromaticCorrection.
//        OVR_COMPILER_ASSERT( ( WP_PROGRAM_MAX & 1 ) == 0 );
//        OVR_COMPILER_ASSERT( ( WP_CHROMATIC - WP_SIMPLE ) ==
//                             ( WP_PROGRAM_MAX - WP_CHROMATIC ) );

        //VGlOperation glOperation;
        m_shutdownRequest.setState( false );
        m_eyeBufferCount.setState( 0 );
        memset( m_warpSources, 0, sizeof( m_warpSources ) );
        memset( m_warpPrograms, 0, sizeof( m_warpPrograms ) );

        // set up our synchronization primitives
        pthread_mutex_init( &m_swapMutex, NULL /* default attributes */ );
        pthread_cond_init( &m_swapIsLatched, NULL /* default attributes */ );

        vInfo( "-------------------- VFrameSmooth() --------------------" );

        // Only allow WarpSwap() to be called from this thread
        m_sStartupTid = gettid();

        // Keep track of the last time WarpSwap() was called.
        m_lastWarpSwapTimeInSeconds.setState( VTimer::Seconds());

        // If this isn't set, Shutdown() doesn't need to kill the thread
        m_warpThread = 0;

        m_async = async;
        m_device = VDevice::instance();;

        // No buffers have been submitted yet
        m_eyeBufferCount.setState( 0 );

        //---------------------------------------------------------
        // OpenGL initialization that can be done on the main thread
        //---------------------------------------------------------

        // Get values for the current OpenGL context
//        m_eglDisplay = eglGetCurrentDisplay();
//        if ( m_eglDisplay == EGL_NO_DISPLAY )
//        {
//            FAIL( "EGL_NO_DISPLAY" );
//        }

        // 初始化smooth线程的gl状态
        m_eglStatus.updateDisplay();


        JavaVM *javaVM = JniUtils::GetJavaVM();
        javaVM->AttachCurrentThread(&m_jni, nullptr);
        m_eglMainThreadSurface = eglGetCurrentSurface( EGL_DRAW );

        if ( m_eglMainThreadSurface == EGL_NO_SURFACE )
		{
			vFatal( "EGL_NO_SURFACE" );
		}

        m_eglShareContext = eglGetCurrentContext();

        if ( m_eglShareContext == EGL_NO_CONTEXT )
        {
		   vFatal( "EGL_NO_CONTEXT" );
        }



        EGLint configID;
        if ( !eglQueryContext( m_eglStatus.m_display, m_eglShareContext, EGL_CONFIG_ID, &configID ) )
        {
            vFatal( "eglQueryContext EGL_CONFIG_ID failed" );
        }
        m_eglStatus.m_config = m_eglStatus.eglConfigForConfigID( configID );
        if ( m_eglStatus.m_config == NULL )
        {
            vFatal( "EglConfigForConfigID failed" );
        }
        if ( !eglQueryContext( m_eglStatus.m_display, m_eglShareContext, EGL_CONTEXT_CLIENT_VERSION, (EGLint *)&m_eglClientVersion ) )
        {
            vFatal( "eglQueryContext EGL_CONTEXT_CLIENT_VERSION failed" );
        }
        vInfo( "Current EGL_CONTEXT_CLIENT_VERSION:" << m_eglClientVersion );


        // It is wasteful for the main config to be anything but a color buffer.
        EGLint depthSize = 0;
        eglGetConfigAttrib( m_eglStatus.m_display, m_eglStatus.m_config, EGL_DEPTH_SIZE, &depthSize );
        if ( depthSize != 0 )
        {
            vInfo( "Share context eglConfig has" <<depthSize<< "depth bits -- should be 0");
        }

        EGLint samples = 0;
        eglGetConfigAttrib( m_eglStatus.m_display, m_eglStatus.m_config, EGL_SAMPLES, &samples );
        if ( samples != 0 )
        {
            vInfo( "Share context eglConfig has" <<samples<< " samples -- should be 0");
        }

        //TODO::Compare the EGLConfig which create the EGLSurface
        vInfo("<<---EGLConfig attributes list start--->>");
        EGLint value = 0;

        eglGetConfigAttrib( m_eglStatus.m_display, m_eglStatus.m_config, EGL_ALPHA_SIZE, &value);
        vInfo("EGL_ALPHA_SIZE : " <<value);
        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_ALPHA_MASK_SIZE, &value);
        vInfo("EGL_ALPHA_MASK_SIZE : "<<value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_BIND_TO_TEXTURE_RGB, &value);
        if (value==EGL_TRUE)
        {
        	vInfo("EGL_BIND_TO_TEXTURE_RGB : TRUE");
        }
        else
        	vInfo("EGL_BIND_TO_TEXTURE_RGB : false");

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_BIND_TO_TEXTURE_RGBA, &value);
        if (value==EGL_TRUE)
		{
			vInfo("EGL_BIND_TO_TEXTURE_RGBA : TRUE");
		}
		else
			vInfo("EGL_BIND_TO_TEXTURE_RGBA : false");

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_BLUE_SIZE, &value);
        vInfo("EGL_BLUE_SIZE : " << value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_BUFFER_SIZE, &value);
        vInfo("EGL_BUFFER_SIZE : " << value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_COLOR_BUFFER_TYPE, &value);
        if (value == EGL_RGB_BUFFER)
        {
        	vInfo("EGL_COLOR_BUFFER_TYPE : EGL_RGB_BUFFER");
        }
        else
        {
        	vInfo("EGL_COLOR_BUFFER_TYPE : EGL_LUMINANCE_BUFFER");
        }

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_CONFIG_CAVEAT, &value);
        if (value == EGL_SLOW_CONFIG)
        {
        	vInfo("EGL_CONFIG_CAVEAT : EGL_SLOW_CONFIG");
        }
        else if (value == EGL_NON_CONFORMANT_CONFIG)
        {
        	vInfo("EGL_CONFIG_VAVEAT : EGL_NON_CONFORMANT_CONFIG");
        }
        else
        {
        	vInfo("EGL_CONFIG_VAVEAT : EGL_NONE");
        }

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_CONFIG_ID, &value);
        vInfo("EGL_CONFIG_ID : " << value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_CONFORMANT, &value);
        vInfo("EGL_CONFORMANT : " << value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_DEPTH_SIZE, &value);
        vInfo("EGL_DEPTH_SIZE : "<< value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_GREEN_SIZE, &value);
        vInfo("EGL_GREEN_SIZE : " << value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_LEVEL, &value);
        vInfo("EGL_LEVEL : " << value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_LUMINANCE_SIZE, &value);
        vInfo("EGL_LUMINANCE_SIZE : " << value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_MAX_PBUFFER_WIDTH, &value);
        vInfo("EGL_MAX_PBUFFER_WIDTH : " << value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_MAX_PBUFFER_HEIGHT, &value);
        vInfo("EGL_MAX_PBUFFER_HEIGHT : " << value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_MAX_SWAP_INTERVAL, &value);
        vInfo("EGL_MAX_SWAP_INTERVAL : " << value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_MIN_SWAP_INTERVAL, &value);
        vInfo("EGL_MIN_SWAP_INTERVAL : " << value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_NATIVE_RENDERABLE, &value);
        if(value == EGL_TRUE)
        {
        	vInfo("EGL_NATIVE_RENDERABLE : TRUE");
        }
        else
        {
        	vInfo("EGL_NATIVE_RENDERABLE : FALSE");
        }

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_NATIVE_VISUAL_ID, &value);
        vInfo("EGL_NATIVE_VISUAL_ID : " << value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_NATIVE_VISUAL_TYPE, &value);
        vInfo("EGL_NATIVE_VISUAL_TYPE : " << value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_RED_SIZE, &value);
        vInfo("EGL_RED_SIZE : " << value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_RENDERABLE_TYPE, &value);
        vInfo("EGL_RENDERABLE_TYPE : " << value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_SAMPLE_BUFFERS, &value);
        vInfo("EGL_SAMPLE_BUFFERS : " << value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_SAMPLES, &value);
        vInfo("EGL_SAMPLES : "<< value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_STENCIL_SIZE, &value);
        vInfo("EGL_STENCIL_SIZE : "<< value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_SURFACE_TYPE, &value);
        vInfo("EGL_SURFACE_TYPE : "<< value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_TRANSPARENT_TYPE, &value);
        vInfo("EGL_TRANSPARENT_TYPE ： " <<value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_TRANSPARENT_RED_VALUE, &value);
        vInfo("EGL_TRANSPARENT_RED_VALUE" << value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_TRANSPARENT_GREEN_VALUE, &value);
        vInfo("EGL_TRANSPARENT_GREEN_VALUE" << value);

        eglGetConfigAttrib(m_eglStatus.m_display, m_eglStatus.m_config, EGL_TRANSPARENT_BLUE_VALUE, &value);
        vInfo("EGL_TRANSPARENT_BLUE_VALUE" << value);

        vInfo("<<---EGLConfig attributes list END--->>");
        //END THE COMPARE

        //TODO::EGLSurface attributes
        EGLDisplay tmpdisplay = m_eglStatus.m_display;

        EGLSurface tmpsurface = m_eglMainThreadSurface;

        eglQuerySurface(tmpdisplay, tmpsurface, EGL_CONFIG_ID, &value);
        vInfo("EGL_CONFIG_ID : "<< value);

        eglQuerySurface(tmpdisplay, tmpsurface, EGL_HEIGHT, &value);
        vInfo("EGL_HEIGHT : " << value);

        eglQuerySurface(tmpdisplay, tmpsurface, EGL_HORIZONTAL_RESOLUTION, &value);
        vInfo("EGL_HORIZONTAL_RESOLUTION : " << value);

        eglQuerySurface(tmpdisplay, tmpsurface, EGL_LARGEST_PBUFFER, &value);
        vInfo("EGL_LARGEST_PBUFFER :" << value);

        eglQuerySurface(tmpdisplay, tmpsurface, EGL_MIPMAP_LEVEL, &value);
        vInfo("EGL_MIPMAP_LEVEL : " << value);

        eglQuerySurface(tmpdisplay, tmpsurface, EGL_MIPMAP_TEXTURE, &value);
        if (value == EGL_TRUE)
        {
        	vInfo("EGL_MIPMAP_TEXTURE : TRUE" );
        }
        else
        	vInfo("EGL_MIPMAP_TEXTURE : FALSE");

        eglQuerySurface(tmpdisplay, tmpsurface, EGL_MULTISAMPLE_RESOLVE, &value);
        if (value == EGL_MULTISAMPLE_RESOLVE_DEFAULT )
        {
        	vInfo("EGL_MULTISAMPLE_RESOLVE : EGL_MULTISAMPLE_RESOLVE_DEFAULT ");
        }
        else
        	vInfo("EGL_MULTISAMPLE_RESOLVE : EGL_MULTISAMPLE_RESOLVE_BOX");

        eglQuerySurface(tmpdisplay, tmpsurface, EGL_PIXEL_ASPECT_RATIO, &value);
        vInfo("EGL_PIXEL_ASPECT_RATIO : " << value);

        eglQuerySurface(tmpdisplay, tmpsurface, EGL_RENDER_BUFFER, &value);
        if (value == EGL_BACK_BUFFER)
        {
        	vInfo("EGL_RENDER_BUFFER : EGL_BACK_BUFFER");
        }
		else if (value == EGL_SINGLE_BUFFER)
		{
			vInfo("EGL_RENDER_BUFFER : EGL_SINGLE_BUFFER");
		}
		else
			vInfo("EGL_RENDER_BUFFER : OTHER_BUFFER");

        eglQuerySurface(tmpdisplay, tmpsurface, EGL_SWAP_BEHAVIOR, &value);
        if (value == EGL_BUFFER_PRESERVED)
        {
        	vInfo("EGL_SWAP_BEHAVIOR : EGL_BUFFER_PRESERVED");
        }
        else if (value == EGL_BUFFER_DESTROYED)
        {
        	vInfo("EGL_SWAP_BEHAVIOR : EGL_BUFFER_DESTROYED");
        }

        eglQuerySurface(tmpdisplay, tmpsurface, EGL_TEXTURE_FORMAT, &value);
        if(value == EGL_NO_TEXTURE)
        {
        	vInfo("EGL_TEXTURE_FORMAT : EGL_NO_TEXTURE");
        }
        else if (value == EGL_TEXTURE_RGB)
        {
        	vInfo("EGL_TEXTURE_FORMAT : EGL_TEXTURE_RGB");
        }
        else if (value == EGL_TEXTURE_RGBA)
        {
        	vInfo("EGL_TEXTURE_FORMAT : EGL_TEXTURE_RGBA");
        }

        eglQuerySurface(tmpdisplay , tmpsurface, EGL_TEXTURE_TARGET, &value);
        if (value == EGL_NO_TEXTURE)
        {
        	vInfo("EGL_TEXTURE_TARGET : EGL_NO_TEXTURE");
        }
        else if (value == EGL_TEXTURE_2D)
        {
        	vInfo("EGL_TEXTURE_TARGET : EGL_TEXTURE_2D");
        }

        eglQuerySurface(tmpdisplay, tmpsurface, EGL_VERTICAL_RESOLUTION, &value);
        vInfo("EGL_VERTICAL_RESOLUTION : " << value);

        eglQuerySurface(tmpdisplay, tmpsurface, EGL_WIDTH, &value);
        vInfo("EGL_WIDTH : " << value);

        // See if we have sRGB_write_control extension
        m_hasEXT_sRGB_write_control = m_eglStatus.glIsExtensionString( "GL_EXT_sRGB_write_control");

        m_buildVersionSDK = VKernel::instance()->getBuildVersion();
        // Skip thread initialization if we are running synchronously
        if ( !m_async )
        {
            m_screen.initForCurrentSurface( m_jni, m_wantSingleBuffer,m_buildVersionSDK);

            // create the framework graphics on this thread
            createFrameworkGraphics();
            vInfo( "Skipping thread setup because !AsynchronousTimeWarp" );
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
                vInfo( "Requesting EGL_CONTEXT_PRIORITY_HIGH_IMG" );
                m_contextPriority = EGL_CONTEXT_PRIORITY_HIGH_IMG;
            }
            else
            {
                // If we can't report the priority, assume the extension isn't there
                vInfo( "IMG_Context_Priority doesn't seem to be present." );
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
            m_eglStatus.m_pbufferSurface = eglCreatePbufferSurface( m_eglStatus.m_display, m_eglStatus.m_config, attrib_list );
            if ( m_eglStatus.m_pbufferSurface == EGL_NO_SURFACE )
            {
                vFatal( "eglCreatePbufferSurface failed");
            }

            if ( eglMakeCurrent( m_eglStatus.m_display, m_eglStatus.m_pbufferSurface, m_eglStatus.m_pbufferSurface,
                                 m_eglShareContext ) == EGL_FALSE )
            {
                vFatal( "eglMakeCurrent: eglMakeCurrent pbuffer failed" );
            }

            EGLSurface tmpSurface = eglGetCurrentSurface(EGL_DRAW);

            vInfo("To compare surface : pbufffersurface : " << m_eglStatus.m_pbufferSurface <<" mainthreadsurface :" <<m_eglMainThreadSurface << " currentsurface : "<<tmpSurface);

            if(eglDestroySurface(m_eglStatus.m_display, m_eglMainThreadSurface) == EGL_FALSE)
            {
            	vInfo("Can't destroy mainthreadsurface!");
            }

            EGLint attribs[100];
            int nums = 0;
            attribs[nums++] = EGL_RENDER_BUFFER;
            attribs[nums++] = EGL_BACK_BUFFER;
            attribs[nums++] = EGL_NONE;

            m_eglMainThreadSurface = eglCreateWindowSurface(m_eglStatus.m_display, m_eglStatus.m_config, VKernel::instance()->m_NativeWindow, attribs);
            if (m_eglMainThreadSurface == EGL_NO_SURFACE)
            {
            	vInfo("Can't create mainthreadsurface!")
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
                vFatal( "pthread_create returned ");
            }

            // Atomically unlock the mutex and block until the warp thread
            // has completed the initialization and either failed or went
            // into WarpThreadLoop()
            pthread_cond_wait( &m_swapIsLatched, &m_swapMutex );

            // Pthread_cond_wait re-locks the mutex before exit.
            pthread_mutex_unlock( &m_swapMutex );
        }

        vInfo( "----------------- VFrameSmooth() End -----------------" );
    }

    void destroy()
    {
        vInfo( "---------------- ~VFrameSmooth() Start ----------------" );
        if ( m_warpThread != 0 )
        {
            // Get the background thread to kill itself.
            m_shutdownRequest.setState( true );

            vInfo( "pthread_join() called");
            void * data;
            pthread_join( m_warpThread, &data );

            vInfo( "pthread_join() returned");

            m_warpThread = 0;

            ////VGlOperation glOperation;
            if ( eglGetCurrentSurface( EGL_DRAW ) != m_eglStatus.m_pbufferSurface )
            {
                vInfo( "eglGetCurrentSurface( EGL_DRAW ) != eglPbufferSurface" );
            }

            // Attach the windowSurface to the calling context again.
            if ( eglMakeCurrent( m_eglStatus.m_display, m_eglMainThreadSurface,
                                 m_eglMainThreadSurface, m_eglShareContext ) == EGL_FALSE)
            {
                vFatal( "eglMakeCurrent to window failed");
            }

            // Destroy the pbuffer surface that was attached to the calling context.
            if ( EGL_FALSE == eglDestroySurface( m_eglStatus.m_display, m_eglStatus.m_pbufferSurface ) )
            {
                vWarn( "Failed to destroy pbuffer." );
            }
            else
            {
                vInfo( "Destroyed pbuffer." );
            }
        }
        else
        {
            // Vertex array objects can only be destroyed on the context they were created on
            // If there is no warp thread then InitParms.AsynchronousTimeWarp was false and
            // CreateFrameworkGraphics() was called from the VFrameSmooth constructor.
            m_screen.shutdown();
            destroyFrameworkGraphics();
        }

        vInfo( "---------------- ~VFrameSmooth() End ----------------" );
    }

    // POSIX thread launching shim, just calls WarpThread()
    static void *	ThreadStarter( void * parm );

    void			threadFunction();
    void 			warpThreadInit();
    void			warpThreadShutdown();
    void			warpSwapInternal( const ovrTimeWarpParms & parms );

    // Ensures that the warpPrograms have a matched set with and without
    // chromatic aberration so it can be universally disabled for slower systems
    // and power saving mode.
    void			buildWarpProgPair( VrKernelProgram simpleIndex,
                                       const char * simpleVertex, const char * simpleFragment,
                                       const char * chromaticVertex, const char * chromaticFragment );

    // If there is no difference between the low and high quality versions, use this function.
    void			buildWarpProgMatchedPair( VrKernelProgram simpleIndex,
                                              const char * vertex, const char * fragment );
    void 			buildWarpProgs();

    // FrameworkGraphics include the latency tester, calibration lines, edge vignette, fps counter,
    // debug graphs.
    void			createFrameworkGraphics();
    void			destroyFrameworkGraphics();
    void			drawFrameworkGraphicsToWindow( const int eye, const int swapOptions);

    //用于管理同步的函数
    double			getFractionalVsync();
    double			framePointTimeInSeconds( const double framePoint ) const;
    float 			sleepUntilTimePoint( const double targetSeconds, const bool busyWait );

    bool m_async;
    VDevice *m_device;

    VGlShader		m_untexturedMvpProgram;
    VGlShader		m_debugLineProgram;
    VGlShader		m_warpPrograms[ WP_PROGRAM_MAX ];
    GLuint			m_blackTexId;
    GLuint			m_defaultLoadingIconTexId;
    VGlGeometry	m_calibrationLines2;		// simple cross
    VGlGeometry	m_warpMesh;
    VGlGeometry	m_sliceMesh;
    VGlGeometry	m_cursorMesh;
    VGlGeometry	m_timingGraph;
    static const int NUM_SLICES_PER_EYE = 4;
    static const int NUM_SLICES_PER_SCREEN = NUM_SLICES_PER_EYE*2;

    //single buffer
    DirectRender	m_screen;
    bool m_wantSingleBuffer;
    int m_buildVersionSDK;

    // Wait for sync points amd warp to screen.
    void			warpToScreen( const double vsyncBase, const swapProgram_t & swap );
    void			warpToScreenSliced( const double vsyncBase, const swapProgram_t & swap );

    const VGlShader & programForParms( const ovrTimeWarpParms & parms, const bool disableChromaticCorrection ) const;
    void			setWarpState( const warpSource_t & currentWarpSource ) const;
    void			bindWarpProgram( const warpSource_t & currentWarpSource, const VMatrix4f timeWarps[2][2],
                                     const VMatrix4f rollingWarp, const int eye, const double vsyncBase ) const;
    void			bindCursorProgram() const;

    bool			m_hasEXT_sRGB_write_control;	// extension

    // It is an error to call WarpSwap() from a different thread
    pid_t			m_sStartupTid;

    // To change SCHED_FIFO on the StartupTid.
    JNIEnv *		m_jni;

    // Last time WarpSwap() was called.
    VLockless<double>		m_lastWarpSwapTimeInSeconds;

    // Retrieved from the main thread context
    //EGLDisplay		m_eglDisplay;

    VEglDriver      m_eglStatus;

//    EGLSurface		m_eglPbufferSurface;
    EGLSurface		m_eglMainThreadSurface;
//    EGLConfig		m_eglConfig;
    EGLint			m_eglClientVersion;	// VFrameSmooth can work with EGL 2.0 or 3.0
    EGLContext		m_eglShareContext;

    // Our private context, only used for warping to the screen.
//    EGLContext		m_eglWarpContext;
    GLuint			m_contextPriority;

    // Data for timing graph
    static const int EYE_LOG_COUNT = 512;
    eyeLog_t		m_eyeLog[EYE_LOG_COUNT];
    long long		m_lastEyeLog;	// eyeLog[(lastEyeLog-1)&(EYE_LOG_COUNT-1)] has valid data

    // The warp loop will exit when this is set true.
    VLockless<bool>		m_shutdownRequest;

    // If this is 0, we don't have a thread running.
    pthread_t		m_warpThread;		// posix pthread
    int				m_warpThreadTid;	// linux tid

    // Used to allow the VrThread to sleep until next vsync.
    pthread_mutex_t m_swapMutex;
    pthread_cond_t	m_swapIsLatched;

    // The VrThread submits a buffer set after all drawing commands have
    // been issued for it and flushed, but probably are not completed.
    //
    // warpSources[eyeBufferCount%MAX_WARP_SOURCES] is the most recently
    // submitted.
    //
    // WarpSwap will not continue until the previous buffer set has completed,
    // to prevent GPU latency pileup.
    static const int MAX_WARP_SOURCES = 4;
    VLockless<long long>			m_eyeBufferCount;	// only set by WarpSwap()
    warpSource_t	m_warpSources[MAX_WARP_SOURCES];

    VLockless<SwapState>		m_swapVsync;		// Set by WarpToScreen(), read by WarpSwap()

    long long			m_lastSwapVsyncCount;			// SwapVsync at return from last WarpSwap()
};

double	VFrameSmooth::Private::getFractionalVsync()
{
    const VsyncState state = UpdatedVsyncState.state();

    const jlong t = VTimer::TicksNanos();
    if ( state.vsyncBaseNano == 0 )
    {
        return 0;
    }
    const double vsync = (double)state.vsyncCount + (double)(t - state.vsyncBaseNano ) / state.vsyncPeriodNano;
    return vsync;
}
double	VFrameSmooth::Private::framePointTimeInSeconds( const double framePoint )const
{
    const VsyncState state = UpdatedVsyncState.state();
    const double seconds = ( state.vsyncBaseNano + ( framePoint - state.vsyncCount ) * state.vsyncPeriodNano ) * 1e-9;
    return seconds;
}
float 	VFrameSmooth::Private::sleepUntilTimePoint( const double targetSeconds, const bool busyWait )
{
    const float sleepSeconds = targetSeconds - VTimer::Seconds();
    if ( sleepSeconds > 0 )
    {
        if ( busyWait )
        {
            while( targetSeconds - VTimer::Seconds() > 0 )
            {
            }
        }
        else
        {
            // I'm assuming we will never sleep more than one full second.
            timespec	t, rem;
            t.tv_sec = 0;
            t.tv_nsec = sleepSeconds * 1e9;
            nanosleep( &t, &rem );
            const double overSleep = VTimer::Seconds() - targetSeconds;
            if ( overSleep > 0.001 )
            {
    			vInfo("Overslept " << overSleep << " seconds");
            }
        }
    }
    return sleepSeconds;
}

// Shim to call a C++ object from a posix thread start.
void *VFrameSmooth::Private::ThreadStarter( void * parm )
{
    VFrameSmooth::Private & tw = *(VFrameSmooth::Private *)parm;
    tw.threadFunction();
    return NULL;
}

void VFrameSmooth::Private::threadFunction()
{
	JavaVM *javaVM = JniUtils::GetJavaVM();
	javaVM->AttachCurrentThread(&m_jni, nullptr);

    warpThreadInit();

    // Signal the main thread to wake up and return.
    pthread_mutex_lock( &m_swapMutex );
    pthread_cond_signal( &m_swapIsLatched );
    pthread_mutex_unlock( &m_swapMutex );

    vInfo( "WarpThreadLoop()" );

    bool removedSchedFifo = false;

    // Loop until we get a shutdown request
    for ( double vsync = 0; ; vsync++ )
    {
        const double current = ceil( getFractionalVsync() );
        if ( abs( current - vsync ) > 2.0 )
        {
            vInfo( "Changing vsync from" <<vsync<<  " to " << current);
            vsync = current;
        }
        if ( m_shutdownRequest.state() )
        {
            vInfo( "ShutdownRequest received" );
            break;
        }

        // The time warp thread functions as a watch dog for the calling thread.
        // If the calling thread does not call WarpSwap() in a long time then this code
        // removes SCHED_FIFO from the calling thread to keep the Android watch dog from
        // rebooting the device.
        // SCHED_FIFO is set back as soon as the calling thread calls WarpSwap() again.
       const double currentTime = VTimer::Seconds();
       const double lastWarpTime = m_lastWarpSwapTimeInSeconds.state();
       if ( removedSchedFifo )
       {
           if ( lastWarpTime > currentTime - 0.1 )
           {
               removedSchedFifo = false;
           }
       }
       else
       {
           if ( lastWarpTime < currentTime - 1.0 )
           {
               removedSchedFifo = true;
           }
       }

        warpToScreen( vsync,m_screen.isFrontBuffer() ? spAsyncFrontBufferPortrait
                                 : spAsyncSwappedBufferPortrait);
    }

    warpThreadShutdown();

    vInfo( "Exiting WarpThreadLoop()" );
}

/*
* Startup()
*/
VFrameSmooth::VFrameSmooth(bool async,bool wantSingleBuffer)
        : d(new Private(async,wantSingleBuffer))
{

}

/*
* Shutdown()
*/
VFrameSmooth::~VFrameSmooth()
{
    d->destroy();
}

int VFrameSmooth::threadId() const
{
    return d->m_warpThreadTid;
}

/*
* WarpThreadInit()
*/
void VFrameSmooth::Private::warpThreadInit()
{
    vInfo( "WarpThreadInit()" );

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

    //VGlOperation glOperation;
    m_eglStatus.m_context = eglCreateContext( m_eglStatus.m_display, m_eglStatus.m_config, m_eglShareContext, contextAttribs );
    if ( m_eglStatus.m_context == EGL_NO_CONTEXT )
    {
        vFatal( "eglCreateContext failed");
    }
    vInfo( "eglWarpContext: " <<  m_eglStatus.m_context );
    if ( m_contextPriority != EGL_CONTEXT_PRIORITY_MEDIUM_IMG )
    {
        // See what context priority we actually got
        EGLint actualPriorityLevel;
        eglQueryContext( m_eglStatus.m_display, m_eglStatus.m_context, EGL_CONTEXT_PRIORITY_LEVEL_IMG, &actualPriorityLevel );
        switch ( actualPriorityLevel )
        {
        case EGL_CONTEXT_PRIORITY_HIGH_IMG: vInfo( "Context is EGL_CONTEXT_PRIORITY_HIGH_IMG" );
            break;
        case EGL_CONTEXT_PRIORITY_MEDIUM_IMG: vInfo( "Context is EGL_CONTEXT_PRIORITY_MEDIUM_IMG" );
            break;
        case EGL_CONTEXT_PRIORITY_LOW_IMG: vInfo( "Context is EGL_CONTEXT_PRIORITY_LOW_IMG" );
            break;
        default: vInfo( "Context has unknown priority level" );
            break;
        }
    }

    // Make the context current on the window, so no more makeCurrent calls will be needed
    vInfo( "eglMakeCurrent on " <<m_eglMainThreadSurface );
    if ( eglMakeCurrent( m_eglStatus.m_display, m_eglMainThreadSurface,
                         m_eglMainThreadSurface, m_eglStatus.m_context ) == EGL_FALSE )
    {
        vFatal( "eglMakeCurrent failed");
    }

    m_screen.initForCurrentSurface( m_jni, m_wantSingleBuffer,m_buildVersionSDK);

    // create the framework graphics on this thread
    createFrameworkGraphics();

    // Get the linux tid so App can set SCHED_FIFO on it
    m_warpThreadTid = gettid();

    vInfo( "WarpThreadInit() - End" );
}

/*
* WarpThreadShutdown()
*/
void VFrameSmooth::Private::warpThreadShutdown()
{
    vInfo( "WarpThreadShutdown()" );

    // Vertex array objects can only be destroyed on the context they were created on
    destroyFrameworkGraphics();

    ////VGlOperation glOperation;
    // Destroy the sync objects
    for ( int i = 0; i < MAX_WARP_SOURCES; i++ )
    {
        warpSource_t & ws = m_warpSources[i];
        if ( ws.GpuSync )
        {
            if ( EGL_FALSE == m_eglStatus.eglDestroySyncKHR( m_eglStatus.m_display, ws.GpuSync ) )
            {
                vInfo( "eglDestroySyncKHR returned EGL_FALSE" );
            }
            ws.GpuSync = 0;
        }
    }

    // release the window so it can be made current by another thread
    if ( eglMakeCurrent( m_eglStatus.m_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                         EGL_NO_CONTEXT ) == EGL_FALSE )
    {
        vFatal( "eglMakeCurrent: shutdown failed" );
    }

    if ( eglDestroyContext( m_eglStatus.m_display, m_eglStatus.m_context ) == EGL_FALSE )
    {
        vFatal( "eglDestroyContext: shutdown failed" );
    }
    m_eglStatus.m_context = 0;


    vInfo( "WarpThreadShutdown() - End" );
}

const VGlShader & VFrameSmooth::Private::programForParms( const ovrTimeWarpParms & parms, const bool disableChromaticCorrection ) const
{
    int program = VAlgorithm::Clamp( (int)parms.WarpProgram, (int)WP_SIMPLE, (int)WP_PROGRAM_MAX - 1 );

    if ( disableChromaticCorrection && program >= WP_CHROMATIC )
    {
        program -= ( WP_CHROMATIC - WP_SIMPLE );
    }
    return m_warpPrograms[program];
}

void VFrameSmooth::Private::setWarpState( const warpSource_t & currentWarpSource ) const
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
            glDisable( VEglDriver::GL_FRAMEBUFFER_SRGB_EXT );
        }
        else
        {
            glEnable( VEglDriver::GL_FRAMEBUFFER_SRGB_EXT );
        }
    }
    //VGlOperation glOperation;
    m_eglStatus.logErrorsEnum( "SetWarpState" );
}

void VFrameSmooth::Private::bindWarpProgram( const warpSource_t & currentWarpSource,
                                     const VMatrix4f timeWarps[2][2], const VMatrix4f rollingWarp,
                                     const int eye, const double vsyncBase /* for spinner */ ) const
{
    // TODO: bake this into the buffer objects
    const VMatrix4f landscapeOrientationMatrix(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f );

    // Select the warp program.
    const VGlShader & warpProg = programForParms( currentWarpSource.WarpParms, currentWarpSource.disableChromaticCorrection );
    glUseProgram( warpProg.program );

    // Set the shader parameters.
    glUniform1f( warpProg.uniformColor, currentWarpSource.WarpParms.ProgramParms[0] );

    glUniformMatrix4fv( warpProg.uniformModelViewProMatrix, 1, GL_FALSE, landscapeOrientationMatrix.transposed().cell[0] );
    glUniformMatrix4fv( warpProg.uniformTexMatrix, 1, GL_FALSE, timeWarps[0][0].transposed().cell[0] );
    glUniformMatrix4fv( warpProg.uniformTexMatrix2, 1, GL_FALSE, timeWarps[0][1].transposed().cell[0] );
    if ( warpProg.uniformTexMatrix3 > 0 )
    {
        glUniformMatrix4fv( warpProg.uniformTexMatrix3, 1, GL_FALSE, timeWarps[1][0].transposed().cell[0] );
        glUniformMatrix4fv( warpProg.uniformTexMatrix4, 1, GL_FALSE, timeWarps[1][1].transposed().cell[0] );
    }
    if ( warpProg.uniformTexMatrix5 > 0 )
    {
        glUniformMatrix4fv( warpProg.uniformTexMatrix5, 1, GL_FALSE, rollingWarp.transposed().cell[0] );
    }
    if ( warpProg.uniformTexClamp > 0 )
    {
        // split screen clamping for UE4
        const VVect2f clamp( eye * 0.5f, (eye+1)* 0.5f );
        glUniform2fv( warpProg.uniformTexClamp, 1, &clamp.x );
    }
    if ( warpProg.uniformRotateScale > 0 )
    {
        const float angle = framePointTimeInSeconds( vsyncBase ) * M_PI * currentWarpSource.WarpParms.ProgramParms[0];
        const V4Vectf RotateScale( sinf( angle ), cosf( angle ), currentWarpSource.WarpParms.ProgramParms[1], 1.0f );
        glUniform4fv(warpProg.uniformRotateScale, 1, RotateScale.data());
    }
}

void VFrameSmooth::Private::bindCursorProgram() const
{
    // TODO: bake this into the buffer objects
    const VMatrix4f landscapeOrientationMatrix(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f );

    // Select the warp program.
    const VGlShader & warpProg = m_warpPrograms[ WP_SIMPLE ];
    glUseProgram( warpProg.program );

    // Set the shader parameters.
    glUniform1f( warpProg.uniformColor, 1.0f );

    glUniformMatrix4fv( warpProg.uniformModelViewProMatrix, 1, GL_FALSE, landscapeOrientationMatrix.transposed().cell[0] );

    VMatrix4f identity;
    glUniformMatrix4fv(warpProg.uniformTexMatrix, 1, GL_FALSE, identity.data());
    glUniformMatrix4fv(warpProg.uniformTexMatrix2, 1, GL_FALSE, identity.data());
}

int CameraTimeWarpLatency = 4;
bool CameraTimeWarpPause;

static void BindEyeTextures( const warpSource_t & currentWarpSource, const int eye )
{
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, currentWarpSource.WarpParms.Images[eye][0].TexId );
    if ( currentWarpSource.WarpParms.WarpOptions & SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER )
    {
        glTexParameteri( GL_TEXTURE_2D, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_SKIP_DECODE_EXT );
    }
    else
    {
        glTexParameteri( GL_TEXTURE_2D, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_DECODE_EXT );
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
        if ( currentWarpSource.WarpParms.WarpOptions & SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER )
        {
            glTexParameteri( GL_TEXTURE_2D, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_SKIP_DECODE_EXT );
        }
        else
        {
            glTexParameteri( GL_TEXTURE_2D, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_DECODE_EXT );
        }
    }
    if ( currentWarpSource.WarpParms.WarpProgram == WP_MASKED_PLANE_EXTERNAL
         || currentWarpSource.WarpParms.WarpProgram == WP_CHROMATIC_MASKED_PLANE_EXTERNAL
         || currentWarpSource.WarpParms.WarpProgram == WP_CAMERA
         || currentWarpSource.WarpParms.WarpProgram == WP_CHROMATIC_CAMERA )
    {
        glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_EXTERNAL_OES, currentWarpSource.WarpParms.Images[eye][1].TexId );

            if ( currentWarpSource.WarpParms.WarpOptions & SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER )
            {
                glTexParameteri( GL_TEXTURE_EXTERNAL_OES, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_SKIP_DECODE_EXT );
            }
            else
            {
                glTexParameteri( GL_TEXTURE_EXTERNAL_OES, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_DECODE_EXT );
            }
    }
    if ( currentWarpSource.WarpParms.WarpProgram == WP_MASKED_CUBE || currentWarpSource.WarpParms.WarpProgram == WP_CHROMATIC_MASKED_CUBE )
    {
        glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_CUBE_MAP, currentWarpSource.WarpParms.Images[eye][1].TexId );
            if ( currentWarpSource.WarpParms.WarpOptions & SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER )
            {
                glTexParameteri( GL_TEXTURE_CUBE_MAP, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_SKIP_DECODE_EXT );
            }
            else
            {
                glTexParameteri( GL_TEXTURE_CUBE_MAP, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_DECODE_EXT );
            }
    }

    if ( currentWarpSource.WarpParms.WarpProgram == WP_CUBE || currentWarpSource.WarpParms.WarpProgram == WP_CHROMATIC_CUBE )
    {
        for ( int i = 0; i < 3; i++ )
        {
            glActiveTexture( GL_TEXTURE1 + i );
            glBindTexture( GL_TEXTURE_CUBE_MAP, currentWarpSource.WarpParms.Images[eye][1].PlanarTexId[i] );
                if ( currentWarpSource.WarpParms.WarpOptions & SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER )
                {
                    glTexParameteri( GL_TEXTURE_CUBE_MAP, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_SKIP_DECODE_EXT );
                }
                else
                {
                    glTexParameteri( GL_TEXTURE_CUBE_MAP, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_DECODE_EXT );
                }
        }
    }

    if ( currentWarpSource.WarpParms.WarpProgram == WP_LOADING_ICON || currentWarpSource.WarpParms.WarpProgram == WP_CHROMATIC_LOADING_ICON )
    {
        glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_2D, currentWarpSource.WarpParms.Images[eye][1].TexId );
            if ( currentWarpSource.WarpParms.WarpOptions & SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER )
            {
                glTexParameteri( GL_TEXTURE_2D, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_SKIP_DECODE_EXT );
            }
            else
            {
                glTexParameteri( GL_TEXTURE_2D, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_DECODE_EXT );
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
 * Calls getFractionalVsync() multiple times, but this only calls kernel time functions, not java
 * Calls sleepUntilTimePoint() for each eye.
 * May write to the log
 * Writes eyeLog[]
 * Reads warpSources
 * Reads eyeBufferCount
 * May lock and unlock swapMutex
 * May signal swapIsLatched
 * Writes SwapVsync
 *
 */
void VFrameSmooth::Private::warpToScreen( const double vsyncBase_, const swapProgram_t & swap )
{
    static double lastReportTime = 0;
    const double timeNow = floor( VTimer::Seconds());
    if ( timeNow > lastReportTime )
    {
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

    // This will only be updated in SCREENEYE_LEFT
    warpSource_t currentWarpSource;

    int screenWidth, screenHeight;
    m_screen.getScreenResolution( screenWidth, screenHeight );
    glViewport( 0, 0, screenWidth, screenHeight );
    glScissor( 0, 0, screenWidth, screenHeight );

    ////VGlOperation glOperation;
    // Warp each eye to the display surface
    for ( int eye = 0; eye <= 1; ++eye )
    {
        vInfo( "Eye" << eye <<": now= " << getFractionalVsync() << " sleepTo= " << vsyncBase + swap.deltaVsync[eye] );

        // Sleep until we are in the correct half of the screen for
        // rendering this eye.  If we are running single threaded,
        // the first eye will probably already be past the sleep point,
        // so only the second eye will be at a dependable time.
        const double sleepTargetVsync = vsyncBase + swap.deltaVsync[eye];
        const double sleepTargetTime = framePointTimeInSeconds( sleepTargetVsync );
        sleepUntilTimePoint( sleepTargetTime, false );
//        const double preFinish = VTimer::Seconds();

       vInfo( "Vsync " << vsyncBase <<":" << eye << "sleep" << sleepTargetTime);

        // Check for availability of updated eye renderings
        // now that we are about to render.
        long long thisEyeBufferNum = 0;
        int	back;

        if ( eye == 0 )
        {
            const long long latestEyeBufferNum = m_eyeBufferCount.state();
            for ( back = 0; back < MAX_WARP_SOURCES - 1; back++ )
            {
                thisEyeBufferNum = latestEyeBufferNum - back;
                if ( thisEyeBufferNum <= 0 )
                {
                    // just starting, and we don't have any eye buffers to use
                    vInfo( "WarpToScreen: No valid Eye Buffers" );
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
                    vInfo( "thisEyeBufferNum " <<thisEyeBufferNum << "had 0 sync");
                    break;
                }

//                if ( VQuatf( testWarpSource.WarpParms.Images[eye][0].Pose.Pose.Orientation ).LengthSq() < 1e-18f )
//                {
//                    //VInfo( "Bad Pose.Orientation in bufferNum %lli!", thisEyeBufferNum );
//                    break;
//                }

                if (testWarpSource.WarpParms.Images[eye][0].Pose.LengthSq() < 1e-18f) {
                    vInfo("Bad Pose.Orientation in bufferNum " << thisEyeBufferNum << "!");
                    break;
                }


                const EGLint wait = m_eglStatus.eglClientWaitSyncKHR( m_eglStatus.m_display, testWarpSource.GpuSync,
                                                                      EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, 0 );
                if ( wait == EGL_TIMEOUT_EXPIRED_KHR )
                {
                    continue;
                }
                if ( wait == EGL_FALSE )
                {
                    vInfo( "eglClientWaitSyncKHR returned EGL_FALSE" );
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

            if ( currentWarpSource.WarpParms.Images[eye][0].TexId == 0 )
            {
                // We don't have anything valid to draw, so just sleep until
                // the next time point and check again.
                vInfo( "WarpToScreen: Nothing valid to draw" );
                sleepUntilTimePoint( framePointTimeInSeconds( sleepTargetVsync + 1.0f ), false );
                break;
            }
        }

        // Build up the external velocity transform
        VMatrix4f velocity;
        const int velocitySteps = std::min( 3, (int)((long long)vsyncBase - currentWarpSource.MinimumVsync) );
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
        VMatrix4f timeWarps[2][2];
        VRotationState sensor[2];
        for ( int scan = 0; scan < 2; scan++ )
        {
            const double vsyncPoint = vsyncBase + swap.predictionPoints[eye][scan];
            const double timePoint = framePointTimeInSeconds( vsyncPoint );
            sensor[scan] = VRotationSensor::instance()->predictState( timePoint );
            const VMatrix4f warp = CalculateTimeWarpMatrix2(
                        currentWarpSource.WarpParms.Images[eye][0].Pose,
                    sensor[scan] ) * velocity;
            timeWarps[0][scan] = VMatrix4f( currentWarpSource.WarpParms.Images[eye][0].TexCoordsFromTanAngles ) * warp;
            if ( dualLayer )
            {
                if ( currentWarpSource.WarpParms.WarpOptions & SWAP_OPTION_FIXED_OVERLAY )
                {	// locked-to-face HUD
                    timeWarps[1][scan] = VMatrix4f( currentWarpSource.WarpParms.Images[eye][1].TexCoordsFromTanAngles );
                }
                else
                {	// locked-to-world surface
                    const VMatrix4f warp2 = CalculateTimeWarpMatrix2(
                                currentWarpSource.WarpParms.Images[eye][1].Pose,
                            sensor[scan]) * velocity;
                    timeWarps[1][scan] = VMatrix4f( currentWarpSource.WarpParms.Images[eye][1].TexCoordsFromTanAngles ) * warp2;
                }
            }
        }

        // The pass through camera support needs to know the warping from the head motion
        // across the display scan independent of any layers, which may drop frames.
        const VMatrix4f rollingWarp = CalculateTimeWarpMatrix2(
                    sensor[0],
                sensor[1]);

        //---------------------------------------------------------
        // Warp a latched buffer to the screen
        //---------------------------------------------------------

        setWarpState( currentWarpSource );

        bindWarpProgram( currentWarpSource, timeWarps, rollingWarp, eye, vsyncBase );

        BindEyeTextures( currentWarpSource, eye );

        m_screen.beginDirectRendering( eye * screenWidth/2, 0, screenWidth/2, screenHeight );

        // Draw the warp triangles.
        VEglDriver::glBindVertexArrayOES( m_warpMesh.vertexArrayObject );
        const int indexCount = m_warpMesh.indexCount / 2;
        const int indexOffset = eye * indexCount;

        EGLDisplay display = eglGetCurrentDisplay();
        EGLSurface windowsurface = eglGetCurrentSurface(EGL_DRAW);
        EGLint res;

        eglQuerySurface(display, windowsurface, EGL_RENDER_BUFFER, &res);

        if (res == EGL_SINGLE_BUFFER)
		{
			vInfo("single buffer is used!");
		}
		else if (res == EGL_BACK_BUFFER)
		{
			vInfo("back buffer is used!");
		}
		else
		{
			vInfo("error no buffer is used!");
		}

        glDrawElements( GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, (void *)(indexOffset * 2 ) );

        // If the gaze cursor is enabled, render those subsets of triangles
        if ( currentWarpSource.WarpParms.WarpOptions & SWAP_OPTION_SHOW_CURSOR )
        {
            bindCursorProgram();
            glEnable( GL_BLEND );
            VEglDriver::glBindVertexArrayOES( m_cursorMesh.vertexArrayObject );
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
        drawFrameworkGraphicsToWindow( eye, currentWarpSource.WarpParms.WarpOptions);

        m_screen.endDirectRendering();

        if (m_screen.isFrontBuffer())
        {
            m_eglStatus.glFinish();
        }

        const double justBeforeFinish = VTimer::Seconds();
        const double postFinish = VTimer::Seconds();

        const float latency = postFinish - justBeforeFinish;
        if ( latency > 0.008f )
        {
            vInfo( "Frame " << (int)vsyncBase<< " Eye "<<eye<< "latency "<< latency);
        }
    }	// for eye

    UnbindEyeTextures();

    glUseProgram( 0 );

    VEglDriver::glBindVertexArrayOES( 0 );

    if ( !m_screen.isFrontBuffer() )
    {
        m_screen.swapBuffers();
    }
}

void VFrameSmooth::Private::warpToScreenSliced( const double vsyncBase, const swapProgram_t & swap )
{
    NV_UNUSED(swap);
    // Fetch vsync timing information once, so we don't have to worry
    // about it changing slightly inside a given frame.
    const VsyncState vsyncState = UpdatedVsyncState.state();
    if ( vsyncState.vsyncBaseNano == 0 )
    {
        return;
    }

    ////VGlOperation glOperation;
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

    int screenWidth, screenHeight;
    m_screen.getScreenResolution( screenWidth, screenHeight );
    glViewport( 0, 0, screenWidth, screenHeight );
    glScissor( 0, 0, screenWidth, screenHeight );

    // This must be long enough to cover CPU scheduling delays, GPU in-flight commands,
    // and the actual drawing of this slice.
    const warpSource_t & latestWarpSource = m_warpSources[m_eyeBufferCount.state()%MAX_WARP_SOURCES];
    const double schedulingCushion = latestWarpSource.WarpParms.PreScheduleSeconds;

    //LOG( "now %fv(%i) %f cush %f", getFractionalVsync(), (int)vsyncBase, VTimer::Seconds(), schedulingCushion );

    // Warp each slice to the display surface
    warpSource_t currentWarpSource = {};
    int	back = 0;	// frame back from most recent
    long long thisEyeBufferNum = 0;
    for ( int screenSlice = 0; screenSlice < NUM_SLICES_PER_SCREEN; screenSlice++ )
    {
        const int	eye = (int)( screenSlice / NUM_SLICES_PER_EYE );

        // Sleep until we are in the correct part of the screen for
        // rendering this slice.
        const double sleepTargetTime = sliceTimes[ screenSlice ] - schedulingCushion;
        sleepUntilTimePoint( sleepTargetTime, false );
        //const double preFinish = VTimer::Seconds();

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
                    vInfo( "WarpToScreen: No valid Eye Buffers" );
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
                    vInfo( "thisEyeBufferNum " << thisEyeBufferNum <<" had 0 sync" );
                    break;
                }

//                if ( VQuatf( testWarpSource.WarpParms.Images[eye][0].Pose.Pose.Orientation ).LengthSq() < 1e-18f )
//                {
//                    //VInfo( "Bad Predicted.Pose.Orientation!" );
//                    continue;
//                }

                if (testWarpSource.WarpParms.Images[eye][0].Pose.LengthSq() < 1e-18f) {
                    vInfo("Bad Pose.Orientation in bufferNum " << thisEyeBufferNum << "!");
                    break;
                }

                const EGLint wait = m_eglStatus.eglClientWaitSyncKHR( m_eglStatus.m_display, testWarpSource.GpuSync,
                                                                      EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, 0 );
                if ( wait == EGL_TIMEOUT_EXPIRED_KHR )
                {
                    continue;
                }
                if ( wait == EGL_FALSE )
                {
                    vInfo( "eglClientWaitSyncKHR returned EGL_FALSE" );
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

            if ( currentWarpSource.WarpParms.Images[eye][0].TexId == 0 )
            {
                // We don't have anything valid to draw, so just sleep until
                // the next time point and check again.
                vInfo( "WarpToScreen: Nothing valid to draw" );
                sleepUntilTimePoint( framePointTimeInSeconds( vsyncBase + 1.0f ), false );
                break;
            }
        }

        // Build up the external velocity transform
        VMatrix4f velocity;
        const int velocitySteps = std::min( 3, (int)((long long)vsyncBase - currentWarpSource.MinimumVsync) );
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
        VMatrix4f timeWarps[2][2];
        static VRotationState sensor[2];
        for ( int scan = 0; scan < 2; scan++ )
        {
            // We always make a new prediciton for the end of the slice,
            // but we only make a new one for the start of the slice when a
            // new eye has just started, otherwise we could get a visible
            // seam at the slice boundary when the prediction changed.
            static VMatrix4f	warp;
            if ( scan == 1 || screenSlice == 0 || screenSlice == NUM_SLICES_PER_EYE )
            {
                // SliceTimes should be the actual time the pixels hit the screen,
                // but we may want a slight adjustment on the prediction time.
                const double timePoint = sliceTimes[screenSlice + scan];
                sensor[scan] = VRotationSensor::instance()->predictState( timePoint );
                warp = CalculateTimeWarpMatrix2(
                            currentWarpSource.WarpParms.Images[eye][0].Pose,
                        sensor[scan]) * velocity;
            }
            timeWarps[0][scan] = VMatrix4f( currentWarpSource.WarpParms.Images[eye][0].TexCoordsFromTanAngles ) * warp;
            if ( dualLayer )
            {
                if ( currentWarpSource.WarpParms.WarpOptions & SWAP_OPTION_FIXED_OVERLAY )
                {	// locked-to-face HUD
                    timeWarps[1][scan] = VMatrix4f( currentWarpSource.WarpParms.Images[eye][1].TexCoordsFromTanAngles );
                }
                else
                {	// locked-to-world surface
                    const VMatrix4f warp2 = CalculateTimeWarpMatrix2(
                                currentWarpSource.WarpParms.Images[eye][1].Pose,
                            sensor[scan]) * velocity;
                    timeWarps[1][scan] = VMatrix4f( currentWarpSource.WarpParms.Images[eye][1].TexCoordsFromTanAngles ) * warp2;
                }
            }
        }
        // The pass through camera support needs to know the warping from the head motion
        // across the display scan independent of any layers, which may drop frames.
        const VMatrix4f rollingWarp = CalculateTimeWarpMatrix2(
                    sensor[0],
                sensor[1]);

        //---------------------------------------------------------
        // Warp a latched buffer to the screen
        //---------------------------------------------------------

        setWarpState( currentWarpSource );

        bindWarpProgram( currentWarpSource, timeWarps, rollingWarp, eye, vsyncBase );

        if ( screenSlice == 0 || screenSlice == NUM_SLICES_PER_EYE )
        {
            BindEyeTextures( currentWarpSource, eye );
        }

        const int sliceSize = screenWidth  / NUM_SLICES_PER_SCREEN;

        m_screen.beginDirectRendering( sliceSize*screenSlice, 0, sliceSize, screenHeight );

        // Draw the warp triangles.
        const VGlGeometry & mesh = m_sliceMesh;
        VEglDriver::glBindVertexArrayOES( mesh.vertexArrayObject );
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
        drawFrameworkGraphicsToWindow( eye, currentWarpSource.WarpParms.WarpOptions);

         m_screen.endDirectRendering();
        if (m_screen.isFrontBuffer() &&
            ( screenSlice == 1 * 4 - 1 || screenSlice == 2 * 4 - 1 ) )
        {
            m_eglStatus.glFinish();
        }

        const double justBeforeFinish = VTimer::Seconds();
        const double postFinish = VTimer::Seconds();

        const float latency = postFinish - justBeforeFinish;
        if ( latency > 0.008f )
        {
            //VInfo( "Frame %i Eye %i latency %5.3f", (int)vsyncBase, eye, latency );
        }
    }	// for screenSlice

    UnbindEyeTextures();

    glUseProgram( 0 );

    VEglDriver::glBindVertexArrayOES( 0 );

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

void VFrameSmooth::Private::warpSwapInternal( const ovrTimeWarpParms & parms )
{
    if ( gettid() != m_sStartupTid )
    {
        vFatal( "WarpSwap: Called with tid  instead of ");
    }

    // Keep track of the last time WarpSwap() was called.
    m_lastWarpSwapTimeInSeconds.setState( VTimer::Seconds());

    // Explicitly bind the framebuffer back to the default, so the
    // eye targets will not be bound while they might be used as warp sources.
    glBindFramebuffer( GL_FRAMEBUFFER, 0 );
    const int minimumVsyncs =  parms.MinimumVsyncs;

    ////VGlOperation glOperation;
    // Prepare to pass the new eye buffers to the background thread if we are running multi-threaded.
    const long long lastBufferCount = m_eyeBufferCount.state();
    warpSource_t & ws = m_warpSources[ ( lastBufferCount + 1 ) % MAX_WARP_SOURCES ];
    ws.MinimumVsync = m_lastSwapVsyncCount + 2 * minimumVsyncs;	// don't use it if from same frame to avoid problems with very fast frames
    ws.FirstDisplayedVsync[0] = 0;			// will be set when it becomes the currentSource
    ws.FirstDisplayedVsync[1] = 0;			// will be set when it becomes the currentSource
    ws.disableChromaticCorrection = ( ( m_eglStatus.eglGetGpuType() & VEglDriver::GPU_TYPE_MALI_T760_EXYNOS_5433 ) != 0 );
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
        if ( EGL_FALSE == m_eglStatus.eglDestroySyncKHR( m_eglStatus.m_display, ws.GpuSync ) )
        {
            vInfo( "eglDestroySyncKHR returned EGL_FALSE" );
        }
    }

    // Add a sync object for this buffer set.
    ws.GpuSync = m_eglStatus.eglCreateSyncKHR( m_eglStatus.m_display, EGL_SYNC_FENCE_KHR, NULL );
    if ( ws.GpuSync == EGL_NO_SYNC_KHR )
    {
        vFatal( "eglCreateSyncKHR_():EGL_NO_SYNC_KHR" );
    }

    // Force it to flush the commands
    if ( EGL_FALSE == m_eglStatus.eglClientWaitSyncKHR( m_eglStatus.m_display, ws.GpuSync,
                                                        EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, 0 ) )
    {
        vInfo( "eglClientWaitSyncKHR returned EGL_FALSE" );
    }

    // Submit this buffer set for use by the VFrameSmooth thread
    //	LOG( "submitting bufferNum %lli: %i %i", lastBufferCount+1,
    //			ws.WarpParms.Images[0][0].TexId, ws.WarpParms.Images[1][0].TexId );
    m_eyeBufferCount.setState( lastBufferCount + 1 );

    // If we are running synchronously instead of using a background
    // thread, call WarpToScreen() directly.
    if ( !m_async )
    {
        // Make sure all eye drawing is completed before we warp the drawing
        // to the display buffer.

        ////VGlOperation glOperation;
        m_eglStatus.glFinish();

        swapProgram_t * swapProg;
        swapProg = &spSyncSwappedBufferPortrait;

        warpToScreen( floor( getFractionalVsync() ), *swapProg );

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
            m_lastSwapVsyncCount = std::max( state.VsyncCount, m_lastSwapVsyncCount + minimumVsyncs );

            // Sleep for at least one millisecond to make sure the main VR thread
            // cannot completely deny the Android watchdog from getting a time slice.
            const uint64_t suspendNanoSeconds = endSuspendNanoSeconds - startSuspendNanoSeconds;
            if ( suspendNanoSeconds < 1000 * 1000 )
            {
                const uint64_t suspendMicroSeconds = ( 1000 * 1000 - suspendNanoSeconds ) / 1000;
                vInfo( "WarpSwap: usleep( "<<suspendMicroSeconds << " )");
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
void VFrameSmooth::doSmooth( const ovrTimeWarpParms & parms )
{
    const int count = ( ( parms.WarpOptions & SWAP_OPTION_FLUSH ) != 0 ) ? 3 : 1;
    for ( int i = 0; i < count; i++ )
    {
        d->warpSwapInternal( parms );
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
    ////VGlOperation glOperation;

    VEglDriver::glGenVertexArraysOES( 1, &geo.vertexArrayObject );
    VEglDriver::glBindVertexArrayOES( geo.vertexArrayObject );

    lineVert_t	* verts = new lineVert_t[lineVertCount];
    const int byteCount = lineVertCount * sizeof( verts[0] );
    memset( verts, 0, byteCount );

    glGenBuffers( 1, &geo.vertexBuffer );
    glBindBuffer( GL_ARRAY_BUFFER, geo.vertexBuffer );
    glBufferData( GL_ARRAY_BUFFER, byteCount, (void *) verts, GL_DYNAMIC_DRAW );

    glEnableVertexAttribArray( VERTEX_POSITION );
    glVertexAttribPointer( VERTEX_POSITION, 2, GL_SHORT, false, sizeof( lineVert_t ), (void *)0 );

    glEnableVertexAttribArray( VERTEX_COLOR );
    glVertexAttribPointer( VERTEX_COLOR, 4, GL_UNSIGNED_BYTE, true, sizeof( lineVert_t ), (void *)4 );

    delete[] verts;

    // these will be drawn with DrawArrays, so no index buffer is needed

    geo.indexCount = lineVertCount;

    VEglDriver::glBindVertexArrayOES( 0 );

    return geo;
}


float calibrateFovScale = 1.0f;	// for interactive tweaking

// VAO (and FBO) are not shared across different contexts, so this needs
// to be called by the thread that will be drawing the warp.
void VFrameSmooth::Private::createFrameworkGraphics()
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
//    glGenTextures( 1, &m_defaultLoadingIconTexId );
//    glBindTexture( GL_TEXTURE_2D, m_defaultLoadingIconTexId );
//    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, oculus_loading_indicator_width, oculus_loading_indicator_height,
//                  0, GL_RGBA, GL_UNSIGNED_BYTE, oculus_loading_indicator_bufferData );
//    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
//    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
//    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
//    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
//    glBindTexture( GL_TEXTURE_2D, 0 );

    // single slice mesh for the normal rendering
    m_warpMesh = VLensDistortion::createDistortionGrid( m_device, 1, calibrateFovScale, false );

    // multi-slice mesh for sliced rendering
    m_sliceMesh = VLensDistortion::createDistortionGrid( m_device, NUM_SLICES_PER_EYE, calibrateFovScale, false );

    // small subset cursor mesh
    m_cursorMesh = VLensDistortion::createDistortionGrid( m_device, 1, calibrateFovScale, true );

    if ( m_warpMesh.indexCount == 0 || m_sliceMesh.indexCount == 0 )
    {
        vFatal( "WarpMesh failed to load");
    }


    m_timingGraph = CreateTimingGraphGeometry( (256+10)*2 );


    m_calibrationLines2.createCalibrationGrid( 0, false );

    m_untexturedMvpProgram.initShader(VGlShader::getUntextureMvpVertexShaderSource(),VGlShader::getUntexturedFragmentShaderSource());
    m_debugLineProgram.initShader(VGlShader::getVertexColorVertexShaderSource(),VGlShader::getUntexturedFragmentShaderSource());
    buildWarpProgs();
}

void VFrameSmooth::Private::destroyFrameworkGraphics()
{
    glDeleteTextures( 1, &m_blackTexId );
    glDeleteTextures( 1, &m_defaultLoadingIconTexId );

    m_calibrationLines2.destroy();
    m_warpMesh.destroy();
    m_sliceMesh.destroy();
    m_cursorMesh.destroy();
    m_timingGraph.destroy();

    m_untexturedMvpProgram.destroy();
    m_debugLineProgram.destroy();

    for ( int i = 0; i < WP_PROGRAM_MAX; i++ )
    {
        m_warpPrograms[i].destroy();
    }
}

// Assumes viewport and scissor is set for the eye already.
// Assumes there is no depth buffer for the window.
void VFrameSmooth::Private::drawFrameworkGraphicsToWindow( const int eye,
                                                   const int swapOptions)
{
    ////VGlOperation glOperation;
    // Latency tester support.
//    unsigned char latencyTesterColorToDisplay[3];

//    if ( ovr_ProcessLatencyTest( latencyTesterColorToDisplay ) )
//    {
//        glClearColor(
//                    latencyTesterColorToDisplay[0] / 255.0f,
//                latencyTesterColorToDisplay[1] / 255.0f,
//                latencyTesterColorToDisplay[2] / 255.0f,
//                1.0f );
//        glClear( GL_COLOR_BUFFER_BIT );
//    }

    // Report latency tester results to the log.
//    const char * results = ovr_GetLatencyTestResult();
//    if ( results != NULL )
//    {
//        //VInfo( "LATENCY TESTER: %s", results );
//    }

    // optionally draw the calibration lines
    if ( swapOptions & SWAP_OPTION_DRAW_CALIBRATION_LINES )
    {
        const float znear = 0.5f;
        const float zfar = 150.0f;
        // flipped for portrait mode
        const VMatrix4f projectionMatrix(
                    0, 1, 0, 0,
                    -1, 0, 0, 0,
                    0, 0, zfar / (znear - zfar), (zfar * znear) / (znear - zfar),
                    0, 0, -1, 0 );
        glUseProgram( m_untexturedMvpProgram.program );
        glLineWidth( 2.0f );
        glUniform4f( m_untexturedMvpProgram.uniformColor, 1, 0, 0, 1 );
        glUniformMatrix4fv( m_untexturedMvpProgram.uniformModelViewProMatrix, 1, GL_FALSE,  // not transposed
                            projectionMatrix.transposed().cell[0] );
        VEglDriver::glBindVertexArrayOES( m_calibrationLines2.vertexArrayObject );

        int width, height;
        m_screen.getScreenResolution( width, height );
        glViewport( width /2 * (int)eye, 0, width /2, height );
        glDrawElements( GL_LINES, m_calibrationLines2.indexCount, GL_UNSIGNED_SHORT, NULL );
        glViewport( 0, 0, width , height );
    }
}

void VFrameSmooth::Private::buildWarpProgPair( VrKernelProgram simpleIndex,
                                       const char * simpleVertex, const char * simpleFragment,
                                       const char * chromaticVertex, const char * chromaticFragment
)
{
    m_warpPrograms[ simpleIndex ] = VGlShader( simpleVertex, simpleFragment );
    m_warpPrograms[ simpleIndex + ( WP_CHROMATIC - WP_SIMPLE ) ] = VGlShader( chromaticVertex, chromaticFragment );
}

void VFrameSmooth::Private::buildWarpProgMatchedPair( VrKernelProgram simpleIndex,
                                              const char * simpleVertex, const char * simpleFragment
)
{
    m_warpPrograms[ simpleIndex ] = VGlShader( simpleVertex, simpleFragment );
    m_warpPrograms[ simpleIndex + ( WP_CHROMATIC - WP_SIMPLE ) ] = VGlShader( simpleVertex, simpleFragment );
}


void VFrameSmooth::Private::buildWarpProgs()
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

