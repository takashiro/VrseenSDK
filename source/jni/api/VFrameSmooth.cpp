#include <errno.h>
#include <math.h>
#include <sched.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "android/JniUtils.h"

#include "VFrameSmooth.h"
#include "VAlgorithm.h"
#include "VDevice.h"
#include "VFrameSmooth.h"
#include "VLensDistortion.h"
#include "VEglDriver.h"
#include "VString.h"
#include "VZipFile.h"
#include "VLockless.h"
#include "VTimer.h"
#include "VGlGeometry.h"
#include "VGlShader.h"
#include "VKernel.h"
#include "VRotationSensor.h"
#include "VDirectRender.h"
#include "App.h"

//TODO: Remove the hacking extern variable
extern jclass VrLibClass;

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

struct swapProgram_t
{

    bool	singleThread;

    bool	dualMonoDisplay;


    float	deltaVsync[2];

    float	predictionPoints[2][2];
};


struct SwapState
{
    SwapState() : VsyncCount(0),EyeBufferCount(0) {}
    long long		VsyncCount;
    long long		EyeBufferCount;
};

struct eyeLog_t
{

    bool		skipped;

    int			bufferNum;

    float		issueFinish;
    float		completeFinish;

    float		poseLatencySeconds;
};

swapProgram_t	spAsyncFrontBufferPortrait = {
        false,	false, { 0.5, 1.0},	{ {1.0, 1.5}, {1.5, 2.0} }
};

swapProgram_t	spAsyncSwappedBufferPortrait = {
    false,	false, { 0.0, 0.5},	{ {1.0, 1.5}, {1.5, 2.0} }
};

swapProgram_t	spSyncFrontBufferPortrait = {
    true,	false, { 0.5, 1.0},	{ {1.0, 1.5}, {1.5, 2.0} }
};

swapProgram_t	spSyncSwappedBufferPortrait = {
    true,	false, { 0.0, 0.0},	{ {2.0, 2.5}, {2.5, 3.0} }
};



void EyeRectLandscape( const VDevice *device,int eye,int &x, int &y, int &width, int &height )
{

    int scissorX = ( eye == 0 ) ? 0 : device->widthbyPixels / 2;
    int scissorY = 0;
    int scissorWidth = device->widthbyPixels / 2;
    int scissorHeight = device->heightbyPixels;
    x = scissorX;
    y = scissorY;
    width = scissorWidth;
    height = scissorHeight;
    return;    
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

VR4Matrixf CalculateTimeWarpMatrix2( const VQuatf &inFrom, const VQuatf &inTo )
{    
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
            from = VQuatf( 0.0f, 0.0f, 0.0f, 1.0f );
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
            to = VQuatf( 0.0f, 0.0f, 0.0f, 1.0f );
        }
    }

    VR4Matrixf lastSensorMatrix(to);
    VR4Matrixf lastViewMatrix(from);

    return ( lastSensorMatrix.Inverted() * lastViewMatrix ).Inverted();
}

static bool IsContextPriorityExtensionPresent()
{
    EGLint currentPriorityLevel = -1;
    if ( !eglQueryContext( eglGetCurrentDisplay(), eglGetCurrentContext(), EGL_CONTEXT_PRIORITY_LEVEL_IMG, &currentPriorityLevel )
         || currentPriorityLevel == -1 )
    {

        return false;
    }
    return true;
}


extern "C"
{

    void Java_com_vrseen_nervgear_VrLib_nativeVsync( JNIEnv *jni, jclass clazz, jlong frameTimeNanos )
    {

        VsyncState	state = UpdatedVsyncState.state();

        // Round this, because different phone models have slightly different periods.
        state.vsyncCount += floor( 0.5 + ( frameTimeNanos - state.vsyncBaseNano ) / state.vsyncPeriodNano );
        state.vsyncPeriodNano = 1e9 / 60.0;
        state.vsyncBaseNano = frameTimeNanos;

        UpdatedVsyncState.setState( state );
    }
}


struct VFrameSmooth::Private
{
    Private(bool async, bool wantSingleBuffer):
            m_untexturedMvpProgram(),
            m_debugLineProgram(),
            m_warpPrograms(),
            m_blackTexId( 0 ),
            m_defaultLoadingIconTexId( 0 ),
            m_wantSingleBuffer(wantSingleBuffer),
            m_hasEXT_sRGB_write_control( false ),
            m_sStartupTid( 0 ),
            m_jni(nullptr),
            m_eglMainThreadSurface( 0 ),
            m_eglClientVersion( 0 ),
            m_eglShareContext( 0 ),
            m_contextPriority( 0 ),
            m_eyeLog(),
            m_lastEyeLog( 0 ),

