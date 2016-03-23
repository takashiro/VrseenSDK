#pragma once

#include "../vglobal.h"
#include "HmdInfo.h"

NV_NAMESPACE_BEGIN

class VFrameSmooth
{
public:
    VFrameSmooth(bool async,hmdInfoInternal_t	hmdInfo);
    ~VFrameSmooth();

    void	doSmooth( const ovrTimeWarpParms & parms );

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VFrameSmooth)
};


NV_NAMESPACE_END


