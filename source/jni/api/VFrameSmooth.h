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

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VFrameSmooth)
};


NV_NAMESPACE_END