            m_smoothThread( 0 ),
            m_smoothThreadTid( 0 ),
            m_lastSwapVsyncCount( 0 )
    {

        static_assert( ( VK_MAX & 1 ) == 0, "WP_PROGRAM_MAX" );
        static_assert( ( VK_DEFAULT_CB - VK_DEFAULT ) ==
                             ( VK_MAX - VK_DEFAULT_CB ) , "WP_CHROMATIC");

        m_shutdownRequest.setState( false );
        m_eyeBufferCount.setState( 0 );
        memset( m_warpPrograms, 0, sizeof( m_warpPrograms ) );

        pthread_mutex_init( &m_smoothMutex, NULL /* default attributes */ );
        pthread_cond_init( &m_smoothIslocked, NULL /* default attributes */ );
        vInfo("-------------------- VFrameSmooth::Private() --------------------");

        m_sStartupTid = gettid();
        m_lastsmoothTimeInSeconds.setState( VTimer::Seconds() );
        m_smoothThread = 0;
        m_async = async;
        m_device = VDevice::instance();
        m_eyeBufferCount.setState( 0 );
// 初始化smooth线程的gl状态
        m_eglStatus.updateDisplay();

        m_eglMainThreadSurface = eglGetCurrentSurface( EGL_DRAW );
        if ( m_eglMainThreadSurface == EGL_NO_SURFACE )
        {
            vFatal("EGL_NO_SURFACE");
        }
        m_eglShareContext = eglGetCurrentContext();
        if ( m_eglShareContext == EGL_NO_CONTEXT )
        {
            vFatal("EGL_NO_CONTEXT");
        }
      //  m_eglStatus.updateEglConfig(m_eglShareContext);



        EGLint configID;
        if ( !eglQueryContext( m_eglStatus.m_display, m_eglShareContext, EGL_CONFIG_ID, &configID ) )
        {
            vFatal("eglQueryContext EGL_CONFIG_ID failed");
        }
        m_eglStatus.m_config = m_eglStatus.eglConfigForConfigID( configID );
        if (  m_eglStatus.m_config == NULL )
        {
            vFatal("EglConfigForConfigID failed");
        }
        if ( !eglQueryContext( m_eglStatus.m_display, m_eglShareContext, EGL_CONTEXT_CLIENT_VERSION, (EGLint *)&m_eglClientVersion ) )
        {
            vFatal("eglQueryContext EGL_CONTEXT_CLIENT_VERSION failed");
        }

        vInfo("Current EGL_CONTEXT_CLIENT_VERSION: " << m_eglClientVersion);

        EGLint depthSize = 0;
        eglGetConfigAttrib( m_eglStatus.m_display, m_eglStatus.m_config, EGL_DEPTH_SIZE, &depthSize );
        if ( depthSize != 0 )
        {
            vWarn("Share context eglConfig has " << depthSize << " depth bits. But it should be 0.");
        }
        EGLint samples = 0;
        eglGetConfigAttrib( m_eglStatus.m_display, m_eglStatus.m_config, EGL_SAMPLES, &samples );
        if ( samples != 0 )
        {

            vInfo("Share context eglConfig has " << samples << " samples. But it should be 0");
        }

        m_buildVersionSDK = VKernel::instance()->getBuildVersion();

        if ( !m_async )
        {
            m_screen.initForCurrentSurface( m_jni, m_wantSingleBuffer,m_buildVersionSDK);
            createFrameworkGraphics();
            vInfo("Skipping thread setup because m_async is false.");
        }
        else
        {
            //---------------------------------------------------------
            // Thread initialization
            //---------------------------------------------------------

            if ( IsContextPriorityExtensionPresent() )
            {
                vInfo("Requesting EGL_CONTEXT_PRIORITY_HIGH_IMG.");
                m_contextPriority = EGL_CONTEXT_PRIORITY_HIGH_IMG;
            }
            else
            {

                vInfo("IMG_Context_Priority is not present.");

                m_contextPriority = EGL_CONTEXT_PRIORITY_MEDIUM_IMG;
            }


            const EGLint attrib_list[] =
                    {
                            EGL_WIDTH, 16,
                            EGL_HEIGHT, 16,
                            EGL_NONE
                    };
            m_eglStatus.m_pbufferSurface = eglCreatePbufferSurface( m_eglStatus.m_display, m_eglStatus.m_config, attrib_list );
            if ( m_eglStatus.m_pbufferSurface == EGL_NO_SURFACE )
            {
                vFatal("eglCreatePbufferSurface failed: " << m_eglStatus.getEglErrorString());
            }

            if ( eglMakeCurrent( m_eglStatus.m_display, m_eglStatus.m_pbufferSurface, m_eglStatus.m_pbufferSurface,
                                 m_eglShareContext ) == EGL_FALSE )
            {
                vFatal("eglMakeCurrent: eglMakeCurrent pbuffer failed");
            }
            m_shutdownRequest.setState( false );
            pthread_mutex_lock( &m_smoothMutex );
            const int createErr = pthread_create( &m_smoothThread, NULL /* default attributes */, &ThreadStarter, this );
            if ( createErr != 0 )
            {
                vFatal("pthread_create returned " << createErr);
            }
            pthread_cond_wait( &m_smoothIslocked, &m_smoothMutex );
            pthread_mutex_unlock( &m_smoothMutex );
        }

        vInfo("----------------- VFrameSmooth::Private() End -----------------");
    }

    void destroy()
    {
    vInfo("---------------- VFrameSmooth::Private::destroy() Start ----------------");
        if ( m_smoothThread != 0 )
        {


            m_shutdownRequest.setState( true );

            void * data;
            pthread_join( m_smoothThread, &data );

            m_smoothThread = 0;




            if ( eglGetCurrentSurface( EGL_DRAW ) != m_eglStatus.m_pbufferSurface)
            {
                vInfo("eglGetCurrentSurface( EGL_DRAW ) != m_eglStatus.m_pbufferSurface.");
            }
            if ( eglMakeCurrent( m_eglStatus.m_display, m_eglMainThreadSurface,
                                 m_eglMainThreadSurface, m_eglShareContext ) == EGL_FALSE)
            {
                vFatal("eglMakeCurrent to window failed: " << m_eglStatus.getEglErrorString());
            }
            if ( EGL_FALSE == eglDestroySurface( m_eglStatus.m_display, m_eglStatus.m_pbufferSurface ) )
            {
                vWarn("Failed to destroy pbuffer.");
            }
            else
            {
                vInfo("Destroyed m_eglStatus.m_pbufferSurface.");
            }
        }
        else
        {
            // Shutdown the front buffer rendering
            m_screen.shutdown();
            destroyFrameworkGraphics();
        }

        vInfo("---------------- VFrameSmooth::Private::destroy() End ----------------");
    }
    static void *	ThreadStarter( void * parm );

    void			threadFunction();
    void 			smoothThreadInit();
    void			smoothThreadShutdown();

    void            smoothInternal();
    void			buildSmoothProgPair( VrKernelProgram simpleIndex,
                                       const char * simpleVertex, const char * simpleFragment,
                                       const char * chromaticVertex, const char * chromaticFragment );

    void			buildSmoothProgMatchedPair( VrKernelProgram simpleIndex,
                                            const char * vertex, const char * fragment );
    void 			buildSmoothProgs();
    void			createFrameworkGraphics();
    void			destroyFrameworkGraphics();
    void			drawFrameworkGraphicsToWindow( const int eye, const int swapOptions);
    //用于管理同步的函数
    double			getFractionalVsync();
    double			framePointTimeInSeconds( const double framePoint ) const;
    float 			sleepUntilTimePoint( const double targetSeconds, const bool busyWait );

    // 平滑的参数：
     bool m_async;
     bool testc;

     unsigned int	m_texId[2][3];
     unsigned int	m_planarTexId[2][3][3];
     VR4Matrixf		m_texMatrix[2][3];
     VRotationState m_pose[2][3];


     int 						m_smoothOptions;
     VR4Matrixf					m_externalVelocity;
     int							m_minimumVsyncs;
     float						m_preScheduleSeconds;
     ushort			m_smoothProgram;
     float						m_programParms[4];



     long long			m_minimumVsync;
     long long			m_firstDisplayedVsync[2];
     bool				m_disableChromaticCorrection;
     EGLSyncKHR			m_gpuSync;
    //平滑参数结束；
    VDevice *m_device;

    VGlShader		m_untexturedMvpProgram;
    VGlShader		m_debugLineProgram;
    VGlShader		m_warpPrograms[ VK_MAX ];
    GLuint			m_blackTexId;
    GLuint			m_defaultLoadingIconTexId;
    VGlGeometry	m_calibrationLines2;		// simple cross
    VGlGeometry	m_smoothMesh;
    VGlGeometry	m_slicesmoothMesh;
    VGlGeometry	m_cursorMesh;

