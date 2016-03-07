#include "VTimer.h"
#include "VLog.h"

#include <time.h>

NV_NAMESPACE_BEGIN

double VTimer::Seconds()
{
    return double(VTimer::TicksNanos()) * 0.000000001;
}

ulonglong VTimer::TicksNanos()
{
    struct timespec tp;
    const int status = clock_gettime(CLOCK_MONOTONIC, &tp);

    if (status != 0) {
        vDebug("clock_gettime status =" << status);
    }
    return (ulonglong) tp.tv_sec * (ulonglong) (1000 * 1000 * 1000) + ulonglong(tp.tv_nsec);
}

NV_NAMESPACE_END
