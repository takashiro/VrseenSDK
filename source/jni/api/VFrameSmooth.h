#pragma once

/*
 * Overview:
 *
 * The VFrameSmooth system is designed to provide a very consistent
 * and low latency rendering loop for a head mounted display, while allowing
 * an application to render at lower and more irregular frame rates.
 *
 */

#include <pthread.h>
#include "api/VGlOperation.h"
#include "Android/LogUtils.h"
#include "DirectRender.h"
#include "VrApi.h"
#include "HmdInfo.h"
#include "../core/VString.h"

#include "Lockless.h"
#include "VGlGeometry.h"
#include "VGlShader.h"

const int SCHED_FIFO_PRIORITY_NONE			= 0;
const int SCHED_FIFO_PRIORITY_VRTHREAD		= 1;
const int SCHED_FIFO_PRIORITY_DEVICEMNGR	= 2;
const int SCHED_FIFO_PRIORITY_TIMEWARP		= 3;

NV_NAMESPACE_BEGIN

/*
 * It is necessary to latch both eye images together as a unit.  If
 * eye images were latched independently, VFrameSmooth would work fine for
 * orientation changes looking at static geometry, but an animation or
 * movement could animate backwards for a frame
 * if the left eye got an update just in time, but the right eye didn't
 * complete before time warp needed it, resulting in the use of an older
 * version.
 *
 * We need to guard against the case where an application is normally
 * holding 60 fps, but very simple scenes can complete in 8 milliseconds,
 * which would allow the right eye to be picked up a frame earlier than
 * the left eye, resulting in a stutter for animation.  Head tracking doesn't
 * care, but joypad yawwing is essentially a whole-world animation that
 * stands out in this case.  Checking that the vsync is not from the current
 * frame accomplishes this without adding latency to the not-holding-60 case.
 *
 */

// To enable the simplification of a symetric FOV, the viewport for each
// eye on screen is slightly larger and offset from the exact half of
// the screen.
enum WhichEyeRect
{
    RECT_SCREEN,	// Exactly covers the pixels in the correct half of the screen.
    RECT_VIEWPORT	// Square and extends past the left and right edges.
};

// The mobile displays scan from left to right, which is unfortunately
// the opposite of DK2.
typedef enum
{
    SCREENEYE_LEFT,
    SCREENEYE_RIGHT
} ScreenEye;

struct warpSource_t
{
    long long			MinimumVsync;				// Never pick up a source if it is from the current vsync.
    long long			FirstDisplayedVsync[2];		// External velocity is added after this vsync.
    bool				disableChromaticCorrection;	// Disable correction for chromatic aberration.
    EGLSyncKHR			GpuSync;					// When this sync completes, the textures are done rendering.
    ovrTimeWarpParms	WarpParms;					// passed into WarpSwap()
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

// This is communicated from the VFrameSmooth thread to the VrThread at
// vsync time.
struct SwapState
{
    SwapState() : VsyncCount(0),EyeBufferCount(0) {}
    long long		VsyncCount;
    long long		EyeBufferCount;
};

enum TimeWarpMode_t
{
    TWM_SWAPBUFFERS,	// No front buffer rendering, so tools and capture programs work.
    TWM_SYNCHRONOUS,	// Front buffer from render thread for debugging.
    TWM_ASYNCHRONOUS	// Default behavior -- front buffer from background thread.
};

class TimeWarpInitParms
{
public:
    TimeWarpInitParms() :
            frontBuffer( true ),
            asynchronousTimeWarp( true ),
            distortionFileName( NULL ),
            javaVm( NULL ),
            vrLibClass( NULL ),
            activityObject( NULL ),
            gameThreadTid( 0 ),
            buildVersionSDK( 0 ) {}

    // Graphics debug and video capture tools may not like front buffer rendering.
    bool				frontBuffer;

    // Shipping applications will almost always want this on,
    // but if you want to draw directly to the screen for
    // debug tasks, you can run synchronously so the init
    // thread is still current on the window.
    bool				asynchronousTimeWarp;

    // If not NULL, the distortion mesh will be loaded from this file
    const char *		distortionFileName;

    // directory to load external data from
    VString				externalStorageDirectory;

    hmdInfoInternal_t	hmdInfo;

    // For changing SCHED_FIFO on the calling thread.
    JavaVM *			javaVm;
    jclass				vrLibClass;
    jobject				activityObject;
    pid_t				gameThreadTid;

    int					buildVersionSDK;
};

// Abstract interface
class VFrameSmooth
{
public:
    VFrameSmooth( const TimeWarpInitParms initParms );

    // Optionally spawns a separate thread that will warp
    // eye images directly to the screen, regardless of the
    // frame rate of the main thread.
    //
    // Call with the main OpenGL context already current, so
    // the new graphics context can know the correct display and config,
    // and can share texture objects with it.
    //
    // On return with useBackgroundThread_, the calling context will be current on
    // a tiny pbuffer, since the background context has taken over
    // the real window surface. EGL_KHR_surfaceless_context is unfortunately
    // not supported on the platforms we care about.
    //
    // Calling Startup() if VFrameSmooth is already active will be a fatal error.
    //
    // Any problems during startup will be a fatal error.
    //
    // Vsync must be initialized before calling.
    static VFrameSmooth * Factory( TimeWarpInitParms initParms );

    // The system should be able to shutdown and reinitialize multiple times
    // by an application.  Each pause of the application will require a shutdown,
    // and resume should restart.
    ~VFrameSmooth();