    //single buffer
    DirectRender	m_screen;
    bool m_wantSingleBuffer;
    int m_buildVersionSDK;

    void			renderToDisplay( const double vsyncBase, const swapProgram_t & swap );
    void			renderToDisplayBySliced( const double vsyncBase, const swapProgram_t & swap );

    const VGlShader & chooseProgram(const bool disableChromaticCorrection ) const;
    void			setSmoothpState( ) const;
    void			bindSmoothProgram( const VR4Matrixf timeWarps[2][2],
                                     const VR4Matrixf rollingWarp, const int eye, const double vsyncBase ) const;
    void			bindCursorProgram() const;
    void            bindEyeTextures( const int eye );

    bool			m_hasEXT_sRGB_write_control;


    pid_t			m_sStartupTid;


    JNIEnv *		m_jni;


    VLockless<double>		m_lastsmoothTimeInSeconds;


   // EGLDisplay		m_eglDisplay;

    VEglDriver      m_eglStatus;

    //EGLSurface		m_eglPbufferSurface;
    EGLSurface		m_eglMainThreadSurface;

    EGLint			m_eglClientVersion;
    EGLContext		m_eglShareContext;



    GLuint			m_contextPriority;

    static const int EYE_LOG_COUNT = 512;
    eyeLog_t		m_eyeLog[EYE_LOG_COUNT];
    long long		m_lastEyeLog;





    VLockless<bool>		m_shutdownRequest;

    pthread_t		m_smoothThread;
    int				m_smoothThreadTid;


    pthread_mutex_t m_smoothMutex;
    pthread_cond_t m_smoothIslocked;
    VLockless<longlong> m_eyeBufferCount;


    VLockless<SwapState> m_swapVsync;
    longlong m_lastSwapVsyncCount;
};

void VFrameSmooth::setSmoothEyeTexture(uint texID,ushort eye,ushort layer)
{
    d->m_texId[eye][layer] =  texID;
}

void VFrameSmooth::setTexMatrix(const VR4Matrixf &mtexMatrix, ushort eye, ushort layer)
{
    d->m_texMatrix[eye][layer] =  mtexMatrix;
}

void VFrameSmooth::setSmoothPose(const VRotationState &pose, ushort eye, ushort layer)
{
    d->m_pose[eye][layer] = pose;
}

void VFrameSmooth::setpTex(uint *mpTexId, ushort eye, ushort layer)
{
     d->m_planarTexId[eye][layer][0] = mpTexId[0];
     d->m_planarTexId[eye][layer][1] = mpTexId[1];
     d->m_planarTexId[eye][layer][2] = mpTexId[2];
}

void VFrameSmooth::setSmoothOption(int option)
{
    d->m_smoothOptions = option;

}

void VFrameSmooth::setMinimumVsncs( int vsnc)
{
    d->m_minimumVsyncs = vsnc;
}

void VFrameSmooth::setExternalVelocity(const VR4Matrixf &extV)

{
    for(int i=0;i<4;i++)
        for( int j=0;j<4;j++)
    {
 d->m_externalVelocity.M[i][j] = extV.M[i][j];


}
}
void VFrameSmooth::setPreScheduleSeconds(float pres)
{

    d->m_preScheduleSeconds = pres;

}
void VFrameSmooth::setSmoothProgram(ushort program)
{

   d->m_smoothProgram = program;


}
void VFrameSmooth::setProgramParms( float * proParms)
{
   //d->m_programParms[0] = proParms[0];
  // d->m_programParms[1] = proParms[1];
  // d->m_programParms[2] = proParms[2];
  //  d->m_programParms[3] = proParms[3];
    for(int i=0;i<4;i++)
    {
       d->m_programParms[i]= proParms[i];

    }
}

int VFrameSmooth::threadId() const
{
    return d->m_smoothThreadTid;
}


void *VFrameSmooth::Private::ThreadStarter( void * parm )
{
    VFrameSmooth::Private & tw = *(VFrameSmooth::Private *)parm;
    tw.threadFunction();
    return NULL;
}


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
    //			vInfo("Overslept " << overSleep << " seconds");
            }
        }
    }
    return sleepSeconds;
}



void VFrameSmooth::Private::threadFunction()
{
    JavaVM *javaVM = JniUtils::GetJavaVM();
    javaVM->AttachCurrentThread(&m_jni, nullptr);

    smoothThreadInit();


    pthread_mutex_lock( &m_smoothMutex );
    pthread_cond_signal( &m_smoothIslocked );
    pthread_mutex_unlock( &m_smoothMutex );

    vInfo("VFrameSmooth::Private::threadFunction().");

    bool removedSchedFifo = false;
    jobject activityObject = vApp->javaObject();

    for (double vsync = 0; ; vsync++) {
        const double current = ceil(getFractionalVsync());
        if (abs(current - vsync) > 2.0) {
            //vInfo("Changing vsync from " << vsync << " to " << current);
            vsync = current;
        }
        if ( m_shutdownRequest.state() )
        {
            vInfo("Received shutdownRequest.");
            break;
        }


        const double currentTime = VTimer::Seconds();
        const double lastWarpTime = m_lastsmoothTimeInSeconds.state();
        jmethodID setSchedFifoMethodId = JniUtils::GetStaticMethodID(m_jni, VrLibClass, "setSchedFifoStatic", "(Landroid/app/Activity;II)I");
        if (removedSchedFifo) {
            if (lastWarpTime > currentTime - 0.1) {
                m_jni->CallStaticIntMethod(VrLibClass, setSchedFifoMethodId, activityObject, m_sStartupTid, 1);
                removedSchedFifo = false;
            }
        } else {
            if (lastWarpTime < currentTime - 1.0) {
                m_jni->CallStaticIntMethod(VrLibClass, setSchedFifoMethodId, activityObject, m_sStartupTid, 0);
                removedSchedFifo = true;
            }
        }

        //vInfo( "WarpThreadLoop enter rendertodisplay");
        renderToDisplay( vsync,m_screen.isFrontBuffer() ? spAsyncFrontBufferPortrait
                                                        : spAsyncSwappedBufferPortrait);
    }

    smoothThreadShutdown();
    javaVM->DetachCurrentThread();
    vInfo("Exiting VFrameSmooth::Private::threadFunction().");
}

VFrameSmooth::VFrameSmooth(bool async, bool wantSingleBuffer)
        : d(new Private(async, wantSingleBuffer))
{
}

VFrameSmooth::~VFrameSmooth()
{
    d->destroy();
}

