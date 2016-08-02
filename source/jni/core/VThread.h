#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

class VThread
{
public:
    typedef int (*Function)(void *data);

    enum State
    {
        NotRunning,
        Running,
        Suspended
    };

    enum Priority
    {
        CriticalPriority,
        HighestPriority,
        AboveNormalPriority,
        NormalPriority,
        BelowNormalPriority,
        LowestPriority,
        IdlePriority,
    };

    VThread();
    VThread(Function function, void *data = nullptr);

    virtual ~VThread();

    uint stackSize() const;
    void setStackSize(uint size);

    Priority priority() const;
    void setPriority(Priority priority);

    void setName(const char *name);

    virtual bool start();
    virtual void exit(int exitCode = 0);

    bool wait();

    bool suspend();
    bool resume();

    bool isFinished() const;
    bool isSuspended() const;
    State state() const;
    uint id() const;

    static int CpuCount();
    static int GetOSPriority(Priority priority);

    static bool Sleep(uint secs);
    static bool MSleep(uint msecs);

    static VThread *currentThread();
    static uint currentThreadId();

protected:
    virtual int run();

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VThread)
};

NV_NAMESPACE_END
