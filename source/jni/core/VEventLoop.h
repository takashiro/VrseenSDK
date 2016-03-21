#pragma once

#include "VEvent.h"

NV_NAMESPACE_BEGIN

class VEventLoop
{
public:
    VEventLoop(int capacity);
    ~VEventLoop();

    void quit();

    //Send out an event
    void post(const VEvent &event);
    void post(const char *command);
    void postf(const char *format, ...);

    //Send out an event and wait until it is proceeded
    void send(const VEvent &event);
    void send(const char *command);
    void sendf(const char *format, ...);

    void wait();
    void clear();

    const char *nextMessage();
    VEvent next();

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VEventLoop)
};

NV_NAMESPACE_END