void VFrameSmooth::Private::smoothThreadInit()
{
    vInfo("VFrameSmooth::Private::smoothThreadInit().");

    pthread_setname_np( pthread_self(), "NervGear::VFrameSmooth" );



    EGLint contextAttribs[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, m_eglClientVersion,
        EGL_NONE, EGL_NONE,
        EGL_NONE
    };

    if ( m_contextPriority != EGL_CONTEXT_PRIORITY_MEDIUM_IMG )
    {
        contextAttribs[2] = EGL_CONTEXT_PRIORITY_LEVEL_IMG;
        contextAttribs[3] = m_contextPriority;
    }


    m_eglStatus.m_context = eglCreateContext( m_eglStatus.m_display, m_eglStatus.m_config, m_eglShareContext, contextAttribs );
    if (m_eglStatus.m_context == EGL_NO_CONTEXT )
    {
        vFatal("eglCreateContext failed: " << m_eglStatus.getEglErrorString());
    }
    vInfo("m_eglStatus.m_context: " << m_eglStatus.m_context);
    if ( m_contextPriority != EGL_CONTEXT_PRIORITY_MEDIUM_IMG )
    {
        // See what context priority we actually got
        EGLint actualPriorityLevel;
        eglQueryContext( m_eglStatus.m_display, m_eglStatus.m_context, EGL_CONTEXT_PRIORITY_LEVEL_IMG, &actualPriorityLevel );
        switch ( actualPriorityLevel )
        {
        case EGL_CONTEXT_PRIORITY_HIGH_IMG: vInfo("Context is EGL_CONTEXT_PRIORITY_HIGH_IMG."); break;
        case EGL_CONTEXT_PRIORITY_MEDIUM_IMG: vInfo("Context is EGL_CONTEXT_PRIORITY_MEDIUM_IMG."); break;
        case EGL_CONTEXT_PRIORITY_LOW_IMG: vInfo("Context is EGL_CONTEXT_PRIORITY_LOW_IMG."); break;
        default: vInfo("Priority level of context is unknown."); break;
        }
    }


    vInfo("eglMakeCurrent() on " << m_eglMainThreadSurface << ".");

    if ( eglMakeCurrent( m_eglStatus.m_display, m_eglMainThreadSurface,
                         m_eglMainThreadSurface, m_eglStatus.m_context ) == EGL_FALSE )
    {
        vFatal("eglMakeCurrent failed: " << m_eglStatus.getEglErrorString());
    }

    m_screen.initForCurrentSurface( m_jni, m_wantSingleBuffer,m_buildVersionSDK);

    createFrameworkGraphics();
    m_smoothThreadTid = gettid();

    vInfo("VFrameSmooth::Private::smoothThreadInit() - End.");
}

void VFrameSmooth::Private::smoothThreadShutdown()
{

    vInfo("VFrameSmooth::Private::smoothThreadShutdown().");


    destroyFrameworkGraphics();

    for ( int i = 0; i < 1; i++ )
    {
        //warpSource_t & ws = m_warpSources[i];
        if ( m_gpuSync )
        {
            if ( EGL_FALSE == m_eglStatus.eglDestroySyncKHR( m_eglStatus.m_display, m_gpuSync) )
            {
                vInfo("m_eglStatus.eglDestroySyncKHR returned EGL_FALSE.");
            }
            m_gpuSync = 0;
        }
    }


    if ( eglMakeCurrent( m_eglStatus.m_display, EGL_NO_SURFACE, EGL_NO_SURFACE,
                         EGL_NO_CONTEXT ) == EGL_FALSE )
    {
        vFatal("eglMakeCurrent: shutdown failed");
    }

    if ( eglDestroyContext( m_eglStatus.m_display, m_eglStatus.m_context ) == EGL_FALSE )
    {
        vFatal("eglDestroyContext: shutdown failed");
    }
    m_eglStatus.m_context = 0;


    vInfo("VFrameSmooth::Private::smoothThreadShutdown() - End.");
}

const VGlShader & VFrameSmooth::Private::chooseProgram( const bool disableChromaticCorrection ) const
{
    int program = VAlgorithm::Clamp( (int)m_smoothProgram, (int)VK_DEFAULT, (int)VK_MAX - 1 );

    if ( disableChromaticCorrection && program >= VK_DEFAULT_CB )
    {
        program -= ( VK_DEFAULT_CB - VK_DEFAULT );
    }
    return m_warpPrograms[program];
}
void VFrameSmooth::Private::setSmoothpState( ) const
{
    glDepthMask( GL_FALSE );	// don't write to depth, even if Unity has depth on window
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_CULL_FACE );
    glDisable( GL_BLEND );
    glEnable( GL_SCISSOR_TEST );

  //m_eglStatus.logErrorsEnum( "SetWarpState" );
}

void VFrameSmooth::Private::bindSmoothProgram( const VR4Matrixf timeWarps[2][2], const VR4Matrixf rollingWarp,
                                     const int eye, const double vsyncBase /* for spinner */ ) const
{
     const VR4Matrixf landscapeOrientationMatrix(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f );

    // Select the warp program.
    const VGlShader & warpProg = chooseProgram(m_disableChromaticCorrection );
    glUseProgram( warpProg.program );

    // Set the shader parameters.
    glUniform1f( warpProg.uniformColor, m_programParms[0] );

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
        const V2Vectf clamp( eye * 0.5f, (eye+1)* 0.5f );
        glUniform2fv( warpProg.uniformTexClamp, 1, &clamp.x );
    }
    if ( warpProg.uniformRotateScale > 0 )
    {

       const double angle = framePointTimeInSeconds( vsyncBase )* M_PI * m_programParms[0];
        const V4Vectf RotateScale( sinf( angle ), cosf( angle ), m_programParms[1], 1.0f );
        glUniform4fv( warpProg.uniformRotateScale, 1, &RotateScale[0] );
    }
}

void VFrameSmooth::Private::bindCursorProgram() const
{
    const VR4Matrixf landscapeOrientationMatrix(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f );

    const VGlShader & warpProg = m_warpPrograms[ VK_DEFAULT ];
    glUseProgram( warpProg.program );
    glUniform1f( warpProg.uniformColor, 1.0f );
    glUniformMatrix4fv( warpProg.uniformModelViewProMatrix, 1, GL_FALSE, landscapeOrientationMatrix.Transposed().M[0] );
    glUniformMatrix4fv( warpProg.uniformTexMatrix, 1, GL_FALSE, VR4Matrixf::Identity().M[0] );
    glUniformMatrix4fv( warpProg.uniformTexMatrix2, 1, GL_FALSE, VR4Matrixf::Identity().M[0] );
}

