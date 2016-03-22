#pragma once

#include <vglobal.h>

NV_NAMESPACE_BEGIN

class VSignal
{
    enum
    {
        Infinite = 0xFFFFFFFF
    };

public:
    VSignal(bool state = false);
    ~VSignal();

    // Wait on an event condition until it is set
    // Delay is specified in milliseconds (1/1000 of a second).
    bool wait(uint delay = Infinite);

    // Set an event, releasing objects waiting on it
    void set();

    // Reset an event, un-signaling it
    void reset();

    // Set and then reset an event once a waiter is released.
    // If threads are already waiting, they will be notified and released
    // If threads are not waiting, the event is set until the first thread comes in
    void pulse();

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VSignal)
};

NV_NAMESPACE_END
