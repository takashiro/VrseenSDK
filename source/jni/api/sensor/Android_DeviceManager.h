#pragma once

#include "vglobal.h"
#include "DeviceImpl.h"
#include "VThread.h"

#include <unistd.h>
#include <sys/poll.h>

NV_NAMESPACE_BEGIN

namespace Android {

class DeviceManagerThread;

//-------------------------------------------------------------------------------------
// ***** Android DeviceManager

class DeviceManager : public DeviceManagerImpl
{
public:
    DeviceManager();
    ~DeviceManager();

    // Initialize/Shutdowncreate and shutdown manger thread.
    bool initialize(DeviceBase* parent) override;
    void shutdown() override;

    ThreadCommandQueue* threadQueue() override;
    uint threadId() const override;
    int threadTid() const override;
    void suspendThread() const override;
    void resumeThread() const override;

    DeviceEnumerator<> enumerateDevicesEx(const DeviceEnumerationArgs& args) override;

    bool getDeviceInfo(DeviceInfo* info) const override;

    DeviceManagerThread *pThread;
};

//-------------------------------------------------------------------------------------
// ***** Device Manager Background Thread

class DeviceManagerThread : public VThread, public ThreadCommandQueue
{
    friend class DeviceManager;
    enum { ThreadStackSize = 64 * 1024 };
public:
    DeviceManagerThread();
    ~DeviceManagerThread();

    // ThreadCommandQueue notifications for CommandEvent handling.
    void onPushNonEmptyLocked() override { write(m_commandFd[1], this, 1); }
    void onPopEmptyLocked() override { }

    class Notifier
    {
    public:
        // Called when I/O is received
        virtual void onEvent(int i, int fd) = 0;

        // Called when timing ticks are updated.
        // Returns the largest number of seconds this function can
        // wait till next call.
        virtual double  onTicks(double tickSeconds)
        {
            OVR_UNUSED1(tickSeconds);
            return 1000;
        }
    };

    // Add I/O notifier
    bool addSelectFd(Notifier* notify, int fd);
    bool removeSelectFd(Notifier* notify, int fd);

    // Add notifier that will be called at regular intervals.
    bool addTicksNotifier(Notifier* notify);
    bool removeTicksNotifier(Notifier* notify);

    int threadTid() { return m_deviceManagerTid; }
    void suspendThread();
    void resumeThread();

protected:
    int run() override;

private:

    bool threadInitialized() { return m_commandFd[0] != 0; }

    pid_t                   m_deviceManagerTid;	// needed to set SCHED_FIFO

    // pipe used to signal commands
    int m_commandFd[2];

    Array<struct pollfd>    m_pollFds;
    Array<Notifier*>        m_fdNotifiers;

    VEvent                  m_startupEvent;
    volatile bool           m_suspend;

    // Ticks notifiers - used for time-dependent events such as keep-alive.
    Array<Notifier*>        m_ticksNotifiers;
};

}

NV_NAMESPACE_END
