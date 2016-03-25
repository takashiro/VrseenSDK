#pragma once
#include <VString.h>
#include "math.h"

#if defined( ANDROID )
#include <jni.h>
#elif defined( __cplusplus )
typedef struct _JNIEnv JNIEnv;
typedef struct _JavaVM JavaVM;
typedef class _jobject * jobject;
#else
typedef const struct JNINativeInterface * JNIEnv;
typedef const struct JNIInvokeInterface * JavaVM;
void * jobject;
#endif

/*

Multiple Android activities that live in the same address space can cooperatively use the VrApi.
However, only one activity can be in "VR mode" at a time The following explains when an activity
is expected to enter/leave VR mode.


Android Activity life cycle
===========================

An Android Activity can only be in VR mode while the activity is in the resumed state.
The following shows how VR mode fits into the Android Activity life cycle.

	1.  ovr_Initialize()
	2.  VrActivity::onCreate()
	3.  VrActivity::onStart() <--------+
	4.  VrActivity::onResume() <----+  |
	5.  ovr_EnterVrMode()           |  |
	6.  ovr_LeaveVrMode()           |  |
	7.  VrActivity::onPause() ------+  |
	8.  VrActivity::onStop() ----------+
	9.  VrActivity::onDestroy()
	10. ovr_Shutdown()


Android Surface life cycle
==========================

An Android Activity can only be in VR mode while there is a valid Android Surface.
The following shows how VR mode fits into the Android Surface life cycle.

	1.  ovr_Initialize()
	2.  VrActivity::surfaceCreated() <-----+
	3.  VrActivity::surfaceChanged()       |
	4.  ovr_EnterVrMode()                  |
	5.  ovr_LeaveVrMode()                  |
	6.  VrActivity::surfaceDestroyed() ----+
	7.  ovr_Shutdown()

Note that the life cycle of a surface is not necessarily tightly coupled with the
life cycle of an activity. These two life cycles may interleave in complex ways.
Usually surfaceCreated() is called after onResume() and surfaceDestroyed() is called
before onDestroy(). However, this is not guaranteed and, for instance, surfaceDestroyed()
may be called after onDestroy().

A call to surfaceCreated() is always immediately followed by a call to surfaceChanged()
and onPause() is always called before surfaceDestroyed(). As such, an Android Activity
is only in the resumed state with a valid Android Surface between surfaceChanged() or
onResume() (whichever comes last) and onPause(). In other words, a VR application will
typically enter VR mode from surfaceChanged() or onResume() (whichever comes last), and
leave VR mode from onPause().


Android VR life cycle
=====================

	// Android Activity/Surface life cycle loop.
	for ( ; ; )
	{
		// Acquire ANativeWindow from Android Surface and create EGLSurface.
		// Create EGLContext and make the context current on the surface.

		// Enter VR mode once the activity is in the resumed state with a
		// valid EGLSurface and current EGLContext.
		ovrModeParms parms;
		ovrHmdInfo returnedHmdInfo;
		ovrMobile * ovr = ovr_EnterVrMode( parms, &returnedHmdInfo );

		// Use the ovrHmdInfo suggested FOV and resolution to setup a projection
		// matrix and eye render targets.

		// Frame loop.
		for ( ; ; )
		{
			// Get the HMD pose, predicted for the middle of the time period during which
			// the new eye images will be displayed. The number of frames predicted ahead
			// depends on the pipeline depth of the engine and the synthesis rate.
			// The better the prediction, the less black will be pulled in at the edges.
			const double predictedTime = ovr_GetPredictedDisplayTime( ovr, MINIMUM_VSYNCS, PIPELINE_DEPTH );
			const ovrSensorState state = ovr_GetPredictedSensorState( ovr, predictedTime );

			// Render eye images and setup ovrTimeWarpParms using ovrSensorState.
			ovrTimeWarpParms parms;
			for ( int eye = 0; eye < 2; eye++ )
			{
				parms.Images[eye][0].TexId = ;	// eye image
				parms.Images[eye][0].Pose = state.Predicted;
			}

			// Hand over the eye images to the time warp.
			ovr_WarpSwap( ovr, &parms );

			// Handle HMD events such as power level state changed, mount/unmount, and dock/undock.
			ovr_HandleDeviceStateChanges( ovr );
		}

		// Leave VR mode when the activity is paused, the Android Surface is
		// destroyed, or when switching to another activity.
		ovr_LeaveVrMode( ovr );
	}


Implementation
==============

The API is designed to work with an Android Activity using a plain Android SurfaceView,
where the Activity life cycle and the Surface life cycle are managed completely in native
code by sending the life cycle events (onCreate, onPause, surfaceChanged etc.) to native code.

The API does not work with an Android Activity using a GLSurfaceView. The GLSurfaceView
class manages the window surface and EGLSurface and the implementation of GLSurfaceView
may unbind the EGLSurface before onPause() gets called. As such, there is no way to
leave VR mode before the EGLSurface disappears. Another problem with GLSurfaceView is
that it creates the EGLContext using eglChooseConfig(). The Android EGL code pushes in
multisample flags in eglChooseConfig() if the user has selected the "force 4x MSAA" option
in settings. Using a multisampled front buffer is completely wasted for time warp
rendering.

Alternatively an Android NativeActivity can be used to avoid manually handling all
the life cycle events. However, it is important to select the EGLConfig manually
without using eglChooseConfig() to make sure the front buffer is not a multisampled.


Eye Image Synthesis
===================

ovr_WarpSwap() controls the synthesis rate through ovrTimeWarpParms::MinimumVsyncs.
ovr_WarpSwap() also controls at which point during a display refresh cycle the
synthesis starts. ovr_WarpSwap() only returns when the previous eye images have
been consumed by the asynchronous time warp thread, and at least MinimumVsyncs
have passed. The asynchronous time warp thread consumes new eye images halfway
through a display refresh cycle, which is the first time it can start updating
the first eye, covering the first half of the display. As a result, ovr_WarpSwap()
returns halfway through a display refresh cycle, after which synthesis can start.

Once ovr_WarpSwap() returns, synthesis has a full display refresh cycle to generate
new eye images up to the next halfway point. At the next halfway point, the time
warp has half a display refresh cycle (up to V-sync) to update the first eye. The
time warp then effectively waits for V-Sync and then has another half a display
refresh cycle (up to the next-next halfway point) to update the second eye. The
asynchronous time warp uses a high priority GPU context and will eat away cycles
from synthesis, so synthesis does not have a full display refresh cycle worth of
actual GPU cycles. However, the asynchronous time warp tends to be very fast,
leaving most of the GPU time for synthesis.

For good performance on tiled GPUs, the CPU command generation and GPU execution
will have to overlap because the GPU will not start rendering a frame until all
rendering commands have been submitted. Because CPU command generation and GPU
execution overlap, the time between sensor input sampling for synthesis, and the
first photons coming out of the display is 2.5 display refresh cycles (when
MimimumVsyncs == 1). Engines with a deeper pipeline, for instance, with a separate
thread for game logic and rendering, will have a larger delay between sensor input
sampling for synthesis, and the first photons coming out of the display. By using
late view binding (or late latching), the time between the sensor input sampling
for synthesis and the first photons coming out of the display can be reduced
to 1.5 display refresh cycles.

Instead of using the latest sensor sampling, synthesis uses predicted sensor input
for the middle of the time period during which the new eye images will be displayed.
This predicted time is calculated using ovr_GetPredictedDisplayTime(). The number
of frames predicted ahead depends on the pipeline depth and the minumum number of
V-syncs in between eye image rendering. Less than half a display refresh cycle
before each eye image will be displayed, the asynchronous time warp will get new
predicted sensor input using the very latest sensor sampling. The asynchronous
time warp then corrects the eye images using this new sensor input. In other words,
the asynchronous time warp will always correct the eye images even if the predicted
sensor input for synthesis was not perfect. However, the better the prediction, for
synthesis, the less black will be pulled in at the edges by the asynchronous time warp.

Ideally the eye images are only displayed for the MinimumVsyncs display refresh
cycles that are centered about the eye image prediction time. In other words, a set
of eye images is first displayed at prediction time minus MinimumVsyncs / 2 display
refresh cycles. The eye images should never be shown before this time because that
can cause intra frame motion judder. Ideally the eye images are also not shown after
the prediction time plus MinimumVsyncs / 2 display refresh cycles, but this may
happen if synthesis fails to produce new eye images in time.

MinimumVsyncs = 1
|---|---|---|---|---|---|---|---|---|  - V-syncs
| * | * | * | * | * | * | * | * | * |  - eye image display periods (* = predicted time)

MinimumVsyncs = 2
|---|---|---|---|---|---|---|---|---|  - V-syncs
|   *   |   *   |   *   |   *   |   *  - eye image display periods (* = predicted time)

MinimumVsyncs = 3
|---|---|---|---|---|---|---|---|---|  - V-syncs
|     *     |     *     |     *     |  - eye image display periods (* = predicted time)

*/

