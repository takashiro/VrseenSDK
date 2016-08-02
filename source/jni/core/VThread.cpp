#include "VThread.h"
#include "VMutex.h"
#include "VWaitCondition.h"
#include "VLog.h"

#include <atomic>
#include <set>
#include <pthread.h>
#include <unistd.h>

NV_NAMESPACE_BEGIN

namespace {

class VThreadList
{
public:
    void add(VThread *pthread)
    {
        VMutex::Locker lock(&m_threadMutex);
        m_threadSet.insert(pthread);
    }

    void remove(VThread *pthread)
    {
        VMutex::Locker lock(&m_threadMutex);
        m_threadSet.erase(pthread);
        delete pthread;
        if (m_threadSet.size() == 0)
            m_threadsEmpty.notify();
    }

    void finishAllThreads()
    {
        // Only original root thread can call this.
        vAssert(pthread_self() == m_rootThreadId);

        VMutex::Locker lock(&m_threadMutex);
        while (m_threadSet.size() != 0)
            m_threadsEmpty.wait(&m_threadMutex);
    }

    VThreadList() { m_rootThreadId = pthread_self(); }
    ~VThreadList() {}

    VThread *find(uint id) const
    {
        for (VThread *thread : m_threadSet) {
            if (thread->id() == id) {
                return thread;
            }
        }
        vWarn("Not in a VThread");
        return nullptr;
    }

private:
    std::set<VThread *> m_threadSet;
    VMutex m_threadMutex;
    VWaitCondition m_threadsEmpty;

    // Track the root thread that created us.
    pthread_t m_rootThreadId;
};

} //anonymous namespace

struct VThread::Private
{
    VThread *self;
    VThread::Function function;
    void *data;
    uint stackSize;
    int processor;
    VThread::State state;
    VThread::Priority priority;
    int exitCode;

    // Thread state flags
    std::atomic<uint> threadFlags;
    std::atomic<int> suspendCount;

    pthread_t handle;

    static int InitDefaultAttr;
    static pthread_attr_t DefaultAttr;
    static VThreadList threadList;

    VMutex exitMutex;

    Private(VThread *self)
        : self(self)
        , function(nullptr)
        , data(nullptr)
        , stackSize(128 * 1024)
        , processor(-1)
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
        threadList.remove(d->self);
        return (void *) result;
    }
};

int VThread::Private::InitDefaultAttr = 0;
pthread_attr_t VThread::Private::DefaultAttr;
VThreadList VThread::Private::threadList;

VThread::VThread(uint stackSize, int processor)
    : d(new Private(this))
{
    d->stackSize = stackSize;
    d->processor = processor;
}

VThread::VThread(VThread::Function function, void *data, uint stackSize, int processor, State state)
    : d(new Private(this))
{
    d->function = function;
    d->data = data;
    d->stackSize = stackSize;
    d->processor = processor;
    d->state = state;
}

VThread::~VThread()
{
    delete d;
}

void VThread::setName(const char *name)
{
    int result = pthread_setname_np(d->handle, name);
    if ( result != 0) {
        vWarn("VThread::setName(\"" << name << "\") failed. Error:" << strerror(result));
    }
}

bool VThread::start(VThread::State initialState)
{
    if (initialState == NotRunning) {
        return false;
    }
    if (state() != NotRunning) {
        vWarn("Thread::Start failed - thread already running" << id());
        return false;
    }

    if (!d->InitDefaultAttr) {
        pthread_attr_init(&d->DefaultAttr);
        pthread_attr_setdetachstate(&d->DefaultAttr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&d->DefaultAttr, 128 * 1024);
        sched_param sparam;
        sparam.sched_priority = VThread::GetOSPriority(NormalPriority);
        pthread_attr_setschedparam(&d->DefaultAttr, &sparam);
        d->InitDefaultAttr = 1;
    }

    d->exitCode = 0;
    d->suspendCount = 0;
    d->threadFlags = (initialState == Running) ? 0 : Private::Suspended;

    // AddRef to us until the thread is finished
    // AddRef();
    d->threadList.add(this);
    d->exitMutex.lock();

    int result;
    if (d->stackSize != 128 * 1024 || d->priority != NormalPriority) {
        pthread_attr_t attr;

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&attr, d->stackSize);
        sched_param sparam;
        sparam.sched_priority = GetOSPriority(d->priority);
        pthread_attr_setschedparam(&attr, &sparam);
        result = pthread_create(&d->handle, &attr, Private::StartFunction, d);
        pthread_attr_destroy(&attr);
    } else {
        result = pthread_create(&d->handle, &d->DefaultAttr, Private::StartFunction, d);
    }

    if (result) {
        d->threadFlags = 0;
        //Release();
        d->threadList.remove(this);
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
    onExit();

    // Signal this thread object as done and release it's references.
    d->threadFlags &= ~(uint) Private::Started;
    d->threadFlags |= Private::Finished;
    // Release();

    d->threadList.remove(this);

    d->exitMutex.unlock();
    pthread_exit((void *) exitCode);
}

bool VThread::wait()
{
    d->exitMutex.lock();
    d->exitMutex.unlock();
    return true;
}

void VThread::onExit()
{
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

bool VThread::exitFlag() const
{
    return (d->threadFlags & Private::Exited) != 0;
}

void VThread::setExitFlag(bool exitFlag)
{
    // The below is atomic since ThreadFlags is AtomicInt.
    if (exitFlag) {
        d->threadFlags |= Private::Exited;
    } else {
        d->threadFlags &= (uint) ~Private::Exited;
    }
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

int VThread::exitCode() const
{
    return d->exitCode;
}

uint VThread::id() const
{
    return (uint) d->handle;
}

void VThread::FinishAllThreads()
{
    Private::threadList.finishAllThreads();
}

int VThread::CpuCount()
{
    return 1;
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
    return Private::threadList.find(currentThreadId());
}

uint VThread::currentThreadId()
{
    return (uint) pthread_self();
}

NV_NAMESPACE_END
