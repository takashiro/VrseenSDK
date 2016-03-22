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
    void post(const VString &command, const VVariant &data);
    void post(const char *command);

    //Send out an event and wait until it is proceeded
    void send(const VEvent &event);
    void send(const VString &command, const VVariant &data);
    void send(const char *command);

    void wait();
    void clear();

    VEvent next();

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VEventLoop)
};

NV_NAMESPACE_END