extern "C" {

// Returns the version + compile time stamp as a string.
// Can be called any time from any thread.
char const *	ovr_GetVersionString();

// Returns global, absolute high-resolution time in seconds. This is the same value
// as used in sensor messages and on Android also the same as Java's system.nanoTime(),
// which is what the Choreographer vsync timestamp is based on.
// Can be called any time from any thread.
double			ovr_GetTimeInSeconds();

//-----------------------------------------------------------------
// HMD information.
//-----------------------------------------------------------------

typedef struct
{
	// With a display resolution of 2560x1440, the pixels at the center
	// of each eye cover about 0.06 degrees of visual arc. To wrap a
	// full 360 degrees, about 6000 pixels would be needed and about one
	// quarter of that would be needed for ~90 degrees FOV. Eye images
	// with a resolution of 1536x1536 would result in a good 1:1 mapping
	// in the center, but they would need mip-maps for off center pixels.
	// To avoid the need for mip-maps and for significantly improved
	// rendering performance this currently returns a conservative 1024x1024.
	int		SuggestedEyeResolution[2];

	// This is a product of the lens distortion and the screen size,
	// but there is no truly correct answer.
	//
	// There is a tradeoff in resolution and coverage.
	// Too small of an fov will leave unrendered pixels visible, but too
	// large wastes resolution or fill rate.  It is unreasonable to
	// increase it until the corners are completely covered, but we do
	// want most of the outside edges completely covered.
	//
	// Applications might choose to render a larger fov when angular
	// acceleration is high to reduce black pull in at the edges by
	// TimeWarp.
	//
	// Currently symmetric 90.0 degrees.
	float	SuggestedEyeFov[2];
} ovrHmdInfo;

//-----------------------------------------------------------------
// Initialization / Shutdown
//-----------------------------------------------------------------

// This must be called by a function called directly from a java thread,
// preferably at JNI_OnLoad().  It will fail if called from a pthread created
// in native code, or from a NativeActivity due to the class-lookup issue:
//
// http://developer.android.com/training/articles/perf-jni.html#faq_FindClass
//
// This should not start any threads or consume any significant amount of
// resources, so hybrid apps aren't penalizing their normal mode of operation
// by supporting VR.
void		ovr_OnLoad( JavaVM * JavaVm_ );

// The JavaVM will be saved in this global variable.
extern JavaVM * VrLibJavaVM;

// For a dedicated VR app this is called from JNI_OnLoad().
// A hybrid app, however, may want to defer calling it until the first headset
// plugin event to avoid starting the device manager.
void		ovr_Init();

// Shutdown without exiting the activity.
// A dedicated VR app will call ovr_ExitActivity instead, but a hybrid
// app may call this when leaving VR mode.
void		ovr_Shutdown();

//-----------------------------------------------------------------
// Enter/Leave VR mode
//-----------------------------------------------------------------

typedef struct
{
	// Shipping applications will almost always want this on,
	// but if you want to draw directly to the screen for
	// debug tasks, you can run synchronously so the init
	// thread is still current on the window.
	bool	AsynchronousTimeWarp;

	// To avoid an incorrect fullscreen transparent window that
	// wastes lots of bandwidth on return from platform UI, we
	// explicitly reset the fullscreen flag.  Unfortunately, that
	// causes Android life cycle events that mess up several
	// NativeActivity codebases like Stratum and UE4, so we need
	// to be able to conditionally disable that.
	// Hopefully this all goes away when platform UI is moved
	// to a separate apk.
	bool	SkipWindowFullscreenReset;	// FIXME:VRAPI remove


	// This thread, in addition to the calling one, will
	// get SCHED_FIFO.
	int		GameThreadTid;

	// The java Activity object is needed to get the windowManager,
	// packageName, systemService, etc.
	jobject ActivityObject;
} ovrModeParms;

// VR context
// To allow multiple Android activities that live in the same address space
// to cooperatively use the VrApi, each activity needs to maintain its own
// separate contexts for a lot of the video related systems.
struct ovrMobile;

// Starts up TimeWarp, vsync tracking, sensor reading, clock locking,
// thread scheduling, and sets video options. The calling thread will be
// given SCHED_FIFO. Should be called when the app is both resumed and
// has a valid window surface. The application must have their preferred
// OpenGL ES context current on the window surface so the correct version
// and config can be matched for the background time warp thread. On return,
// the application context will be current on an invisible pbuffer, because
// the time warp takes ownership of the window surface.
ovrMobile *	ovr_EnterVrMode( ovrModeParms parms, ovrHmdInfo * returnedHmdInfo );

// Shut everything down for window destruction.
// The ovrMobile object is freed by this function.
// Must be called from the same thread that called ovr_EnterVrMode().
void		ovr_LeaveVrMode( ovrMobile * ovr );

// Called every frame to handle power level state changes and
// HMD events, such as mount/unmount and dock/undock.
// Must be called from the same thread that called ovr_EnterVrMode().
void		ovr_HandleDeviceStateChanges( ovrMobile * ovr );	// FIXME:VRAPI move into ovr_WarpSwap()

//-----------------------------------------------------------------
// HMD sensor input
//-----------------------------------------------------------------

typedef struct ovrQuatf_
{
    float x, y, z, w;
} ovrQuatf;

typedef struct ovrVector3f_
{
    float x, y, z;
} ovrVector3f;

// Position and orientation together.
typedef struct ovrPosef_
{
	ovrQuatf	Orientation;
	ovrVector3f	Position;
} ovrPosef;

// Full pose (rigid body) configuration with first and second derivatives.
typedef struct ovrPoseStatef_
{
	ovrPosef	Pose;
	ovrVector3f	AngularVelocity;
	ovrVector3f	LinearVelocity;
	ovrVector3f	AngularAcceleration;
	ovrVector3f	LinearAcceleration;
	double		TimeInSeconds;         // Absolute time of this state sample.
} ovrPoseStatef;

// Bit flags describing the current status of sensor tracking.
typedef enum
{
	ovrStatus_OrientationTracked	= 0x0001,	// Orientation is currently tracked (connected and in use).
	ovrStatus_PositionTracked		= 0x0002,	// Position is currently tracked (FALSE if out of range).
	ovrStatus_PositionConnected		= 0x0020,	// Position tracking HW is connected.
	ovrStatus_HmdConnected			= 0x0080	// HMD Display is available & connected.
} ovrStatusBits;

// State of the sensor at a given absolute time.
typedef struct ovrSensorState_
{
	// Predicted pose configuration at requested absolute time.
	// One can determine the time difference between predicted and actual
	// readings by comparing ovrPoseState.TimeInSeconds.
	ovrPoseStatef	Predicted;
	// Actual recorded pose configuration based on the sensor sample at a
	// moment closest to the requested time.
	ovrPoseStatef	Recorded;
	// Sensor temperature reading, in degrees Celsius, as sample time.
	float			Temperature;
	// Sensor status described by ovrStatusBits.
	unsigned		Status;
} ovrSensorState;

// Predict the time at which the next set of eye images, for which synthesis
// is about to start, will be displayed. The predicted time is the middle of
// the time period during which the new eye images will be displayed. The
// number of frames predicted ahead depends on the pipeline depth of the
// engine and the minumum number of V-syncs in between eye image rendering.
// The better the prediction, the less black will be pulled in at the edges
// by the time warp.
// Can be called from any thread while in VR mode.
double			ovr_GetPredictedDisplayTime( ovrMobile * ovr, int minimumVsyncs, int pipelineDepth );

// Returns the predicted sensor state based on the specified absolute system time.
// Pass absTime value of 0.0 to request the most recent sensor reading, in which
// case both Predicted and Recorded will have the same value.
// Can be called from any thread while in VR mode.
ovrSensorState	ovr_GetPredictedSensorState( ovrMobile * ovr, double absTime );

// Recenters the orientation on the yaw axis.
// Note that this immediately affects ovr_GetPredictedSensorState() which may
// be called asynchronously from the time warp. It is therefore best to
// make sure the screen is black before recentering to avoid previous eye
// images from being abrubtly warped across the screen.
void			ovr_RecenterYaw( ovrMobile * ovr );

//-----------------------------------------------------------------
// Warp Swap
//-----------------------------------------------------------------

// row-major 4x4 matrix
typedef struct ovrMatrix4f_
{
    float M[4][4];
} ovrMatrix4f;

typedef enum
{
	// To get gamma correct sRGB filtering of the eye textures, the textures must be
	// allocated with GL_SRGB8_ALPHA8 format and the window surface must be allocated
	// with these attributes:
	// EGL_GL_COLORSPACE_KHR,  EGL_GL_COLORSPACE_SRGB_KHR
	//
	// While we can reallocate textures easily enough, we can't change the window
	// colorspace without relaunching the entire application, so if you want to
	// be able to toggle between gamma correct and incorrect, you must allocate
	// the framebuffer as sRGB, then inhibit that processing when using normal
	// textures.
	SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER	= 1,
	// Enable / disable the sliced warp
	SWAP_OPTION_USE_SLICED_WARP				= 2,
	// Flush the warp swap pipeline so the images show up immediately.
	// This is expensive and should only be used when an immediate transition
	// is needed like displaying black when resetting the HMD orientation.
	SWAP_OPTION_FLUSH						= 4,
	// The overlay plane is a HUD, and should ignore head tracking.
	// This is generally poor practice for VR.
	SWAP_OPTION_FIXED_OVERLAY				= 8,
	// The third image plane is blended separately over only a small, central
	// section of each eye for performance reasons, so it is enabled with
	// a flag instead of a shared ovrTimeWarpProgram.
	SWAP_OPTION_SHOW_CURSOR					= 16,
	// Use default images. This is used for showing black and the loading icon.
	SWAP_OPTION_DEFAULT_IMAGES				= 32,
	// Draw the axis lines after warp to show the skew with the pre-warp lines.
	SWAP_OPTION_DRAW_CALIBRATION_LINES		= 64
} ovrSwapOption;

// NOTE: the code which auto-disables chromatic aberration expects the ovrTimeWarpProgram
// list to be symmetric: each program has a version with and a version without
// correction for chromatic aberration. For details see disableChromaticCorrection.
typedef enum
{
	// without correction for chromatic aberration
	WP_SIMPLE,
	WP_MASKED_PLANE,						// overlay plane shows through masked areas in eyes
	WP_MASKED_PLANE_EXTERNAL,				// overlay plane shows through masked areas in eyes, using external texture as source
	WP_MASKED_CUBE,							// overlay cube shows through masked areas in eyes
	WP_CUBE,								// overlay cube only, no main scene (for power savings)
	WP_LOADING_ICON,						// overlay loading icon
	WP_MIDDLE_CLAMP,						// UE4 stereo in a single texture
	WP_OVERLAY_PLANE,						// world shows through transparent parts of overlay plane
	WP_OVERLAY_PLANE_SHOW_LOD,				// debug tool to color tint based on mip levels
	WP_CAMERA,

	// with correction for chromatic aberration
	WP_CHROMATIC,
	WP_CHROMATIC_MASKED_PLANE,				// overlay plane shows through masked areas in eyes
	WP_CHROMATIC_MASKED_PLANE_EXTERNAL,		// overlay plane shows through masked areas in eyes, using external texture as source
	WP_CHROMATIC_MASKED_CUBE,				// overlay cube shows through masked areas in eyes
	WP_CHROMATIC_CUBE,						// overlay cube only, no main scene (for power savings)
	WP_CHROMATIC_LOADING_ICON,				// overlay loading icon
	WP_CHROMATIC_MIDDLE_CLAMP,				// UE4 stereo in a single texture
	WP_CHROMATIC_OVERLAY_PLANE,				// world shows through transparent parts of overlay plane
	WP_CHROMATIC_OVERLAY_PLANE_SHOW_LOD,	// debug tool to color tint based on mip levels
	WP_CHROMATIC_CAMERA,

	WP_PROGRAM_MAX
} ovrTimeWarpProgram;

typedef enum
{
	DEBUG_PERF_OFF,			// data still being collected, just not displayed
	DEBUG_PERF_RUNNING,		// display continuously changing graph
	DEBUG_PERF_FROZEN,		// no new data collection, but displayed
	DEBUG_PERF_MAX
} ovrTimeWarpDebugPerfMode;

typedef enum
{
	DEBUG_VALUE_DRAW,		// start and end times of the draw
	DEBUG_VALUE_LATENCY,	// seconds from eye buffer orientation time
	DEBUG_VALUE_MAX
} ovrTimeWarpDebugPerfValue;

// Note that if overlays are dynamic, they must be triple buffered just
// like the eye images.
typedef struct
{
	// If TexId == 0, this image is disabled.
	// Most applications will have the overlay image
	// disabled.
	//
	// Because OpenGL ES doesn't support clampToBorder,
	// it is the application's responsibility to make sure
	// that all mip levels of the texture have a black border
	// that will show up when time warp pushes the texture partially
	// off screen.
	//
	// Overlap textures will only show through where alpha on the
	// primary texture is not 1.0, so they do not require a border.
	unsigned int	TexId;

	// Experimental separate R/G/B cube maps
	unsigned int	PlanarTexId[3];

	// Points on the screen are mapped by a distortion correction
	// function into ( TanX, TanY, 1, 1 ) vectors that are transformed
	// by this matrix to get ( S, T, Q, _ ) vectors that are looked
	// up with texture2dproj() to get texels.
	ovrMatrix4f		TexCoordsFromTanAngles;

	// The sensor state for which ModelViewMatrix is correct.
	// It is ok to update the orientation for each eye, which
	// can help minimize black edge pull-in, but the position
	// must remain the same for both eyes, or the position would
	// seem to judder "backwards in time" if a frame is dropped.
	ovrPoseStatef	Pose;
} ovrTimeWarpImage;

static const int	MAX_WARP_EYES = 2;
static const int	MAX_WARP_IMAGES = 3;

typedef struct
{
	// Images used for each eye.
	// Per eye: 0 = world, 1 = overlay screen, 2 = gaze cursor
	ovrTimeWarpImage 			Images[MAX_WARP_EYES][MAX_WARP_IMAGES];

	// Combination of ovrTimeWarpOption flags.
	int 						WarpOptions;

	// Rotation from a joypad can be added on generated frames to reduce
	// judder in FPS style experiences when the application framerate is
	// lower than the vsync rate.
	// This will be applied to the view space distorted
	// eye vectors before applying the rest of the time warp.
	// This will only be added when the same ovrTimeWarpParms is used for
	// more than one vsync.
	ovrMatrix4f					ExternalVelocity;

	// WarpSwap will not return until at least this many vsyncs have
	// passed since the previous WarpSwap returned.
	// Setting to 2 will reduce power consumption and may make animation
	// more regular for applications that can't hold full frame rate.
	int							MinimumVsyncs;

	// Time in seconds to start drawing before each slice.
	// Clamped at 0.014 high and 0.002 low, but the very low
	// values will usually result in screen tearing.
	float						PreScheduleSeconds;

	// Which program to run with these images.
	ovrTimeWarpProgram			WarpProgram;

	// Program-specific tuning values.
	float						ProgramParms[4];

	// Controls the collection and display of timing data.
	ovrTimeWarpDebugPerfMode	DebugGraphMode;			// FIXME:VRAPI move to local preferences
	ovrTimeWarpDebugPerfValue	DebugGraphValue;		// FIXME:VRAPI move to local preferences

} ovrTimeWarpParms;

// Accepts a new pos + texture set that will be used for future warps.
// The parms are copied, and are not referenced after the function returns.
//
// The application GL context that rendered the eye images must be current,
// but drawing does not need to be completed.  A sync object will be added
// to the current context so the background thread can know when rendering
// of the eye images has completed.
//
// This will block until the textures from the previous WarpSwap have completed
// rendering, to allow one frame of overlap for maximum GPU utilization, but
// prevent multiple frames from piling up variable latency.
//
// This will block until at least one vsync has passed since the last
// call to WarpSwap to prevent applications with simple scenes from
// generating completely wasted frames.
//
// IMPORTANT: you must triple buffer the textures that you pass to WarpSwap
// to avoid flickering and performance problems.
//
// Must be called from the same thread that called ovr_EnterVrMode().
void		ovr_WarpSwap( ovrMobile * ovr, const ovrTimeWarpParms * parms );


typedef enum
{
	EXIT_TYPE_NONE,				// This will not exit the activity at all -- normally used for starting the platform UI activity
	EXIT_TYPE_FINISH,			// This will finish the current activity.
	EXIT_TYPE_FINISH_AFFINITY,	// This will finish all activities on the stack.
	EXIT_TYPE_EXIT				// This calls ovr_Shutdown() and exit(0).
	// Must be called from the Java thread!
} eExitType;

void ovr_ExitActivity( ovrMobile * ovr, eExitType type );

int ovr_GetVolume();
double ovr_GetTimeSinceLastVolumeChange();


enum eVrApiEventStatus
{
	VRAPI_EVENT_ERROR_INTERNAL = -2,		// queue isn't created, etc.
	VRAPI_EVENT_ERROR_INVALID_BUFFER = -1,	// the buffer passed in was invalid
	VRAPI_EVENT_NOT_PENDING = 0,			// no event is waiting
	VRAPI_EVENT_PENDING,					// an event is waiting
	VRAPI_EVENT_CONSUMED,					// an event was pending but was consumed internally
	VRAPI_EVENT_BUFFER_OVERFLOW,			// an event is being returned, but it could not fit into the buffer
	VRAPI_EVENT_INVALID_JSON				// there was an error parsing the JSON data
};
eVrApiEventStatus ovr_nextPendingEvent(NervGear::VString& buffer, unsigned int const bufferSize );

// This must match the value declared in ProximityReceiver.java / SystemActivityReceiver.java
#define SYSTEM_ACTIVITY_INTENT "com.oculus.system_activity"
#define	SYSTEM_ACTIVITY_EVENT_REORIENT "reorient"
#define SYSTEM_ACTIVITY_EVENT_RETURN_TO_LAUNCHER "returnToLauncher"
#define SYSTEM_ACTIVITY_EVENT_EXIT_TO_HOME "exitToHome"

}	// extern "C"

