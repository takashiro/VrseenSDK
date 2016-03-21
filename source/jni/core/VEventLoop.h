#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

class VEventLoop
{
public:
    VEventLoop(int capacity);
    ~VEventLoop();

    void quit();

    //Send out an event
    void post(const char * msg);
    void postf(const char * fmt, ...);

    //Send out an event and wait until it is proceeded
    void send(const char * msg);
    void sendf(const char * fmt, ...);

    void wait();
    void clear();

    const char *nextMessage();

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VEventLoop)
};

NV_NAMESPACE_END
