#pragma once

#include "../vglobal.h"
#include "VDevice.h"
#include "VKernel.h"

NV_NAMESPACE_BEGIN

class VFrameSmooth
{
public:
    VFrameSmooth(bool async,VDevice *device);
    ~VFrameSmooth();
    void setSmoothEyeTexture(unsigned int texID,ushort eye,ushort layer);
    void setTexMatrix(VR4Matrixf	mtexMatrix,ushort eye,ushort layer);
    void setSmoothPose(VKpose	mpose,ushort eye,ushort layer);
    void setpTex(unsigned int	*mpTexId,ushort eye,ushort layer);


    void setSmoothOption(int option);
    void setMinimumVsncs( int vsnc);
    void setExternalVelocity(VR4Matrixf extV);
    void setPreScheduleSeconds(float pres);
    void setSmoothProgram(ushort program);
    void setProgramParms( float * proParms);
    void	doSmooth();



private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VFrameSmooth)
};


NV_NAMESPACE_END


