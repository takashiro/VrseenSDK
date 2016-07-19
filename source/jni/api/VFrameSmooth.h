#pragma once

#include "../vglobal.h"
#include "VDevice.h"
#include "VKernel.h"

NV_NAMESPACE_BEGIN

class VFrameSmooth
{
public:
    VFrameSmooth(bool async,bool wantSingleBuffer);
    ~VFrameSmooth();

    void	doSmooth( const ovrTimeWarpParms & parms );
    int threadId() const;
private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VFrameSmooth)
};


NV_NAMESPACE_END


