#pragma once

#include "VRotationState.h"
#include "VMatrix.h"
#include <jni.h>

enum{
    VK_INHIBIT_SRGB_FB = 1,
    VK_USE_S = 2,
    VK_FLUSH = 4,
    VK_FIXED_LAYER = 8,
    VK_DISPLAY_CURSOR = 16,
    VK_IMAGE = 32,
    VK_DRAW_LINES = 64
};

enum VrKernelProgram {
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
};

enum eExitType {
    EXIT_TYPE_NONE,
    EXIT_TYPE_FINISH,
    EXIT_TYPE_FINISH_AFFINITY,
    EXIT_TYPE_EXIT
};

NV_NAMESPACE_BEGIN


class VKernel
{
public:
    static VKernel *instance();
    void run();
    void exit();
    void destroy(eExitType type);

    bool asyncSmooth;
    int msaa;
    bool isRunning;

    void doSmooth();
    void syncSmoothParms();
    void setSmoothEyeTexture(uint texID, ushort eye, ushort layer);
    void setTexMatrix(const VR4Matrixf &mtexMatrix, ushort eye, ushort layer);
    void setSmoothPose(const VRotationState &pose, ushort eye, ushort layer);
    void setpTex(uint *mpTexId,ushort eye,ushort layer);

    void setSmoothOption(int option);
    void setMinimumVsncs(int vsnc);
    void setExternalVelocity(const VR4Matrixf &extV);
    void setPreScheduleSeconds(float pres);
    void setSmoothProgram(ushort program);
    void setProgramParms(float proParms[4]);

    void InitTimeWarpParms();
    int getBuildVersion();

    int m_smoothOptions;
    VR4Matrixf m_externalVelocity;
    int m_minimumVsyncs;
    float m_preScheduleSeconds;
    ushort m_smoothProgram;
    float m_programParms[4];

    uint m_texId[2][3];
    uint m_planarTexId[2][3][3];
    VR4Matrixf m_texMatrix[2][3];
    VRotationState m_pose[2][3];
    jobject m_ActivityObject;

private:
    VKernel();
};

NV_NAMESPACE_END