int CameraTimeWarpLatency = 4;
bool CameraTimeWarpPause;

void VFrameSmooth::Private::bindEyeTextures( const int eye )
{
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, m_texId[eye][0] );
    if ( m_smoothOptions & VK_INHIBIT_SRGB_FB )
    {
        glTexParameteri( GL_TEXTURE_2D, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_SKIP_DECODE_EXT );
    }
    else
    {
        glTexParameteri( GL_TEXTURE_2D, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_DECODE_EXT );
    }

    if ( m_smoothProgram == VK_PLANE
         || m_smoothProgram == VK_PLANE_CB
         || m_smoothProgram == VK_PLANE_LAYER
         || m_smoothProgram == VK_PLANE_LAYER_CB
         || m_smoothProgram == VK_PLANE_LOD
         || m_smoothProgram == VK_PLANE_LOD_CB
         )
    {
        glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_2D, m_texId[eye][1] );
        if ( m_smoothOptions & VK_INHIBIT_SRGB_FB )
        {
            glTexParameteri( GL_TEXTURE_2D, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_SKIP_DECODE_EXT );
        }
        else
        {
            glTexParameteri( GL_TEXTURE_2D, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_DECODE_EXT );
        }
    }
    if ( m_smoothProgram == VK_PLANE_SPECIAL
         || m_smoothProgram == VK_PLANE_SPECIAL_CB
         || m_smoothProgram == VK_RESERVED
         || m_smoothProgram == VK_RESERVED_CB )
    {
        glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_EXTERNAL_OES, m_texId[eye][1] );

            if ( m_smoothOptions & VK_INHIBIT_SRGB_FB )
            {
                glTexParameteri( GL_TEXTURE_EXTERNAL_OES, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_SKIP_DECODE_EXT );
            }
            else
            {
                glTexParameteri( GL_TEXTURE_EXTERNAL_OES, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_DECODE_EXT );
            }
    }
    if ( m_smoothProgram == VK_CUBE || m_smoothProgram == VK_CUBE_CB )
    {
        glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_CUBE_MAP, m_texId[eye][1]);
            if ( m_smoothOptions & VK_INHIBIT_SRGB_FB )
            {
                glTexParameteri( GL_TEXTURE_CUBE_MAP, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_SKIP_DECODE_EXT );
            }
            else
            {
                glTexParameteri( GL_TEXTURE_CUBE_MAP, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_DECODE_EXT );
            }
    }

    if ( m_smoothProgram == VK_CUBE_SPECIAL || m_smoothProgram == VK_CUBE_SPECIAL_CB )
    {
        for ( int i = 0; i < 3; i++ )
        {
            glActiveTexture( GL_TEXTURE1 + i );
            glBindTexture( GL_TEXTURE_CUBE_MAP, m_planarTexId[eye][1][i] );
                if ( m_smoothOptions & VK_INHIBIT_SRGB_FB )
                {
                    glTexParameteri( GL_TEXTURE_CUBE_MAP, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_SKIP_DECODE_EXT );
                }
                else
                {
                    glTexParameteri( GL_TEXTURE_CUBE_MAP, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT, VEglDriver::GL_DECODE_EXT );
                }
        }
    }

    if ( m_smoothProgram == VK_LOGO || m_smoothProgram == VK_LOGO_CB )
    {
        glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_2D, m_texId[eye][1] );
            if ( m_smoothOptions & VK_INHIBIT_SRGB_FB )
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

