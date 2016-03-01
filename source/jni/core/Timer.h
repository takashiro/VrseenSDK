#pragma once

#include "vglobal.h"

#include "Types.h"

NV_NAMESPACE_BEGIN

//-----------------------------------------------------------------------------------
// ***** Timer

// Timer class defines a family of static functions used for application
// timing and profiling.

class Timer
{
public:
    enum {
        MsPerSecond     = 1000, // Milliseconds in one second.
        NanosPerSecond  = MsPerSecond * 1000 * 1000
    };

    // ***** Timing APIs for Application

    // These APIs should be used to guide animation and other program functions
    // that require precision.

    // Returns global high-resolution application timer in seconds.
    static double  OVR_STDCALL GetSeconds();

    // Returns time in Nanoseconds, using highest possible system resolution.
    static UInt64  OVR_STDCALL GetTicksNanos();

    // Kept for compatibility.
    // Returns ticks in milliseconds, as a 32-bit number. May wrap around every 49.2 days.
    // Use either time difference of two values of GetTicks to avoid wrap-around.
    static UInt32  OVR_STDCALL GetTicksMs()
    { return  UInt32(GetTicksNanos() / 1000000); }

private:
    friend class System;
    // System called during program startup/shutdown.
    static void InitializeTimerSystem();
    static void ShutdownTimerSystem();
};

NV_NAMESPACE_END