//-----------------------------------------------------------------
// Matrix helper functions
//-----------------------------------------------------------------

// Returns a 3x3 minor of a 4x4 matrix.
inline float ovrMatrix4f_Minor( const ovrMatrix4f * m, int r0, int r1, int r2, int c0, int c1, int c2 )
{
	return	m->M[r0][c0] * ( m->M[r1][c1] * m->M[r2][c2] - m->M[r2][c1] * m->M[r1][c2] ) -
			  m->M[r0][c1] * ( m->M[r1][c0] * m->M[r2][c2] - m->M[r2][c0] * m->M[r1][c2] ) +
			  m->M[r0][c2] * ( m->M[r1][c0] * m->M[r2][c1] - m->M[r2][c0] * m->M[r1][c1] );
}

// Returns the inverse of a 4x4 matrix.
inline ovrMatrix4f ovrMatrix4f_Inverse( const ovrMatrix4f * m )
{
	const float rcpDet = 1.0f / (	m->M[0][0] * ovrMatrix4f_Minor( m, 1, 2, 3, 1, 2, 3 ) -
									 m->M[0][1] * ovrMatrix4f_Minor( m, 1, 2, 3, 0, 2, 3 ) +
									 m->M[0][2] * ovrMatrix4f_Minor( m, 1, 2, 3, 0, 1, 3 ) -
									 m->M[0][3] * ovrMatrix4f_Minor( m, 1, 2, 3, 0, 1, 2 ) );
	ovrMatrix4f out;
	out.M[0][0] =  ovrMatrix4f_Minor( m, 1, 2, 3, 1, 2, 3 ) * rcpDet;
	out.M[0][1] = -ovrMatrix4f_Minor( m, 0, 2, 3, 1, 2, 3 ) * rcpDet;
	out.M[0][2] =  ovrMatrix4f_Minor( m, 0, 1, 3, 1, 2, 3 ) * rcpDet;
	out.M[0][3] = -ovrMatrix4f_Minor( m, 0, 1, 2, 1, 2, 3 ) * rcpDet;
	out.M[1][0] = -ovrMatrix4f_Minor( m, 1, 2, 3, 0, 2, 3 ) * rcpDet;
	out.M[1][1] =  ovrMatrix4f_Minor( m, 0, 2, 3, 0, 2, 3 ) * rcpDet;
	out.M[1][2] = -ovrMatrix4f_Minor( m, 0, 1, 3, 0, 2, 3 ) * rcpDet;
	out.M[1][3] =  ovrMatrix4f_Minor( m, 0, 1, 2, 0, 2, 3 ) * rcpDet;
	out.M[2][0] =  ovrMatrix4f_Minor( m, 1, 2, 3, 0, 1, 3 ) * rcpDet;
	out.M[2][1] = -ovrMatrix4f_Minor( m, 0, 2, 3, 0, 1, 3 ) * rcpDet;
	out.M[2][2] =  ovrMatrix4f_Minor( m, 0, 1, 3, 0, 1, 3 ) * rcpDet;
	out.M[2][3] = -ovrMatrix4f_Minor( m, 0, 1, 2, 0, 1, 3 ) * rcpDet;
	out.M[3][0] = -ovrMatrix4f_Minor( m, 1, 2, 3, 0, 1, 2 ) * rcpDet;
	out.M[3][1] =  ovrMatrix4f_Minor( m, 0, 2, 3, 0, 1, 2 ) * rcpDet;
	out.M[3][2] = -ovrMatrix4f_Minor( m, 0, 1, 3, 0, 1, 2 ) * rcpDet;
	out.M[3][3] =  ovrMatrix4f_Minor( m, 0, 1, 2, 0, 1, 2 ) * rcpDet;
	return out;
}