    // Accepts a new pos + texture set that will be used for future warps.
    // The parms are copied, and are not referenced after the function returns.
    //
    // The application GL context that rendered the eye images must be current,
    // but drawing does not need to be completed.  In the asynchronous case, a
    // sync object will be added to the current context so the background thread
    // can know when it is ready to use.
    //
    // In the async case, this will block until the textures from the previous
    // WarpSwap have completed rendering, to allow one frame of overlap for maximum
    // GPU utilization, but prevent multiple frames from piling up variable latency.
    //
    // This will block until at least one vsync has passed since the last
    // call to WarpSwap to prevent applications with simple scenes from
    // generating completely wasted frames.
    //
    // Calling with VFrameSmooth uninitialized will log a warning and return.
    //
    // Calling from a thread other than the one that called startup will be
    // a fatal error.
    void		warpSwap( const ovrTimeWarpParms & parms );

    // Get the thread ID so we can set SCHED_FIFO
    int			warpThreadTid() const { return m_warpThreadTid; }
    pthread_t	warpThread() const { return m_warpThread; }

private:
    // POSIX thread launching shim, just calls WarpThread()
    static void *	ThreadStarter( void * parm );

    void			threadFunction();
    void 			warpThreadInit();
    void			warpThreadShutdown();

    void			warpSwapInternal( const ovrTimeWarpParms & parms );

    // Ensures that the warpPrograms have a matched set with and without
    // chromatic aberration so it can be universally disabled for slower systems
    // and power saving mode.
    void			buildWarpProgPair( ovrTimeWarpProgram simpleIndex,
                                       const char * simpleVertex, const char * simpleFragment,
                                       const char * chromaticVertex, const char * chromaticFragment );

    // If there is no difference between the low and high quality versions, use this function.
    void			buildWarpProgMatchedPair( ovrTimeWarpProgram simpleIndex,
                                              const char * vertex, const char * fragment );
    void 			buildWarpProgs();

    // FrameworkGraphics include the latency tester, calibration lines, edge vignette, fps counter,
    // debug graphs.
    void			createFrameworkGraphics();
    void			destroyFrameworkGraphics();
    void			drawFrameworkGraphicsToWindow( const ScreenEye eye, const int swapOptions,
                                                   const bool drawTimingGraph );
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

    // Wait for sync points amd warp to screen.
    void			warpToScreen( const double vsyncBase, const swapProgram_t & swap );
    void			warpToScreenSliced( const double vsyncBase, const swapProgram_t & swap );

    // Build new verts for the timing graph, call once each frame
    void			updateTimingGraphVerts( const ovrTimeWarpDebugPerfMode debugPerfMode, const ovrTimeWarpDebugPerfValue debugValue );

    // Draw debug graphs
    void 			drawTimingGraph( const ScreenEye eye );

    const VGlShader & programForParms( const ovrTimeWarpParms & parms, const bool disableChromaticCorrection ) const;
    void			setWarpState( const warpSource_t & currentWarpSource ) const;
    void			bindWarpProgram( const warpSource_t & currentWarpSource, const VR4Matrixf timeWarps[2][2],
                                     const VR4Matrixf rollingWarp, const int eye, const double vsyncBase ) const;
    void			bindCursorProgram() const;

    // Parameters from Startup()
    TimeWarpInitParms m_initParms;

    DirectRender	m_screen;

    bool			m_hasEXT_sRGB_write_control;	// extension

    // It is an error to call WarpSwap() from a different thread
    pid_t			m_sStartupTid;

    // To change SCHED_FIFO on the StartupTid.
    JNIEnv *		m_jni;
    jmethodID		m_setSchedFifoMethodId;

    // Support for updating a SurfaceTexture from the warp thread
    jmethodID		m_updateTexImageMethodId;
    jmethodID 		m_getTimestampMethodId;

    // Last time WarpSwap() was called.
    LocklessUpdater<double>		m_lastWarpSwapTimeInSeconds;

    // Retrieved from the main thread context
    EGLDisplay		m_eglDisplay;

    EGLSurface		m_eglPbufferSurface;
    EGLSurface		m_eglMainThreadSurface;
    EGLConfig		m_eglConfig;
    EGLint			m_eglClientVersion;	// VFrameSmooth can work with EGL 2.0 or 3.0
    EGLContext		m_eglShareContext;

    // Our private context, only used for warping to the screen.
    EGLContext		m_eglWarpContext;
    GLuint			m_contextPriority;

    // Data for timing graph
    static const int EYE_LOG_COUNT = 512;
    eyeLog_t		m_eyeLog[EYE_LOG_COUNT];
    long long		m_lastEyeLog;	// eyeLog[(lastEyeLog-1)&(EYE_LOG_COUNT-1)] has valid data

    // GPU time queries around eye warp rendering.
    LogGpuTime<NUM_SLICES_PER_SCREEN>	m_logEyeWarpGpuTime;

    // The warp loop will exit when this is set true.
    LocklessUpdater<bool>		m_shutdownRequest;

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
    LocklessUpdater<long long>			m_eyeBufferCount;	// only set by WarpSwap()
    warpSource_t	m_warpSources[MAX_WARP_SOURCES];

    LocklessUpdater<SwapState>		m_swapVsync;		// Set by WarpToScreen(), read by WarpSwap()

    long long			m_lastSwapVsyncCount;			// SwapVsync at return from last WarpSwap()
};

VR4Matrixf CalculateTimeWarpMatrix( const VQuatf &from, const VQuatf &to,
                                  const float fovDegrees );

void	WarpTexCoord2( const hmdInfoInternal_t & hmdInfo, const float in[2], float out[2] );

NV_NAMESPACE_END


