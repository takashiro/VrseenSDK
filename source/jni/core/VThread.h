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

    VThread(uint stackSize = 128 * 1024, int processor = -1);
    VThread(Function function, void *data = nullptr, uint stackSize = 128 * 1024, int processor = -1, State state = NotRunning);

    virtual ~VThread();

    void setName(const char *name);

    virtual bool start(State initialState = Running);
    virtual void exit(int exitCode = 0);

    int wait();

    bool suspend();
    bool resume();

    bool exitFlag() const;
    void setExitFlag(bool exitFlag);

    bool isFinished() const;
    bool isSuspended() const;
    State state() const;
    int exitCode() const;
    uint id() const;

    static void InitThreadList();
    static void FinishAllThreads();
    static int CpuCount();
    static int GetOSPriority(Priority priority);

    static bool Sleep(uint secs);
    static bool MSleep(uint msecs);

    static VThread *currentThread();
    static uint currentThreadId();

protected:
    virtual int run();
    virtual void onExit();

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VThread)
};

NV_NAMESPACE_END