// Returns the 4x4 rotation matrix for the given quaternion.
inline ovrMatrix4f ovrMatrix4f_CreateFromQuaternion( const ovrQuatf * q )
{
	const float ww = q->w * q->w;
	const float xx = q->x * q->x;
	const float yy = q->y * q->y;
	const float zz = q->z * q->z;

	ovrMatrix4f out;
	out.M[0][0] = ww + xx - yy - zz;
	out.M[0][1] = 2 * ( q->x * q->y - q->w * q->z );
	out.M[0][2] = 2 * ( q->x * q->z + q->w * q->y );
	out.M[0][3] = 0;
	out.M[1][0] = 2 * ( q->x * q->y + q->w * q->z );
	out.M[1][1] = ww - xx + yy - zz;
	out.M[1][2] = 2 * ( q->y * q->z - q->w * q->x );
	out.M[1][3] = 0;
	out.M[2][0] = 2 * ( q->x * q->z - q->w * q->y );
	out.M[2][1] = 2 * ( q->y * q->z + q->w * q->x );
	out.M[2][2] = ww - xx - yy + zz;
	out.M[2][3] = 0;
	out.M[3][0] = 0;
	out.M[3][1] = 0;
	out.M[3][2] = 0;
	out.M[3][3] = 1;
	return out;
}

