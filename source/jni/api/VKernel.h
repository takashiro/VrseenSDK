#pragma once
#include <VString.h>
#include "math.h"
#include "VBasicmath.h"


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


double			ovr_GetTimeInSeconds();

extern JavaVM * VrLibJavaVM;

typedef struct ovrPosef_
{
    NervGear::VQuat<float>	Orientation;
    NervGear::V3Vect<float>	Position;
} ovrPosef;

typedef struct ovrPoseStatef_
{
    ovrPosef	Pose;
    NervGear::V3Vect<float>	AngularVelocity;
    NervGear::V3Vect<float>	LinearVelocity;
    NervGear::V3Vect<float>	AngularAcceleration;
    NervGear::V3Vect<float>	LinearAcceleration;
    double		TimeInSeconds;
} ovrPoseStatef;

typedef enum
{
    ovrStatus_OrientationTracked	= 0x0001,
    ovrStatus_PositionTracked		= 0x0002,
    ovrStatus_PositionConnected		= 0x0020,
    ovrStatus_HmdConnected			= 0x0080
} ovrStatusBits;

typedef struct ovrSensorState_
{
    ovrPoseStatef	Predicted;

    ovrPoseStatef	Recorded;

    float			Temperature;

    unsigned		Status;
} ovrSensorState;


typedef enum
{
            SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER	= 1,

            SWAP_OPTION_USE_SLICED_WARP				= 2,

            SWAP_OPTION_FLUSH						= 4,

            SWAP_OPTION_FIXED_OVERLAY				= 8,

            SWAP_OPTION_SHOW_CURSOR					= 16,

            SWAP_OPTION_DEFAULT_IMAGES				= 32,

            SWAP_OPTION_DRAW_CALIBRATION_LINES		= 64
} ovrSwapOption;


typedef enum
{

            WP_SIMPLE,
    WP_MASKED_PLANE,
    WP_MASKED_PLANE_EXTERNAL,
    WP_MASKED_CUBE,
    WP_CUBE,
    WP_LOADING_ICON,
    WP_MIDDLE_CLAMP,
    WP_OVERLAY_PLANE,
    WP_OVERLAY_PLANE_SHOW_LOD,
    WP_CAMERA,

            WP_CHROMATIC,
    WP_CHROMATIC_MASKED_PLANE,
    WP_CHROMATIC_MASKED_PLANE_EXTERNAL,
    WP_CHROMATIC_MASKED_CUBE,
    WP_CHROMATIC_CUBE,
    WP_CHROMATIC_LOADING_ICON,
    WP_CHROMATIC_MIDDLE_CLAMP,
    WP_CHROMATIC_OVERLAY_PLANE,
    WP_CHROMATIC_OVERLAY_PLANE_SHOW_LOD,
    WP_CHROMATIC_CAMERA,

    WP_PROGRAM_MAX
} ovrTimeWarpProgram;

typedef struct
{

    unsigned int	TexId;


    unsigned int	PlanarTexId[3];


    NervGear::VR4Matrix<float>		TexCoordsFromTanAngles;


    ovrPoseStatef	Pose;
} ovrTimeWarpImage;



typedef struct
{

    ovrTimeWarpImage 			Images[2][3];


    int 						WarpOptions;

    NervGear::VR4Matrix<float>					ExternalVelocity;


    int							MinimumVsyncs;


    float						PreScheduleSeconds;


    ovrTimeWarpProgram			WarpProgram;

    float						ProgramParms[4];
} ovrTimeWarpParms;


typedef enum
{
    EXIT_TYPE_NONE,
    EXIT_TYPE_FINISH,
    EXIT_TYPE_FINISH_AFFINITY,
    EXIT_TYPE_EXIT

} eExitType;


int ovr_GetVolume();
double ovr_GetTimeSinceLastVolumeChange();


enum eVrApiEventStatus
{
    VRAPI_EVENT_ERROR_INTERNAL = -2,
    VRAPI_EVENT_ERROR_INVALID_BUFFER = -1,
    VRAPI_EVENT_NOT_PENDING = 0,
    VRAPI_EVENT_PENDING,
    VRAPI_EVENT_CONSUMED,
    VRAPI_EVENT_BUFFER_OVERFLOW,
    VRAPI_EVENT_INVALID_JSON
};
eVrApiEventStatus ovr_nextPendingEvent(NervGear::VString& buffer, unsigned int const bufferSize );


#define SYSTEM_ACTIVITY_INTENT "com.oculus.system_activity"
#define	SYSTEM_ACTIVITY_EVENT_REORIENT "reorient"
#define SYSTEM_ACTIVITY_EVENT_RETURN_TO_LAUNCHER "returnToLauncher"
#define SYSTEM_ACTIVITY_EVENT_EXIT_TO_HOME "exitToHome"

}



typedef enum
{
    WARP_INIT_DEFAULT,
    WARP_INIT_BLACK,
    WARP_INIT_LOADING_ICON,
    WARP_INIT_MESSAGE
} ovrWarpInit;



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
    void setTexMatrix(VR4Matrixf	mtexMatrix,ushort eye,ushort layer);
    void setSmoothPose(ovrPoseStatef	mpose,ushort eye,ushort layer);
    void setpTex(unsigned int	*mpTexId,ushort eye,ushort layer);


    void setSmoothOption(int option);
    void setMinimumVsncs( int vsnc);
    void setExternalVelocity(VR4Matrixf extV);
    void setPreScheduleSeconds(float pres);
    void setSmoothProgram(ovrTimeWarpProgram program);
    void setProgramParms( float * proParms);
    void setSmoothParms(const ovrTimeWarpParms &  parms);
    ovrTimeWarpParms  getSmoothParms();
    ovrTimeWarpParms  InitTimeWarpParms( const ovrWarpInit init = WARP_INIT_DEFAULT, const unsigned int texId =0 );


    int 						m_smoothOptions;
    VR4Matrixf					m_externalVelocity;
    int							m_minimumVsyncs;
    float						m_preScheduleSeconds;
    ovrTimeWarpProgram			m_smoothProgram;
    float						m_programParms[4];

    unsigned int	m_texId[2][3];
    unsigned int	m_planarTexId[2][3][3];
    VR4Matrixf		m_texMatrix[2][3];
    ovrPoseStatef	m_pose[2][3];

private:
    VKernel();
};

NV_NAMESPACE_END
