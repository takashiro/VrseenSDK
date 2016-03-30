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

extern "C" {

// Returns global, absolute high-resolution time in seconds. This is the same value
// as used in sensor messages and on Android also the same as Java's system.nanoTime(),
// which is what the Choreographer vsync timestamp is based on.
// Can be called any time from any thread.
double			ovr_GetTimeInSeconds();

//-----------------------------------------------------------------
// Initialization / Shutdown
//-----------------------------------------------------------------

// The JavaVM will be saved in this global variable.
extern JavaVM * VrLibJavaVM;


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
} ovrTimeWarpParms;


typedef enum
{
    EXIT_TYPE_NONE,				// This will not exit the activity at all -- normally used for starting the platform UI activity
    EXIT_TYPE_FINISH,			// This will finish the current activity.
    EXIT_TYPE_FINISH_AFFINITY,	// This will finish all activities on the stack.
    EXIT_TYPE_EXIT				// This calls ovr_Shutdown() and exit(0).
    // Must be called from the Java thread!
} eExitType;


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


// Utility function to default initialize the ovrTimeWarpParms.

typedef enum
{
    WARP_INIT_DEFAULT,
    WARP_INIT_BLACK,
    WARP_INIT_LOADING_ICON,
    WARP_INIT_MESSAGE
} ovrWarpInit;

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

NV_NAMESPACE_BEGIN

class VDevice;

class VKernel
{
public:
    static  VKernel*  GetInstance();
    void run();
    void exit();
    void destroy(eExitType type);

    void doSmooth(const ovrTimeWarpParms * parms );

    void ovr_HandleDeviceStateChanges();
    ovrSensorState	ovr_GetPredictedSensorState(double absTime );
    void			ovr_RecenterYaw();

    VDevice* device;
    bool asyncSmooth;
    int msaa;

    bool isRunning;


    void doSmooth();
    void syncSmoothParms();
    void setSmoothEyeTexture(unsigned int texID,ushort eye,ushort layer);
    void setSmoothOption(int option);
    void setMinimumVsncs( int vsnc);
    void setExternalVelocity(ovrMatrix4f extV);
    void setPreScheduleSeconds(float pres);
    void setSmoothProgram(ovrTimeWarpProgram program);
    void setProgramParms( float * proParms);
    void setSmoothParms(ovrTimeWarpParms   parms);
    ovrTimeWarpParms  getSmoothParms();


    ovrTimeWarpImage 			m_images[2][3];
    int 						m_smoothOptions;
    ovrMatrix4f					m_externalVelocity;
    int							m_minimumVsyncs;
    float						m_preScheduleSeconds;
    ovrTimeWarpProgram			m_smoothProgram;
    float						m_programParms[4];

private:
    VKernel();
};

NV_NAMESPACE_END