//-----------------------------------------------------------------
// ovrTimeWarpParms initialization helper functions
//-----------------------------------------------------------------

// Convert a standard projection matrix into a TanAngle matrix for
// the primary time warp surface.
inline ovrMatrix4f TanAngleMatrixFromProjection( const ovrMatrix4f * projection )
{
	// A projection matrix goes from a view point to NDC, or -1 to 1 space.
	// Scale and bias to convert that to a 0 to 1 space.
	const ovrMatrix4f tanAngleMatrix =
			{ {
					  { 0.5f * projection->M[0][0], 0.5f * projection->M[0][1], 0.5f * projection->M[0][2] - 0.5f, 0.5f * projection->M[0][3] },
					  { 0.5f * projection->M[1][0], 0.5f * projection->M[1][1], 0.5f * projection->M[1][2] - 0.5f, 0.5f * projection->M[1][3] },
					  { 0.0f, 0.0f, -1.0f, 0.0f },
					  { 0.0f, 0.0f, -1.0f, 0.0f }
			  } };
	return tanAngleMatrix;
}

// Trivial version of TanAngleMatrixFromProjection() for a symmetric field of view.
inline ovrMatrix4f TanAngleMatrixFromFov( const float fovDegrees )
{
	const float tanHalfFov = tanf( 0.5f * fovDegrees * ( M_PI / 180.0f ) );
	const ovrMatrix4f tanAngleMatrix =
			{ {
					  { 0.5f / tanHalfFov, 0.0f, -0.5f, 0.0f },
					  { 0.0f, 0.5f / tanHalfFov, -0.5f, 0.0f },
					  { 0.0f, 0.0f, -1.0f, 0.0f },
					  { 0.0f, 0.0f, -1.0f, 0.0f }
			  } };
	return tanAngleMatrix;
}

