#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

class VMutex;

class VWaitCondition
{
public:
    enum {
        Infinite = 0xFFFFFFFF
    };

    VWaitCondition();
    ~VWaitCondition();

    bool wait(VMutex *mutex, uint delay = Infinite);

    void notify();
    void notifyAll();

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VWaitCondition)
};

NV_NAMESPACE_END