void VFrameSmooth::Private::renderToDisplay( const double vsyncBase_, const swapProgram_t & swap )
{
    static double lastReportTime = 0;
    const double timeNow = floor( VTimer::Seconds() );
    if ( timeNow > lastReportTime )
    {



        lastReportTime = timeNow;
    }

    if ( m_smoothOptions & VK_USE_S )
    {
        renderToDisplayBySliced( vsyncBase_, swap );
        return;
    }

    const double vsyncBase = vsyncBase_;

    // The mesh covers the full screen, but we only draw part of it at a time
    int screenWidth, screenHeight;
    m_screen.getScreenResolution( screenWidth, screenHeight );
    glViewport( 0, 0, screenWidth, screenHeight );
    glScissor( 0, 0, screenWidth, screenHeight );

    for ( int eye = 0; eye <= 1; ++eye )
    {

        //vInfo("Eye " << eye << ": now=" << getFractionalVsync() << "  sleepTo=" << vsyncBase + swap.deltaVsync[eye]);

        // Sleep until we are in the correct half of the screen for
        // rendering this eye.  If we are running single threaded,
        // the first eye will probably already be past the sleep point,
        // so only the second eye will be at a dependable time.
        const double sleepTargetVsync = vsyncBase + swap.deltaVsync[eye];
        const double sleepTargetTime = framePointTimeInSeconds( sleepTargetVsync );
        sleepUntilTimePoint( sleepTargetTime, false );


        //vInfo("Vsync " << vsyncBase << ":" << eye << " sleep " << secondsToSleep);


        long long thisEyeBufferNum = 0;
        int	back;

        if ( eye == 0 )
        {
            const long long latestEyeBufferNum = m_eyeBufferCount.state();
            for ( back = 0; back < 3; back++ )
            {
                thisEyeBufferNum = latestEyeBufferNum - back;
                if ( thisEyeBufferNum <= 0 )
                {
                    //vInfo("WarpToScreen: No valid Eye Buffers");
                    break;
                }

                if ( m_minimumVsync > vsyncBase )
                {

                    continue;
                }
                if ( m_gpuSync == 0 )
                {
                    vInfo("thisEyeBufferNum " << thisEyeBufferNum << " had 0 sync.");
                    break;
                }

                if (m_pose[eye][0].LengthSq() < 1e-18f) {
                    vWarn("m_pose[eye][0] is invalid and now thisEyeBufferNum is " << thisEyeBufferNum << "!");
                    break;
                }

                const EGLint wait = VEglDriver::eglClientWaitSyncKHR( m_eglStatus.m_display, m_gpuSync,
                                                                      EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, 0 );
                if ( wait == EGL_TIMEOUT_EXPIRED_KHR )
                {
                    continue;
                }
                if ( wait == EGL_FALSE )
                {
                    vInfo("eglClientWaitSyncKHR returned EGL_FALSE.");
                }


                if ( m_firstDisplayedVsync[eye] == 0 )
                {
                    m_firstDisplayedVsync[eye] = (long long)vsyncBase;
                }

                break;
            }


            {
                SwapState	state;
                state.VsyncCount = (long long)vsyncBase;
                state.EyeBufferCount = thisEyeBufferNum;
                m_swapVsync.setState( state );


                if ( !pthread_mutex_trylock( &m_smoothMutex ) )
                {
                    pthread_cond_signal( &m_smoothIslocked );
                    pthread_mutex_unlock( &m_smoothMutex );
                }
            }

            if ( m_texId[eye][0] == 0 )
            {

                //vInfo("WarpToScreen: Nothing valid to draw");

                sleepUntilTimePoint( framePointTimeInSeconds( sleepTargetVsync + 1.0f ), false );
                break;
            }
        }


        VR4Matrixf velocity;
        const int velocitySteps = std::min( 3, (int)((long long)vsyncBase - m_minimumVsync) );
        for ( int i = 0; i < velocitySteps; i++ )
        {
            velocity = velocity * m_externalVelocity;
        }


        const bool dualLayer = ( m_texId[eye][1] > 0 );


        VR4Matrixf timeWarps[2][2];
        VRotationState sensor[2];
        for ( int scan = 0; scan < 2; scan++ )
        {
            const double vsyncPoint = vsyncBase + swap.predictionPoints[eye][scan];
            const double timePoint = framePointTimeInSeconds( vsyncPoint );
            sensor[scan] = VRotationSensor::instance()->predictState( timePoint );
            const VR4Matrixf warp = CalculateTimeWarpMatrix2(m_pose[eye][0], sensor[scan]) * velocity;
            timeWarps[0][scan] = m_texMatrix[eye][0] * warp;
            if ( dualLayer )
            {
                if ( m_smoothOptions & VK_FIXED_LAYER )
                {
                    timeWarps[1][scan] = m_texMatrix[eye][1];
                }
                else
                {
                    const VR4Matrixf warp2 = CalculateTimeWarpMatrix2(m_pose[eye][1], sensor[scan]) * velocity;
                    timeWarps[1][scan] = m_texMatrix[eye][1] * warp2;
                }
            }
        }


        const VR4Matrixf rollingWarp = CalculateTimeWarpMatrix2(sensor[0], sensor[1]);





        setSmoothpState( );

        bindSmoothProgram( timeWarps, rollingWarp, eye, vsyncBase );

        bindEyeTextures( eye );

        m_screen.beginDirectRendering( eye * screenWidth/2, 0, screenWidth/2, screenHeight );

        VEglDriver::glBindVertexArrayOES( m_smoothMesh.vertexArrayObject );
        const int indexCount = m_smoothMesh.indexCount / 2;
        const int indexOffset = eye * indexCount;
        glDrawElements( GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, (void *)(indexOffset * 2 ) );


        if ( m_smoothOptions & VK_DISPLAY_CURSOR )
        {
            bindCursorProgram();
            glEnable( GL_BLEND );
            VEglDriver::glBindVertexArrayOES( m_cursorMesh.vertexArrayObject );
            const int indexCount = m_cursorMesh.indexCount / 2;
            const int indexOffset = eye * indexCount;
            glDrawElements( GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, (void *)(indexOffset * 2 ) );
            glDisable( GL_BLEND );
        }


        drawFrameworkGraphicsToWindow( eye, m_smoothOptions);

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
            vInfo("Frame: " << (int)vsyncBase << "; Eye: " << eye << "; latency: " << latency << ".");
        }
    }

    UnbindEyeTextures();

    glUseProgram( 0 );

    VEglDriver::glBindVertexArrayOES( 0 );

    if ( !m_screen.isFrontBuffer() )
    {
        m_screen.swapBuffers();
    }
}