// If a simple quad defined as a -1 to 1 XY unit square is transformed to
// the camera view with the given modelView matrix, it can alternately be
// drawn as a TimeWarp overlay image to take advantage of the full window
// resolution, which is usually higher than the eye buffer textures, and
// avoid resampling both into the eye buffer, and again to the screen.
// This is used for high quality movie screens and user interface planes.
//
// Note that this is NOT an MVP matrix -- the "projection" is handled
// by the distortion process.
//
// The exact composition of the overlay image and the base image is
// determined by the warpProgram, you may still need to draw the geometry
// into the eye buffer to punch a hole in the alpha channel to let the
// overlay/underlay show through.
//
// This utility functions converts a model-view matrix that would normally
// draw a -1 to 1 unit square to the view into a TanAngle matrix for an
// overlay surface.
//
// The resulting z value should be straight ahead distance to the plane.
// The x and y values will be pre-multiplied by z for projective texturing.
inline ovrMatrix4f TanAngleMatrixFromUnitSquare( const ovrMatrix4f * modelView )
{
	const ovrMatrix4f inv = ovrMatrix4f_Inverse( modelView );
	ovrMatrix4f m;
	m.M[0][0] = 0.5f * inv.M[2][0] - 0.5f * ( inv.M[0][0] * inv.M[2][3] - inv.M[0][3] * inv.M[2][0] );
	m.M[0][1] = 0.5f * inv.M[2][1] - 0.5f * ( inv.M[0][1] * inv.M[2][3] - inv.M[0][3] * inv.M[2][1] );
	m.M[0][2] = 0.5f * inv.M[2][2] - 0.5f * ( inv.M[0][2] * inv.M[2][3] - inv.M[0][3] * inv.M[2][2] );
	m.M[0][3] = 0.0f;
	m.M[1][0] = 0.5f * inv.M[2][0] + 0.5f * ( inv.M[1][0] * inv.M[2][3] - inv.M[1][3] * inv.M[2][0] );
	m.M[1][1] = 0.5f * inv.M[2][1] + 0.5f * ( inv.M[1][1] * inv.M[2][3] - inv.M[1][3] * inv.M[2][1] );
	m.M[1][2] = 0.5f * inv.M[2][2] + 0.5f * ( inv.M[1][2] * inv.M[2][3] - inv.M[1][3] * inv.M[2][2] );
	m.M[1][3] = 0.0f;
	m.M[2][0] = m.M[3][0] = inv.M[2][0];
	m.M[2][1] = m.M[3][1] = inv.M[2][1];
	m.M[2][2] = m.M[3][2] = inv.M[2][2];
	m.M[2][3] = m.M[3][3] = 0.0f;
	return m;
}

