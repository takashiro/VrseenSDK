#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

class VTimer
{
public:
    static double Seconds();
    static ulonglong TicksNanos();
    static uint TicksMs() { return  uint(TicksNanos() / 1000000); }
};

NV_NAMESPACE_END
