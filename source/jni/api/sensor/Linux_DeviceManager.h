#pragma once

#include "vglobal.h"

#include "DeviceImpl.h"

#include <unistd.h>
#include <sys/poll.h>


namespace NervGear { namespace Linux {

class DeviceManagerThread;

//-------------------------------------------------------------------------------------
// ***** Linux DeviceManager

class DeviceManager : public DeviceManagerImpl
{
public:
    DeviceManager();
    ~DeviceManager();

    // Initialize/Shutdowncreate and shutdown manger thread.
    virtual bool Initialize(DeviceBase* parent);
    virtual void Shutdown();

    virtual ThreadCommandQueue* GetThreadQueue();
    virtual ThreadId GetThreadId() const;
    virtual int GetThreadTid() const;
    virtual void SuspendThread() const;
    virtual void ResumeThread() const;

    virtual DeviceEnumerator<> EnumerateDevicesEx(const DeviceEnumerationArgs& args);

    virtual bool  GetDeviceInfo(DeviceInfo* info) const;

    Ptr<DeviceManagerThread> pThread;
};

//-------------------------------------------------------------------------------------
// ***** Device Manager Background Thread

class DeviceManagerThread : public Thread, public ThreadCommandQueue
{
    friend class DeviceManager;
    enum { ThreadStackSize = 64 * 1024 };
public:
    DeviceManagerThread();
    ~DeviceManagerThread();

    virtual int Run();

    // ThreadCommandQueue notifications for CommandEvent handling.
    virtual void OnPushNonEmpty_Locked() { write(CommandFd[1], this, 1); }
    virtual void OnPopEmpty_Locked()     { }

    class Notifier
    {
    public:
        // Called when I/O is received
        virtual void OnEvent(int i, int fd) = 0;

        // Called when timing ticks are updated.
        // Returns the largest number of seconds this function can
        // wait till next call.
        virtual double  OnTicks(double tickSeconds)
        {
            NV_UNUSED(tickSeconds);
            return 1000.0;
        }
    };

    // Add I/O notifier
    bool AddSelectFd(Notifier* notify, int fd);
    bool RemoveSelectFd(Notifier* notify, int fd);

    // Add notifier that will be called at regular intervals.
    bool AddTicksNotifier(Notifier* notify);
    bool RemoveTicksNotifier(Notifier* notify);

private:

    bool threadInitialized() { return CommandFd[0] != 0; }

    // pipe used to signal commands
    int CommandFd[2];

    VArray<struct pollfd>    PollFds;
    VArray<Notifier*>        FdNotifiers;

    Event                   StartupEvent;

    // Ticks notifiers - used for time-dependent events such as keep-alive.
    VArray<Notifier*>        TicksNotifiers;
};

}} // namespace Linux::OVR