// Utility function to calculate external velocity for smooth stick yaw turning.
// To reduce judder in FPS style experiences when the application framerate is
// lower than the vsync rate, the rotation from a joypad can be applied to the
// view space distorted eye vectors before applying the time warp.
inline ovrMatrix4f CalculateExternalVelocity( const ovrMatrix4f * viewMatrix, const float yawRadiansPerSecond )
{
	const float angle = yawRadiansPerSecond * ( -1.0f / 60.0f );
	const float sinHalfAngle = sinf( angle * 0.5f );
	const float cosHalfAngle = cosf( angle * 0.5f );

	// Yaw is always going to be around the world Y axis
	ovrQuatf quat;
	quat.x = viewMatrix->M[0][1] * sinHalfAngle;
	quat.y = viewMatrix->M[1][1] * sinHalfAngle;
	quat.z = viewMatrix->M[2][1] * sinHalfAngle;
	quat.w = cosHalfAngle;
	return ovrMatrix4f_CreateFromQuaternion( &quat );
}

// Utility function to default initialize the ovrTimeWarpParms.

typedef enum
{
	WARP_INIT_DEFAULT,
	WARP_INIT_BLACK,
	WARP_INIT_LOADING_ICON,
	WARP_INIT_MESSAGE
} ovrWarpInit;

