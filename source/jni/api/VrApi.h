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


char const *	ovr_GetVersionString();


double			ovr_GetTimeInSeconds();



typedef struct
{

	int		SuggestedEyeResolution[2];


	float	SuggestedEyeFov[2];
} ovrHmdInfo;


void		ovr_OnLoad( JavaVM * JavaVm_ );
extern JavaVM * VrLibJavaVM;
void		ovr_Init();
void		ovr_Shutdown();


typedef struct
{

	bool	AsynchronousTimeWarp;	
    bool	SkipWindowFullscreenReset;
	int		GameThreadTid;	
	jobject ActivityObject;
} ovrModeParms;


struct ovrMobile;

ovrMobile *	ovr_EnterVrMode( ovrModeParms parms, ovrHmdInfo * returnedHmdInfo );


void		ovr_LeaveVrMode( ovrMobile * ovr );


void		ovr_HandleDeviceStateChanges( ovrMobile * ovr );


typedef struct ovrQuatf_
{
    float x, y, z, w;
} ovrQuatf;

typedef struct ovrVector3f_
{
    float x, y, z;
} ovrVector3f;


typedef struct ovrPosef_
{
	ovrQuatf	Orientation;
	ovrVector3f	Position;
} ovrPosef;


typedef struct ovrPoseStatef_
{
	ovrPosef	Pose;
	ovrVector3f	AngularVelocity;
	ovrVector3f	LinearVelocity;
	ovrVector3f	AngularAcceleration;
	ovrVector3f	LinearAcceleration;
    double		TimeInSeconds;
} ovrPoseStatef;


typedef enum
{
	ovrStatus_OrientationTracked	= 0x0001,	// Orientation is currently tracked (connected and in use).
	ovrStatus_PositionTracked		= 0x0002,	// Position is currently tracked (FALSE if out of range).
	ovrStatus_PositionConnected		= 0x0020,	// Position tracking HW is connected.
	ovrStatus_HmdConnected			= 0x0080	// HMD Display is available & connected.
} ovrStatusBits;


typedef struct ovrSensorState_
{	
	ovrPoseStatef	Predicted;	
	ovrPoseStatef	Recorded;	
	float			Temperature;	
	unsigned		Status;
} ovrSensorState;

double			ovr_GetPredictedDisplayTime( ovrMobile * ovr, int minimumVsyncs, int pipelineDepth );
ovrSensorState	ovr_GetPredictedSensorState( ovrMobile * ovr, double absTime );
void			ovr_RecenterYaw( ovrMobile * ovr );


typedef struct ovrMatrix4f_
{
    float M[4][4];
} ovrMatrix4f;

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

	// with correction for chromatic aberration
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

typedef enum
{
    DEBUG_PERF_OFF,
    DEBUG_PERF_RUNNING,
    DEBUG_PERF_FROZEN,
	DEBUG_PERF_MAX
} ovrTimeWarpDebugPerfMode;

typedef enum
{
    DEBUG_VALUE_DRAW,
    DEBUG_VALUE_LATENCY,
	DEBUG_VALUE_MAX
} ovrTimeWarpDebugPerfValue;


typedef struct
{
	unsigned int	TexId;	
	unsigned int	PlanarTexId[3];	
    ovrMatrix4f		TexCoordsFromTanAngles;
	ovrPoseStatef	Pose;
} ovrTimeWarpImage;

static const int	MAX_WARP_EYES = 2;
static const int	MAX_WARP_IMAGES = 3;

typedef struct
{

	ovrTimeWarpImage 			Images[MAX_WARP_EYES][MAX_WARP_IMAGES];	
	int 						WarpOptions;
	ovrMatrix4f					ExternalVelocity;	
	int							MinimumVsyncs;
	float						PreScheduleSeconds;
    ovrTimeWarpProgram			WarpProgram;
	float						ProgramParms[4];
    ovrTimeWarpDebugPerfMode	DebugGraphMode;
    ovrTimeWarpDebugPerfValue	DebugGraphValue;
} ovrTimeWarpParms;

