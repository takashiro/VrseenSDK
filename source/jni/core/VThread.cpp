#include "VThread.h"
#include "VMap.h"
#include "VMutex.h"
#include "VLog.h"

#include <atomic>
#include <unistd.h>
#include <pthread.h>

NV_NAMESPACE_BEGIN

namespace {

class VThreadList
{
public:
    void add(VThread *thread)
    {
        if (thread) {
            m_mutex.lock();
            m_pool.insert(thread->id(), thread);
            m_mutex.unlock();
        }
    }

    void remove(VThread *thread)
    {
        if (thread) {
            m_mutex.lock();
            m_pool.erase(thread->id());
            m_mutex.unlock();
        }
    }

    VThread *find(uint id) const
    {
        m_mutex.lock();
        VThread *thread = m_pool.value(id);
        m_mutex.unlock();
        return thread;
    }

private:
    VMap<uint, VThread *> m_pool;
    mutable VMutex m_mutex;
};

} //anonymous namespace

struct VThread::Private
{
    VThread *self;
    VThread::Function function;
    void *data;
    uint stackSize;
    VThread::State state;
    VThread::Priority priority;
    int exitCode;

    // Thread state flags
    std::atomic<uint> threadFlags;
    std::atomic<int> suspendCount;

    pthread_t handle;

    static VThreadList pool;

    VMutex exitMutex;

    Private(VThread *self)
        : self(self)
        , function(nullptr)
        , data(nullptr)
        , stackSize(128 * 1024)
        , state(VThread::NotRunning)
        , priority(VThread::NormalPriority)
        , exitCode(0)
        , threadFlags(0)
        , suspendCount(0)
        , handle(0)
    {
    }

    enum StateFlag
    {
        Started = 0x01,
        Finished = 0x02,
        Suspended = 0x08,
        Exited = 0x10
    };

    int run()
    {
        // Suspend us on start, if requested
        if (threadFlags & Suspended) {
            self->suspend();
            threadFlags &= (uint) ~Suspended;
        }

        // Call the virtual run function
        exitCode = self->run();
        self->exit();
        return exitCode;
    }

    static void *StartFunction(void *data)
    {
        Private *d = (Private *) data;
        int result = d->run();
        // Signal the thread as done and release it atomically.
        d->threadFlags &= ~(uint) Private::Started;
        d->threadFlags |= Private::Finished;
        // d->self->Release();
        // At this point Thread object might be dead; however we can still pass
        // it to RemoveRunningThread since it is only used as a key there.
        pool.remove(d->self);
        return (void *) result;
    }
};

VThreadList VThread::Private::pool;

VThread::VThread()
    : d(new Private(this))
{
}

VThread::VThread(VThread::Function function, void *data)
    : d(new Private(this))
{
    d->function = function;
    d->data = data;
}

VThread::~VThread()
{
    delete d;
}

uint VThread::stackSize() const
{
    return d->stackSize;
}

void VThread::setStackSize(uint size)
{
    d->stackSize = size;
}

VThread::Priority VThread::priority() const
{
    return d->priority;
}

void VThread::setPriority(VThread::Priority priority)
{
    d->priority = priority;
}

void VThread::setName(const char *name)
{
    int result = pthread_setname_np(d->handle, name);
    if (result != 0) {
        vWarn("VThread::setName(\"" << name << "\") failed. Error:" << strerror(result));
    }
}

bool VThread::start()
{
    if (state() != NotRunning) {
        vWarn("Thread::Start failed - thread already running" << id());
        return false;
    }

    d->exitCode = 0;
    d->suspendCount = 0;
    d->threadFlags = 0;
    d->exitMutex.lock();

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, d->stackSize);
    sched_param sparam;
    sparam.sched_priority = VThread::GetOSPriority(d->priority);
    pthread_attr_setschedparam(&attr, &sparam);

    int result = pthread_create(&d->handle, &attr, Private::StartFunction, d);
    pthread_attr_destroy(&attr);
    if (result) {
        d->threadFlags = 0;
        return false;
    }

    return true;
}

int VThread::run()
{
    return (d->function) ? d->function(d->data) : 0;
}

void VThread::exit(int exitCode)
{
    // Signal this thread object as done and release it's references.
    d->threadFlags &= ~(uint) Private::Started;
    d->threadFlags |= Private::Finished;
    // Release();

    d->pool.remove(this);

    d->exitMutex.unlock();
    pthread_exit((void *) exitCode);
}

bool VThread::wait()
{
    d->exitMutex.lock();
    d->exitMutex.unlock();
    return true;
}

bool VThread::suspend()
{
    vWarn("Thread::Suspend - cannot suspend threads on this system");
    return 0;
}

bool VThread::resume()
{
    vWarn("Thread::Suspend - cannot resume threads on this system");
    return 0;
}

bool VThread::isFinished() const
{
    return (d->threadFlags & Private::Finished) != 0;
}

bool VThread::isSuspended() const
{
    return d->suspendCount > 0;
}

VThread::State VThread::state() const
{
    if (isSuspended())
        return Suspended;
    if (d->threadFlags & Private::Started)
        return Running;
    return NotRunning;
}

uint VThread::id() const
{
    return (uint) d->handle;
}

int VThread::GetOSPriority(VThread::Priority priority)
{
    const int minPriority = sched_get_priority_min(SCHED_NORMAL);
    const int maxPriority = sched_get_priority_max(SCHED_NORMAL);
    switch(priority)
    {
    case CriticalPriority: return minPriority + (maxPriority - minPriority) * 7 / 8;
    case HighestPriority: return minPriority + (maxPriority - minPriority) * 6 / 8;
    case AboveNormalPriority: return minPriority + (maxPriority - minPriority) * 5 / 8;
    case NormalPriority: return minPriority + (maxPriority - minPriority) * 4 / 8;
    case BelowNormalPriority: return minPriority + (maxPriority - minPriority) * 3 / 8;
    case LowestPriority: return minPriority + (maxPriority - minPriority) * 2 / 8;
    case IdlePriority: return minPriority + (maxPriority - minPriority) * 1 / 8;
    default: return minPriority + (maxPriority - minPriority) * 4 / 8;
    }
}

bool VThread::Sleep(uint secs)
{
    sleep(secs);
    return true;
}

bool VThread::MSleep(uint msecs)
{
    usleep(msecs * 1000);
    return true;
}

VThread *VThread::currentThread()
{
    return Private::pool.find(currentThreadId());
}

uint VThread::currentThreadId()
{
    return (uint) pthread_self();
}

NV_NAMESPACE_END