void VFrameSmooth::Private::renderToDisplayBySliced( const double vsyncBase, const swapProgram_t & swap )
{

    const VsyncState vsyncState = UpdatedVsyncState.state();
    if ( vsyncState.vsyncBaseNano == 0 )
    {
        return;
    }



    double	sliceTimes[9];

    static const double startBias = 0.0;
    static const double activeFraction = 112.0 / 135;
    for ( int i = 0; i <= 8; i++ )
    {
        const double framePoint = vsyncBase + activeFraction * (float)i / 8;
        sliceTimes[i] = ( vsyncState.vsyncBaseNano +
                          ( framePoint - vsyncState.vsyncCount ) * vsyncState.vsyncPeriodNano )
                * 0.000000001 + startBias;
    }

    // The mesh covers the full screen, but we only draw part of it at a time
    int screenWidth, screenHeight;
    m_screen.getScreenResolution( screenWidth, screenHeight );
    glViewport( 0, 0, screenWidth, screenHeight );
    glScissor( 0, 0, screenWidth, screenHeight );

    // This must be long enough to cover CPU scheduling delays, GPU in-flight commands,
    // and the actual drawing of this slice.
//    const warpSource_t & latestWarpSource = m_warpSources[m_eyeBufferCount.state()%MAX_WARP_SOURCES];
//    const double schedulingCushion = latestWarpSource.WarpParms.PreScheduleSeconds;

    const double schedulingCushion = m_preScheduleSeconds;
    int	back = 0;

    long long thisEyeBufferNum = 0;
    for ( int screenSlice = 0; screenSlice < 8; screenSlice++ )
    {
        const int	eye = (int)( screenSlice / 4 );

        // Sleep until we are in the correct part of the screen for
        // rendering this slice.
        const double sleepTargetTime = sliceTimes[ screenSlice ] - schedulingCushion;
        sleepUntilTimePoint( sleepTargetTime, false );


        //vInfo("slice " << screenSlice << " targ " << sleepTargetTime << " slept " << secondsToSleep);

        if ( screenSlice == 0 )
        {
            const long long latestEyeBufferNum = m_eyeBufferCount.state();
            for ( back = 0; back < 3; back++ )
            {
                thisEyeBufferNum = latestEyeBufferNum - back;
                if ( thisEyeBufferNum <= 0 )
                {

                    vWarn("No valid Eye Buffers.");
                    break;
                }


                if ( m_minimumVsync > vsyncBase )
                {
                    // a full frame got completed in less time than a single eye; don't use it to avoid stuttering
                    continue;
                }

                if ( m_gpuSync == 0 )
                {
                    vInfo("thisEyeBufferNum " << thisEyeBufferNum << " had 0 sync");
                    break;
                }

                if (m_pose[eye][0].LengthSq() < 1e-18f) {
                    vWarn("m_pose[eye][0] is invalid!");
                    continue;
                }

                const EGLint wait = VEglDriver::eglClientWaitSyncKHR( m_eglStatus.m_display, m_gpuSync,
                                                                      EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, 0 );
                if ( wait == EGL_TIMEOUT_EXPIRED_KHR )
                {
                    continue;
                }
                if ( wait == EGL_FALSE )
                {
                    vInfo("eglClientWaitSyncKHR returned EGL_FALSE.");
                }


                if ( m_firstDisplayedVsync[eye] == 0 )
                {
                    m_firstDisplayedVsync[eye] = (long long)vsyncBase;
                }

                break;
            }


            if ( screenSlice == 0 )
            {
                SwapState	state;
                state.VsyncCount = (long long)vsyncBase;
                state.EyeBufferCount = thisEyeBufferNum;
                m_swapVsync.setState( state );


                if ( !pthread_mutex_trylock( &m_smoothMutex ) )
                {
                    pthread_cond_signal( &m_smoothIslocked );
                    pthread_mutex_unlock( &m_smoothMutex );
                }
            }

            if ( m_texId[eye][0] == 0 )
            {

                vInfo("Nothing valid to draw.");

                sleepUntilTimePoint( framePointTimeInSeconds( vsyncBase + 1.0f ), false );
                break;
            }
        }


        VR4Matrixf velocity;
        const int velocitySteps = std::min( 3, (int)((long long)vsyncBase - m_minimumVsync) );
        for ( int i = 0; i < velocitySteps; i++ )
        {
            velocity = velocity * m_externalVelocity;
        }

        const bool dualLayer = ( m_texId[eye][1] > 0 );


        VR4Matrixf timeWarps[2][2];
        static VRotationState sensor[2];
        for ( int scan = 0; scan < 2; scan++ )
        {

            static VR4Matrixf	warp;
            if ( scan == 1 || screenSlice == 0 || screenSlice == 4 )
            {
                const double timePoint = sliceTimes[screenSlice + scan];
                sensor[scan] = VRotationSensor::instance()->predictState(timePoint);
                warp = CalculateTimeWarpMatrix2(m_pose[eye][0], sensor[scan]) * velocity;
            }
            timeWarps[0][scan] = m_texMatrix[eye][0] * warp;
            if ( dualLayer )
            {
                if ( m_smoothOptions & VK_FIXED_LAYER )
                {
                    timeWarps[1][scan] = m_texMatrix[eye][1];
                }
                else
                {
                    const VR4Matrixf warp2 = CalculateTimeWarpMatrix2(m_pose[eye][1], sensor[scan]) * velocity;
                    timeWarps[1][scan] = m_texMatrix[eye][1] * warp2;
                }
            }
        }

        const VR4Matrixf rollingWarp = CalculateTimeWarpMatrix2(sensor[0], sensor[1]);

        setSmoothpState();

        bindSmoothProgram( timeWarps, rollingWarp, eye, vsyncBase );

        if ( screenSlice == 0 || screenSlice == 4 )
        {
            bindEyeTextures( eye );
        }

        const int sliceSize = screenWidth / 8;

        m_screen.beginDirectRendering( sliceSize*screenSlice, 0, sliceSize, screenHeight );

        const VGlGeometry & mesh = m_slicesmoothMesh;
        VEglDriver::glBindVertexArrayOES( mesh.vertexArrayObject );
        const int indexCount = mesh.indexCount / 8;
        const int indexOffset = screenSlice * indexCount;
        glDrawElements( GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, (void *)(indexOffset * 2 ) );

        if ( 0 )
        {
            const int cycleColor = (int)vsyncBase + screenSlice;
            glClearColor( cycleColor & 1, ( cycleColor >> 1 ) & 1, ( cycleColor >> 2 ) & 1, 1 );
            glClear( GL_COLOR_BUFFER_BIT );
        }


        drawFrameworkGraphicsToWindow( eye, m_smoothOptions);

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
            vInfo("Frame: " << (int)vsyncBase << "; Eye: " << eye << "; latency: " << latency << ".");
        }
    }

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

void VFrameSmooth::Private::smoothInternal( )
{
    if ( gettid() != m_sStartupTid )
    {
        vFatal("WarpSwap: Called with tid " << gettid() << " instead of " << m_sStartupTid);
    }

    m_lastsmoothTimeInSeconds.setState( VTimer::Seconds() );


    glBindFramebuffer( GL_FRAMEBUFFER, 0 );




    const long long lastBufferCount = m_eyeBufferCount.state();

    m_minimumVsync = m_lastSwapVsyncCount + 2 * m_minimumVsyncs;	// don't use it if from same frame to avoid problems with very fast frames
    m_firstDisplayedVsync[0] = 0;			// will be set when it becomes the currentSource
    m_firstDisplayedVsync[1] = 0;			// will be set when it becomes the currentSource
    m_disableChromaticCorrection = ( ( m_eglStatus.eglGetGpuType() & VEglDriver::GPU_TYPE_MALI_T760_EXYNOS_5433 ) != 0 );

    if ( ( m_smoothProgram & VK_IMAGE ) != 0 )
    {
        for ( int eye = 0; eye < 2; eye++ )
        {
            if ( m_texId[eye][0] == 0 )
            {
                m_texId[eye][0] = m_blackTexId;
            }
            if ( m_texId[eye][1] == 0 )
            {
                m_texId[eye][1] = m_defaultLoadingIconTexId;
            }
        }
    }


    if ( m_gpuSync != EGL_NO_SYNC_KHR )
    {
        if ( EGL_FALSE == m_eglStatus.eglDestroySyncKHR( m_eglStatus.m_display, m_gpuSync ) )
        {
            vInfo( "m_eglStatus.eglDestroySyncKHR returned EGL_FALSE." );
        }
    }


    m_gpuSync = m_eglStatus.eglCreateSyncKHR( m_eglStatus.m_display, EGL_SYNC_FENCE_KHR, NULL );
    if ( m_gpuSync == EGL_NO_SYNC_KHR )
    {
        vInfo( "eglCreateSyncKHR_(): EGL_NO_SYNC_KHR." );
    }


    if ( EGL_FALSE == m_eglStatus.eglClientWaitSyncKHR( m_eglStatus.m_display, m_gpuSync,
                                                        EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, 0 ) )
    {
        vInfo( "eglClientWaitSyncKHR returned EGL_FALSE." );
    }


    m_eyeBufferCount.setState( lastBufferCount + 1 );


    if ( !m_async )
    {


        m_eglStatus.glFinish();

        swapProgram_t * swapProg;
        if ( m_screen.isFrontBuffer() )
        {
            swapProg = &spSyncFrontBufferPortrait;
        }
        else
        {
            swapProg = &spSyncSwappedBufferPortrait;
        }

        renderToDisplay( floor( getFractionalVsync() ), *swapProg );

        const SwapState state = m_swapVsync.state();
        m_lastSwapVsyncCount = state.VsyncCount;

        return;
    }

    for ( ; ; )
    {
        const uint64_t startSuspendNanoSeconds = GetNanoSecondsUint64();


        pthread_mutex_lock( &m_smoothMutex );

        pthread_cond_wait( &m_smoothIslocked, &m_smoothMutex );

        pthread_mutex_unlock( &m_smoothMutex );

        const uint64_t endSuspendNanoSeconds = GetNanoSecondsUint64();

        const SwapState state = m_swapVsync.state();
        if ( state.EyeBufferCount >= lastBufferCount )
        {

            m_lastSwapVsyncCount = std::max( state.VsyncCount, m_lastSwapVsyncCount + m_minimumVsyncs );

            const uint64_t suspendNanoSeconds = endSuspendNanoSeconds - startSuspendNanoSeconds;
            if ( suspendNanoSeconds < 1000 * 1000 )
            {
                const uint64_t suspendMicroSeconds = ( 1000 * 1000 - suspendNanoSeconds ) / 1000;

                usleep( suspendMicroSeconds );
            }
            return;
        }
    }
}



