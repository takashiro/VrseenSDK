/************************************************************************************

Filename    :   OVR_Android_DeviceManager.h
Content     :   Android implementation of DeviceManager.
Created     :
Authors     :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "Android_DeviceManager.h"

// Sensor & HMD Factories
#include "LatencyTestDeviceImpl.h"
#include "SensorDeviceImpl.h"
#include "Android_HIDDevice.h"
#include "Android_HMDDevice.h"

#include "VTimer.h"
#include "Log.h"

#include <jni.h>
#include <memory>

jobject gRiftconnection;

namespace NervGear { namespace Android {

//-------------------------------------------------------------------------------------
// **** Android::DeviceManager

DeviceManager::DeviceManager()
{
}

DeviceManager::~DeviceManager()
{
}

bool DeviceManager::initialize(DeviceBase*)
{
    if (!DeviceManagerImpl:: initialize(0))
        return false;

    pThread = new DeviceManagerThread;
    if (!pThread || !pThread->start())
        return false;

    // Wait for the thread to be fully up and running.
    pThread->m_startupEvent.wait();

    // Do this now that we know the thread's run loop.
    HidDeviceManager = *HIDDeviceManager::CreateInternal(this);

    pCreateDesc->pDevice = this;
    LogText("NervGear::DeviceManager - initialized.\n");
    return true;
}

void DeviceManager::shutdown()
{
    LogText("NervGear::DeviceManager - shutting down.\n");

    // Set Manager shutdown marker variable; this prevents
    // any existing DeviceHandle objects from accessing device.
    pCreateDesc->pLock->pManager = 0;

    // Push for thread shutdown *WITH NO WAIT*.
    // This will have the following effect:
    //  - Exit command will get enqueued, which will be executed later on the thread itself.
    //  - Beyond this point, this DeviceManager object may be deleted by our caller.
    //  - Other commands, such as CreateDevice, may execute before ExitCommand, but they will
    //    fail gracefully due to pLock->pManager == 0. Future commands can't be enqued
    //    after pManager is null.
    //  - Once ExitCommand executes, ThreadCommand::Run loop will exit and release the last
    //    reference to the thread object.
    pThread->PushExitCommand(false);
    // pThread.Clear();

    DeviceManagerImpl::shutdown();
}

ThreadCommandQueue* DeviceManager::threadQueue()
{
    return pThread;
}

uint DeviceManager::threadId() const
{
    return pThread->id();
}

int DeviceManager::threadTid() const
{
    return pThread->threadTid();
}

void DeviceManager::suspendThread() const
{
    pThread->suspendThread();
}

void DeviceManager::resumeThread() const
{
    pThread->resumeThread();
}

bool DeviceManager::getDeviceInfo(DeviceInfo* info) const
{
    if ((info->InfoClassType != Device_Manager) &&
        (info->InfoClassType != Device_None))
        return false;

    info->Type    = Device_Manager;
    info->Version = 0;
    info->ProductName = "DeviceManager";
    info->Manufacturer = "Oculus VR, LLC";
    return true;
}

DeviceEnumerator<> DeviceManager::enumerateDevicesEx(const DeviceEnumerationArgs& args)
{
    // TBD: Can this be avoided in the future, once proper device notification is in place?
    pThread->PushCall((DeviceManagerImpl*)this,
                      &DeviceManager::EnumerateAllFactoryDevices, true);

    return DeviceManagerImpl::enumerateDevicesEx(args);
}


//-------------------------------------------------------------------------------------
// ***** DeviceManager Thread

DeviceManagerThread::DeviceManagerThread()
    : VThread(ThreadStackSize),
      m_suspend( false )
{
    int result = pipe(m_commandFd);
	OVR_UNUSED( result );	// no warning
    OVR_ASSERT(!result);

    addSelectFd(NULL, m_commandFd[0]);
}

DeviceManagerThread::~DeviceManagerThread()
{
    if (m_commandFd[0])
    {
        removeSelectFd(NULL, m_commandFd[0]);
        close(m_commandFd[0]);
        close(m_commandFd[1]);
    }
}

bool DeviceManagerThread::addSelectFd(Notifier* notify, int fd)
{
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN|POLLHUP|POLLERR;
    pfd.revents = 0;

    m_fdNotifiers.append(notify);
    m_pollFds.append(pfd);

    OVR_ASSERT(m_fdNotifiers.size() == m_pollFds.size());
    LogText( "DeviceManagerThread::AddSelectFd %d (Tid=%d)\n", fd, threadTid() );
    return true;
}

bool DeviceManagerThread::removeSelectFd(Notifier* notify, int fd)
{
    // [0] is reserved for thread commands with notify of null, but we still
    // can use this function to remove it.

    LogText( "DeviceManagerThread::RemoveSelectFd %d (Tid=%d)\n", fd, threadTid() );
    for (uint i = 0; i < m_fdNotifiers.size(); i++)
    {
        if ((m_fdNotifiers[i] == notify) && (m_pollFds[i].fd == fd))
        {
            m_fdNotifiers.removeAt(i);
            m_pollFds.removeAt(i);
            return true;
        }
    }
    LogText( "DeviceManagerThread::RemoveSelectFd failed %d (Tid=%d)\n", fd, threadTid() );
    return false;
}

static int event_count = 0;
static double event_time = 0;

int DeviceManagerThread::run()
{
    ThreadCommand::PopBuffer command;

    setName("NVDeviceMgr");

    LogText( "DeviceManagerThread - running (Tid=%d).\n", threadTid() );

    // needed to set SCHED_FIFO
    m_deviceManagerTid = gettid();

    // Signal to the parent thread that initialization has finished.
    m_startupEvent.set();

    while(!IsExiting())
    {
        // PopCommand will reset event on empty queue.
        if (PopCommand(&command))
        {
            command.Execute();
        }
        else
        {
            bool commands = false;
            do
            {
                int waitMs = INT_MAX;

                // If devices have time-dependent logic registered, get the longest wait
                // allowed based on current ticks.
                if (!m_ticksNotifiers.isEmpty())
                {
                    double timeSeconds = VTimer::Seconds();
                    int    waitAllowed;

                    for (uint j = 0; j < m_ticksNotifiers.size(); j++)
                    {
                        waitAllowed = (int)(m_ticksNotifiers[j]->onTicks(timeSeconds) * 1000);
                        if (waitAllowed < (int)waitMs)
                        {
                            waitMs = waitAllowed;
                        }
                    }
                }

                nfds_t nfds = m_pollFds.size();
                if (m_suspend)
                {
                    // only poll for commands when device polling is suspended
                    nfds = Alg::Min(nfds, (nfds_t)1);
                    // wait no more than 100 milliseconds to allow polling of the devices to resume
                    // within 100 milliseconds to avoid any noticeable loss of head tracking
                    waitMs = Alg::Min(waitMs, 100);
                }

                // wait until there is data available on one of the devices or the timeout expires
                int n = poll(&m_pollFds[0], nfds, waitMs);

                if (n > 0)
                {
                    // Iterate backwards through the list so the ordering will not be
                    // affected if the called object gets removed during the callback
                    // Also, the HID data streams are located toward the back of the list
                    // and servicing them first will allow a disconnect to be handled
                    // and cleaned directly at the device first instead of the general HID monitor
                    for (int i = nfds - 1; i >= 0; i--)
                    {
                    	const short revents = m_pollFds[i].revents;

                    	// If there was an error or hangup then we continue, the read will fail, and we'll close it.
                    	if (revents & (POLLIN | POLLERR | POLLHUP))
                        {
                            if ( revents & POLLERR )
                            {
                                LogText( "DeviceManagerThread - poll error event %d (Tid=%d)\n", m_pollFds[i].fd, threadTid() );
                            }
                            if (m_fdNotifiers[i])
                            {
                                event_count++;
                                if ( event_count >= 500 )
                                {
                                    const double current_time = VTimer::Seconds();
                                    const int eventHz = (int)( event_count / ( current_time - event_time ) + 0.5 );
                                    LogText( "DeviceManagerThread - event %d (%dHz) (Tid=%d)\n", m_pollFds[i].fd, eventHz, threadTid() );
                                    event_count = 0;
                                    event_time = current_time;
                                }

                                m_fdNotifiers[i]->onEvent(i, m_pollFds[i].fd);
                            }
                            else if (i == 0) // command
                            {
                                char dummy[128];
                                read(m_pollFds[i].fd, dummy, 128);
                                commands = true;
                            }
                        }

                        if (revents != 0)
                        {
                            n--;
                            if (n == 0)
                            {
                                break;
                            }
                        }
                    }
                }
                else
                {
                    if ( waitMs > 1 && !m_suspend )
                    {
                        LogText( "DeviceManagerThread - poll(fds,%d,%d) = %d (Tid=%d)\n", nfds, waitMs, n, threadTid() );
                    }
                }
            } while (m_pollFds.size() > 0 && !commands);
        }
    }

    LogText( "DeviceManagerThread - exiting (Tid=%d).\n", threadTid() );
    return 0;
}

bool DeviceManagerThread::addTicksNotifier(Notifier* notify)
{
     m_ticksNotifiers.append(notify);
     return true;
}

bool DeviceManagerThread::removeTicksNotifier(Notifier* notify)
{
    for (uint i = 0; i < m_ticksNotifiers.size(); i++)
    {
        if (m_ticksNotifiers[i] == notify)
        {
            m_ticksNotifiers.removeAt(i);
            return true;
        }
    }
    return false;
}

void DeviceManagerThread::suspendThread()
{
    m_suspend = true;
    LogText( "DeviceManagerThread - Suspend = true\n" );
}

void DeviceManagerThread::resumeThread()
{
    m_suspend = false;
    LogText( "DeviceManagerThread - Suspend = false\n" );
}

} // namespace Android


//-------------------------------------------------------------------------------------
// ***** Creation


// Creates a new DeviceManager and initializes OVR.
std::shared_ptr<DeviceManager> DeviceManager::Create()
{
    if (!System::IsInitialized())
    {
        // Use custom message, since Log is not yet installed.
        OVR_DEBUG_STATEMENT(Log::GetDefaultLog()->
            LogMessage(Log_Debug, "DeviceManager::Create failed - NervGear::System not initialized"); );
        return 0;
    }

    std::shared_ptr<Android::DeviceManager> manager = std::make_shared<Android::DeviceManager>();
    if (manager != nullptr) {
        if (manager->initialize(0)) {
            manager->AddFactory(&LatencyTestDeviceFactory::GetInstance());
            manager->AddFactory(&SensorDeviceFactory::GetInstance());
            manager->AddFactory(&Android::HMDDeviceFactory::GetInstance());
        } else {
            manager.reset();
        }

    }

    return manager;
}


} // namespace NervGear

