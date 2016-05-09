#pragma once

#include "VMatrix.h"
#include "VKernel.h"

class VDevice;

NV_NAMESPACE_BEGIN

class VFrameSmooth
{
public:
    VFrameSmooth(bool async, VDevice *device);
    ~VFrameSmooth();

    void setSmoothEyeTexture(uint texID,ushort eye,ushort layer);
    void setTexMatrix(const VR4Matrixf &mtexMatrix, ushort eye, ushort layer);
    void setSmoothPose(const VRotationState &mpose, ushort eye, ushort layer);
    void setpTex(uint *mpTexId, ushort eye, ushort layer);
    void setSmoothOption(int option);
    void setMinimumVsncs(int vsnc);
    void setExternalVelocity(const VR4Matrixf &extV);
    void setPreScheduleSeconds(float pres);
    void setSmoothProgram(ushort program);
    void setProgramParms(float *proParms);

    int threadId() const;
    void doSmooth();

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VFrameSmooth)
};

NV_NAMESPACE_END
