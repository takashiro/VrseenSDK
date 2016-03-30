#pragma once

#include "../vglobal.h"
#include "VDevice.h"

NV_NAMESPACE_BEGIN

class VFrameSmooth
{
public:
    VFrameSmooth(bool async,VDevice *device);
    ~VFrameSmooth();

    void	doSmooth( const ovrTimeWarpParms & parms );




    void setSmoothEyeTexture(unsigned int texID,ushort eye,ushort layer);
    void setSmoothOption(int option);
    void setMinimumVsncs( int vsnc);
    void setExternalVelocity(ovrMatrix4f extV);
    void setPreScheduleSeconds(float pres);
    void setSmoothProgram(ovrTimeWarpProgram program);
    void setProgramParms( float * proParms);
    void	doSmooth();



private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VFrameSmooth)
};


NV_NAMESPACE_END