void	VFrameSmooth::doSmooth()
{


    const int count = ( ( d->m_smoothOptions & VK_FLUSH ) != 0 ) ? 3 : 1;
    for ( int i = 0; i < count; i++ )
    {


        d->smoothInternal();
    }

}
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


    geo.indexCount = lineVertCount;

    VEglDriver::glBindVertexArrayOES( 0 );

    return geo;
}


float calibrateFovScale = 1.0f;

void VFrameSmooth::Private::createFrameworkGraphics()
{

    unsigned char blackData[4] = {};
    glGenTextures( 1, &m_blackTexId );
    glBindTexture( GL_TEXTURE_2D, m_blackTexId );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, blackData );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glBindTexture( GL_TEXTURE_2D, 0 );


    m_smoothMesh = VLensDistortion::createDistortionGrid( m_device, 1, calibrateFovScale, false );

    m_slicesmoothMesh = VLensDistortion::createDistortionGrid( m_device, 4, calibrateFovScale, false );

    m_cursorMesh = VLensDistortion::createDistortionGrid( m_device, 1, calibrateFovScale, true );

    if ( m_smoothMesh.indexCount == 0 || m_slicesmoothMesh.indexCount == 0 )
    {
        vFatal("WarpMesh failed to load");
    }


   // m_timingGraph = CreateTimingGraphGeometry( (256+10)*2 );


    m_calibrationLines2.createCalibrationGrid( 0, false );

    m_untexturedMvpProgram.initShader(VGlShader::getUntextureMvpVertexShaderSource(),VGlShader::getUntexturedFragmentShaderSource());
    m_debugLineProgram.initShader(VGlShader::getVertexColorVertexShaderSource(),VGlShader::getUntexturedFragmentShaderSource());
    buildSmoothProgs();
}

void VFrameSmooth::Private::destroyFrameworkGraphics()
{
    glDeleteTextures( 1, &m_blackTexId );
    glDeleteTextures( 1, &m_defaultLoadingIconTexId );

    m_calibrationLines2.destroy();
    m_smoothMesh.destroy();
    m_slicesmoothMesh.destroy();
    m_cursorMesh.destroy();


    m_untexturedMvpProgram.destroy();
    m_debugLineProgram.destroy();

    for ( int i = 0; i < VK_MAX; i++ )
    {
        m_warpPrograms[i].destroy();
    }
}


void VFrameSmooth::Private::drawFrameworkGraphicsToWindow( const int eye,
                                                   const int swapOptions)
{
    /*unsigned char latencyTesterColorToDisplay[3];
    if ( ovr_ProcessLatencyTest( latencyTesterColorToDisplay ) )
    {
        glClearColor(
                    latencyTesterColorToDisplay[0] / 255.0f,
                latencyTesterColorToDisplay[1] / 255.0f,
                latencyTesterColorToDisplay[2] / 255.0f,
                1.0f );
        glClear( GL_COLOR_BUFFER_BIT );
    }*/

    if ( swapOptions & VK_DRAW_LINES )
    {
        const float znear = 0.5f;
        const float zfar = 150.0f;

        const VR4Matrixf projectionMatrix(
                    0, 1, 0, 0,
                    -1, 0, 0, 0,
                    0, 0, zfar / (znear - zfar), (zfar * znear) / (znear - zfar),
                    0, 0, -1, 0 );
        glUseProgram( m_untexturedMvpProgram.program );
        glLineWidth( 2.0f );
        glUniform4f( m_untexturedMvpProgram.uniformColor, 1, 0, 0, 1 );
        glUniformMatrix4fv( m_untexturedMvpProgram.uniformModelViewProMatrix, 1, GL_FALSE,  // not transposed
                            projectionMatrix.Transposed().M[0] );
        VEglDriver::glBindVertexArrayOES( m_calibrationLines2.vertexArrayObject );


        int width, height;
        m_screen.getScreenResolution( width, height );
        glViewport( width/2 * (int)eye, 0, width/2, height );
        glDrawElements( GL_LINES, m_calibrationLines2.indexCount, GL_UNSIGNED_SHORT, NULL );
        glViewport( 0, 0, width, height );
    }
}

void VFrameSmooth::Private::buildSmoothProgPair( VrKernelProgram simpleIndex,
                                       const char * simpleVertex, const char * simpleFragment,
                                       const char * chromaticVertex, const char * chromaticFragment
)
{
    m_warpPrograms[ simpleIndex ] = VGlShader( simpleVertex, simpleFragment );
    m_warpPrograms[ simpleIndex + ( VK_DEFAULT_CB - VK_DEFAULT ) ] = VGlShader( chromaticVertex, chromaticFragment );
}

void VFrameSmooth::Private::buildSmoothProgMatchedPair( VrKernelProgram simpleIndex,
                                              const char * simpleVertex, const char * simpleFragment
)
{
    m_warpPrograms[ simpleIndex ] = VGlShader( simpleVertex, simpleFragment );
    m_warpPrograms[ simpleIndex + ( VK_DEFAULT_CB - VK_DEFAULT ) ] = VGlShader( simpleVertex, simpleFragment );
}


void VFrameSmooth::Private::buildSmoothProgs()
{
    buildSmoothProgPair( VK_DEFAULT,
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

    buildSmoothProgPair( VK_PLANE,
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
    buildSmoothProgPair( VK_PLANE_SPECIAL,
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
    buildSmoothProgPair( VK_CUBE,
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
    buildSmoothProgPair( VK_CUBE_SPECIAL,
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
    buildSmoothProgPair( VK_LOGO,
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
    buildSmoothProgPair( VK_HALF,
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

    buildSmoothProgPair( VK_PLANE_LAYER,
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
    buildSmoothProgMatchedPair( VK_PLANE_LOD,
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

    buildSmoothProgMatchedPair( VK_RESERVED,
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