void	ovr_WarpSwap( ovrMobile * ovr, const ovrTimeWarpParms * parms );


typedef enum
{
    EXIT_TYPE_NONE,
    EXIT_TYPE_FINISH,
    EXIT_TYPE_FINISH_AFFINITY,
    EXIT_TYPE_EXIT
} eExitType;

void ovr_ExitActivity( ovrMobile * ovr, eExitType type );
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







inline float ovrMatrix4f_Minor( const ovrMatrix4f * m, int r0, int r1, int r2, int c0, int c1, int c2 )
{
	return	m->M[r0][c0] * ( m->M[r1][c1] * m->M[r2][c2] - m->M[r2][c1] * m->M[r1][c2] ) -
			  m->M[r0][c1] * ( m->M[r1][c0] * m->M[r2][c2] - m->M[r2][c0] * m->M[r1][c2] ) +
			  m->M[r0][c2] * ( m->M[r1][c0] * m->M[r2][c1] - m->M[r2][c0] * m->M[r1][c1] );
}


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


inline ovrMatrix4f TanAngleMatrixFromProjection( const ovrMatrix4f * projection )
{

	const ovrMatrix4f tanAngleMatrix =
			{ {
					  { 0.5f * projection->M[0][0], 0.5f * projection->M[0][1], 0.5f * projection->M[0][2] - 0.5f, 0.5f * projection->M[0][3] },
					  { 0.5f * projection->M[1][0], 0.5f * projection->M[1][1], 0.5f * projection->M[1][2] - 0.5f, 0.5f * projection->M[1][3] },
					  { 0.0f, 0.0f, -1.0f, 0.0f },
					  { 0.0f, 0.0f, -1.0f, 0.0f }
			  } };
	return tanAngleMatrix;
}


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


inline ovrMatrix4f CalculateExternalVelocity( const ovrMatrix4f * viewMatrix, const float yawRadiansPerSecond )
{
	const float angle = yawRadiansPerSecond * ( -1.0f / 60.0f );
	const float sinHalfAngle = sinf( angle * 0.5f );
	const float cosHalfAngle = cosf( angle * 0.5f );


	ovrQuatf quat;
	quat.x = viewMatrix->M[0][1] * sinHalfAngle;
	quat.y = viewMatrix->M[1][1] * sinHalfAngle;
	quat.z = viewMatrix->M[2][1] * sinHalfAngle;
	quat.w = cosHalfAngle;
	return ovrMatrix4f_CreateFromQuaternion( &quat );
}



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
                parms.Images[eye][0].TexId = 0;
			}
			break;
		}
		case WARP_INIT_LOADING_ICON:
		{
			parms.WarpOptions = SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER | SWAP_OPTION_FLUSH | SWAP_OPTION_DEFAULT_IMAGES;
			parms.WarpProgram = WP_LOADING_ICON;
            parms.ProgramParms[0] = 1.0f;
            parms.ProgramParms[1] = 16.0f;
			for ( int eye = 0; eye < MAX_WARP_EYES; eye++ )
			{
                parms.Images[eye][0].TexId = 0;
                parms.Images[eye][1].TexId = texId;
			}
			break;
		}
		case WARP_INIT_MESSAGE:
		{
			parms.WarpOptions = SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER | SWAP_OPTION_FLUSH | SWAP_OPTION_DEFAULT_IMAGES;
			parms.WarpProgram = WP_LOADING_ICON;
            parms.ProgramParms[0] = 0.0f;
            parms.ProgramParms[1] = 2.0f;
			for ( int eye = 0; eye < MAX_WARP_EYES; eye++ )
			{
                parms.Images[eye][0].TexId = 0;
                parms.Images[eye][1].TexId = texId;
			}
			break;
		}
	}
	return parms;
}