inline ovrTimeWarpParms InitTimeWarpParms( const ovrWarpInit init = WARP_INIT_DEFAULT, const unsigned int texId = 0 )
{
	const ovrMatrix4f tanAngleMatrix = TanAngleMatrixFromFov( 90.0f );

	ovrTimeWarpParms parms;
	memset( &parms, 0, sizeof( parms ) );

	for ( int eye = 0; eye < MAX_WARP_EYES; eye++ )
	{
		for ( int i = 0; i < MAX_WARP_IMAGES; i++ )
		{
			parms.Images[eye][i].TexCoordsFromTanAngles = tanAngleMatrix;
			parms.Images[eye][i].Pose.Pose.Orientation.w = 1.0f;
		}
	}
	parms.ExternalVelocity.M[0][0] = 1.0f;
	parms.ExternalVelocity.M[1][1] = 1.0f;
	parms.ExternalVelocity.M[2][2] = 1.0f;
	parms.ExternalVelocity.M[3][3] = 1.0f;
	parms.MinimumVsyncs = 1;
	parms.PreScheduleSeconds = 0.014f;
	parms.WarpProgram = WP_SIMPLE;
	parms.DebugGraphMode = DEBUG_PERF_OFF;
	parms.DebugGraphValue = DEBUG_VALUE_DRAW;

	switch ( init )
	{
		case WARP_INIT_DEFAULT:
		{
			break;
		}
		case WARP_INIT_BLACK:
		{
			parms.WarpOptions = SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER | SWAP_OPTION_FLUSH | SWAP_OPTION_DEFAULT_IMAGES;
			parms.WarpProgram = WP_SIMPLE;
			for ( int eye = 0; eye < MAX_WARP_EYES; eye++ )
			{
				parms.Images[eye][0].TexId = 0;		// default replaced with a black texture
			}
			break;
		}
		case WARP_INIT_LOADING_ICON:
		{
			parms.WarpOptions = SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER | SWAP_OPTION_FLUSH | SWAP_OPTION_DEFAULT_IMAGES;
			parms.WarpProgram = WP_LOADING_ICON;
			parms.ProgramParms[0] = 1.0f;		// rotation in radians per second
			parms.ProgramParms[1] = 16.0f;		// icon size factor smaller than fullscreen
			for ( int eye = 0; eye < MAX_WARP_EYES; eye++ )
			{
				parms.Images[eye][0].TexId = 0;		// default replaced with a black texture
				parms.Images[eye][1].TexId = texId;	// loading icon texture
			}
			break;
		}
		case WARP_INIT_MESSAGE:
		{
			parms.WarpOptions = SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER | SWAP_OPTION_FLUSH | SWAP_OPTION_DEFAULT_IMAGES;
			parms.WarpProgram = WP_LOADING_ICON;
			parms.ProgramParms[0] = 0.0f;		// rotation in radians per second
			parms.ProgramParms[1] = 2.0f;		// message size factor smaller than fullscreen
			for ( int eye = 0; eye < MAX_WARP_EYES; eye++ )
			{
				parms.Images[eye][0].TexId = 0;		// default replaced with a black texture
				parms.Images[eye][1].TexId = texId;	// message texture
			}
			break;
		}
	}
	return parms;
}

