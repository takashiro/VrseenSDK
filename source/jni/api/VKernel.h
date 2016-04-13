#pragma once
#include <VString.h>
#include "math.h"
#include "VBasicmath.h"

#include "VRotationSensor.h"

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

extern JavaVM * VrLibJavaVM;

struct VKpose
{
    NervGear::VQuat<float>	Orientation;
    NervGear::V3Vect<float>	Position;
    NervGear::V3Vect<float>	Angular;
    NervGear::V3Vect<float>	Linear;
    NervGear::V3Vect<float>	AngularAc;
    NervGear::V3Vect<float>	LinearAc;
    double		TimeBySeconds;

    VKpose()
    {
    }

    VKpose(const NervGear::VRotationSensor::State &state)
    {
        Orientation.w = state.w;
        Orientation.x = state.x;
        Orientation.y = state.y;
        Orientation.z = state.z;
        TimeBySeconds = state.timestamp;
    }

    VKpose &operator = (const NervGear::VRotationSensor::State &state)
    {
        Orientation.w = state.w;
        Orientation.x = state.x;
        Orientation.y = state.y;
        Orientation.z = state.z;
        TimeBySeconds = state.timestamp;
        return *this;
    }
};

typedef enum
{
            VK_INHIBIT_SRGB_FB	= 1,

            VK_USE_S				= 2,

            VK_FLUSH						= 4,

            VK_FIXED_LAYER				= 8,

            VK_DISPLAY_CURSOR					= 16,

            VK_IMAGE				= 32,

            VK_DRAW_LINES		= 64
} VKoption;


typedef enum
{

    VK_DEFAULT,
    VK_PLANE,
    VK_PLANE_SPECIAL,
    VK_CUBE,
    VK_CUBE_SPECIAL,
    VK_LOGO,
    VK_HALF,
    VK_PLANE_LAYER,
    VK_PLANE_LOD,
    VK_RESERVED,

    VK_DEFAULT_CB,
    VK_PLANE_CB,
    VK_PLANE_SPECIAL_CB,
    VK_CUBE_CB,
    VK_CUBE_SPECIAL_CB,
    VK_LOGO_CB,
    VK_HALF_CB,
    VK_PLANE_LAYER_CB,
    VK_PLANE_LOD_CB,
    VK_RESERVED_CB,

    VK_MAX
} VrKernelProgram;




typedef enum
{
    EXIT_TYPE_NONE,
    EXIT_TYPE_FINISH,
    EXIT_TYPE_FINISH_AFFINITY,
    EXIT_TYPE_EXIT

} eExitType;

#define	SYSTEM_ACTIVITY_EVENT_REORIENT "reorient"
#define SYSTEM_ACTIVITY_EVENT_RETURN_TO_LAUNCHER "returnToLauncher"

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

    VDevice* device;
    bool asyncSmooth;
    int msaa;

    bool isRunning;


    void doSmooth();
    void syncSmoothParms();
    void setSmoothEyeTexture(unsigned int texID,ushort eye,ushort layer);
    void setTexMatrix(VR4Matrixf	mtexMatrix,ushort eye,ushort layer);
    void setSmoothPose(const VRotationSensor::State &pose, ushort eye, ushort layer);
    void setpTex(unsigned int	*mpTexId,ushort eye,ushort layer);


    void setSmoothOption(int option);
    void setMinimumVsncs( int vsnc);
    void setExternalVelocity(VR4Matrixf extV);
    void setPreScheduleSeconds(float pres);
    void setSmoothProgram(ushort program);
    void setProgramParms( float * proParms);


     void InitTimeWarpParms( );


    int 						m_smoothOptions;
    VR4Matrixf					m_externalVelocity;
    int							m_minimumVsyncs;
    float						m_preScheduleSeconds;
    ushort			            m_smoothProgram;
    float						m_programParms[4];

    unsigned int	m_texId[2][3];
    unsigned int	m_planarTexId[2][3][3];
    VR4Matrixf		m_texMatrix[2][3];
    VRotationSensor::State m_pose[2][3];

private:
    VKernel();
};

NV_NAMESPACE_END
