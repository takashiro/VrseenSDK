#pragma once

#include "../vglobal.h"
#include "VDevice.h"
#include "VKernel.h"

NV_NAMESPACE_BEGIN

class VFrameSmooth
{
public:

    /* When vsync signal comes, there are two directions for hardware to refresh screen */
    enum HardwareRefreshDirection {
        HWRD_TB,        /* from top to bottom */
        HWRD_BT         /* from bottom to top */
    };

    VFrameSmooth(bool wantSingleBuffer, HardwareRefreshDirection direction);
    ~VFrameSmooth();

    void	doSmooth( const VTimeWarpParms & parms );
    int threadId() const;

    void pause();
    void setupSurface(EGLSurface surface);
    void resume();
private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VFrameSmooth)
};


NV_NAMESPACE_END
