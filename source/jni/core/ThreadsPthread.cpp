/************************************************************************************

Filename    :   OVR_ThreadsPthread.cpp
Content     :
Created     :
Notes       :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#include "Threads.h"
#include "Hash.h"

#ifdef OVR_ENABLE_THREADS

#include "Timer.h"
#include "Log.h"

#include <pthread.h>
#include <time.h>

#ifdef OVR_OS_PS3
#include <sys/sys_time.h>
#include <sys/timer.h>
#include <sys/synchronization.h>
#define sleep(x) sys_timer_sleep(x)
#define usleep(x) sys_timer_usleep(x)
using std::timespec;
#else
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#endif

#ifdef ANDROID
#include <android/log.h>
#endif

namespace NervGear {

// ***** Mutex implementation


// *** Internal Mutex implementation structure

class MutexImpl : public NewOverrideBase
{
    // System mutex or semaphore
    pthread_mutex_t   SMutex;
    bool          Recursive;
    unsigned      LockCount;
    pthread_t     LockedBy;

    friend class WaitConditionImpl;

public:
    // Constructor/destructor
    MutexImpl(Mutex* pmutex, bool recursive = 1);
    ~MutexImpl();

    // Locking functions
    void                DoLock();
    bool                TryLock();
    void                Unlock(Mutex* pmutex);
    // Returns 1 if the mutes is currently locked
    bool                IsLockedByAnotherThread(Mutex* pmutex);
    bool                IsSignaled() const;
};

pthread_mutexattr_t Lock::RecursiveAttr;
bool Lock::RecursiveAttrInit = 0;

// *** Constructor/destructor
MutexImpl::MutexImpl(Mutex* pmutex, bool recursive)
{
    Recursive           = recursive;
    LockCount           = 0;

    if (Recursive)
    {
        if (!Lock::RecursiveAttrInit)
        {
            pthread_mutexattr_init(&Lock::RecursiveAttr);
            pthread_mutexattr_settype(&Lock::RecursiveAttr, PTHREAD_MUTEX_RECURSIVE);
            Lock::RecursiveAttrInit = 1;
        }

        pthread_mutex_init(&SMutex, &Lock::RecursiveAttr);
    }
    else
        pthread_mutex_init(&SMutex, 0);
}

MutexImpl::~MutexImpl()
{
    pthread_mutex_destroy(&SMutex);
}


// Lock and try lock
void MutexImpl::DoLock()
{
    while (pthread_mutex_lock(&SMutex));
    LockCount++;
    LockedBy = pthread_self();
}

bool MutexImpl::TryLock()
{
    if (!pthread_mutex_trylock(&SMutex))
    {
        LockCount++;
        LockedBy = pthread_self();
        return 1;
    }

    return 0;
}

void MutexImpl::Unlock(Mutex* pmutex)
{
    OVR_ASSERT(pthread_self() == LockedBy && LockCount > 0);

    LockCount--;

    pthread_mutex_unlock(&SMutex);
}

bool    MutexImpl::IsLockedByAnotherThread(Mutex* pmutex)
{
    // There could be multiple interpretations of IsLocked with respect to current thread
    if (LockCount == 0)
        return 0;
    if (pthread_self() != LockedBy)
        return 1;
    return 0;
}

bool    MutexImpl::IsSignaled() const
{
    // An mutex is signaled if it is not locked ANYWHERE
    // Note that this is different from IsLockedByAnotherThread function,
    // that takes current thread into account
    return LockCount == 0;
}


// *** Actual Mutex class implementation

Mutex::Mutex(bool recursive)
{
    // NOTE: RefCount mode already thread-safe for all waitables.
    m_impl = new MutexImpl(this, recursive);
}

Mutex::~Mutex()
{
    delete m_impl;
}

// Lock and try lock
void Mutex::lock()
{
    m_impl->DoLock();
}
bool Mutex::tryLock()
{
    return m_impl->TryLock();
}
void Mutex::unlock()
{
    m_impl->Unlock(this);
}
bool    Mutex::isLockedByAnotherThread()
{
    return m_impl->IsLockedByAnotherThread(this);
}



//-----------------------------------------------------------------------------------
// ***** Event

bool Event::wait(unsigned delay)
{
    Mutex::Locker lock(&m_stateMutex);

    // Do the correct amount of waiting
    if (delay == OVR_WAIT_INFINITE)
    {
        while(!m_state)
            m_stateWaitCondition.wait(&m_stateMutex);
    }
    else if (delay)
    {
        if (!m_state)
            m_stateWaitCondition.wait(&m_stateMutex, delay);
    }

    bool state = m_state;
    // Take care of temporary 'pulsing' of a state
    if (m_temporary)
    {
        m_temporary   = false;
        m_state       = false;
    }
    return state;
}

void Event::updateState(bool newState, bool newTemp, bool mustNotify)
{
    Mutex::Locker lock(&m_stateMutex);
    m_state       = newState;
    m_temporary   = newTemp;
    if (mustNotify)
        m_stateWaitCondition.notifyAll();
}



// ***** Wait Condition Implementation

// Internal implementation class
class WaitConditionImpl : public NewOverrideBase
{
    pthread_mutex_t     SMutex;
    pthread_cond_t      Condv;

public:

    // Constructor/destructor
    WaitConditionImpl();
    ~WaitConditionImpl();

    // Release mutex and wait for condition. The mutex is re-aqured after the wait.
    bool    Wait(Mutex *pmutex, unsigned delay = OVR_WAIT_INFINITE);

    // Notify a condition, releasing at one object waiting
    void    Notify();
    // Notify a condition, releasing all objects waiting
    void    NotifyAll();
};


WaitConditionImpl::WaitConditionImpl()
{
    pthread_mutex_init(&SMutex, 0);
    pthread_cond_init(&Condv, 0);
}

WaitConditionImpl::~WaitConditionImpl()
{
    pthread_mutex_destroy(&SMutex);
    pthread_cond_destroy(&Condv);
}

bool    WaitConditionImpl::Wait(Mutex *pmutex, unsigned delay)
{
    bool            result = 1;
    unsigned            lockCount = pmutex->m_impl->LockCount;

    // Mutex must have been locked
    if (lockCount == 0)
        return 0;

    pthread_mutex_lock(&SMutex);

    // Finally, release a mutex or semaphore
    if (pmutex->m_impl->Recursive)
    {
        // Release the recursive mutex N times
        pmutex->m_impl->LockCount = 0;
        for(unsigned i=0; i<lockCount; i++)
            pthread_mutex_unlock(&pmutex->m_impl->SMutex);
    }
    else
    {
        pmutex->m_impl->LockCount = 0;
        pthread_mutex_unlock(&pmutex->m_impl->SMutex);
    }

    // Note that there is a gap here between mutex.Unlock() and Wait().
    // The other mutex protects this gap.

    if (delay == OVR_WAIT_INFINITE)
        pthread_cond_wait(&Condv,&SMutex);
    else
    {
        timespec ts;
#ifdef OVR_OS_PS3
        sys_time_sec_t s;
        sys_time_nsec_t ns;
        sys_time_get_current_time(&s, &ns);

        ts.tv_sec = s + (delay / 1000);
        ts.tv_nsec = ns + (delay % 1000) * 1000000;

#else
        struct timeval tv;
        gettimeofday(&tv, 0);

        ts.tv_sec = tv.tv_sec + (delay / 1000);
        ts.tv_nsec = (tv.tv_usec + (delay % 1000) * 1000) * 1000;
#endif
        if (ts.tv_nsec > 999999999)
        {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }
        int r = pthread_cond_timedwait(&Condv,&SMutex, &ts);
        OVR_ASSERT(r == 0 || r == ETIMEDOUT);
        if (r)
            result = 0;
    }

    pthread_mutex_unlock(&SMutex);

    // Re-aquire the mutex
    for(unsigned i=0; i<lockCount; i++)
        pmutex->lock();

    // Return the result
    return result;
}

// Notify a condition, releasing the least object in a queue
void    WaitConditionImpl::Notify()
{
    pthread_mutex_lock(&SMutex);
    pthread_cond_signal(&Condv);
    pthread_mutex_unlock(&SMutex);
}

// Notify a condition, releasing all objects waiting
void    WaitConditionImpl::NotifyAll()
{
    pthread_mutex_lock(&SMutex);
    pthread_cond_broadcast(&Condv);
    pthread_mutex_unlock(&SMutex);
}



// *** Actual implementation of WaitCondition

WaitCondition::WaitCondition()
{
    m_impl = new WaitConditionImpl;
}
WaitCondition::~WaitCondition()
{
    delete m_impl;
}

bool    WaitCondition::wait(Mutex *pmutex, unsigned delay)
{
    return m_impl->Wait(pmutex, delay);
}
// Notification
void    WaitCondition::notify()
{
    m_impl->Notify();
}
void    WaitCondition::notifyAll()
{
    m_impl->NotifyAll();
}


// ***** Current thread

// Per-thread variable
/*
static __thread Thread* pCurrentThread = 0;

// Static function to return a pointer to the current thread
void    Thread::InitCurrentThread(Thread *pthread)
{
    pCurrentThread = pthread;
}

// Static function to return a pointer to the current thread
Thread*    Thread::GetThread()
{
    return pCurrentThread;
}
*/


// *** Thread constructors.

Thread::Thread(UPInt stackSize, int processor)
{
    // NOTE: RefCount mode already thread-safe for all Waitable objects.
    CreateParams params;
    params.stackSize = stackSize;
    params.processor = processor;
    init(params);
}

Thread::Thread(Thread::ThreadFn threadFunction, void*  userHandle, UPInt stackSize,
                 int processor, Thread::ThreadState initialState)
{
    CreateParams params(threadFunction, userHandle, stackSize, processor, initialState);
    init(params);
}

Thread::Thread(const CreateParams& params)
{
    init(params);
}

void Thread::init(const CreateParams& params)
{
    // Clear the variables
    m_threadFlags     = 0;
    m_threadHandle    = 0;
    m_exitCode        = 0;
    m_suspendCount    = 0;
    m_stackSize       = params.stackSize;
    m_processor       = params.processor;
    m_priority        = params.priority;

    // Clear Function pointers
    threadFunction  = params.threadFunction;
    userHandle      = params.userHandle;
    if (params.initialState != NotRunning)
        start(params.initialState);
}

Thread::~Thread()
{
    // Thread should not running while object is being destroyed,
    // this would indicate ref-counting issue.
    //OVR_ASSERT(IsRunning() == 0);

    // Clean up thread.
    m_threadHandle = 0;
}



// *** Overridable User functions.

// Default Run implementation
int    Thread::run()
{
    // Call pointer to function, if available.
    return (threadFunction) ? threadFunction(this, userHandle) : 0;
}
void    Thread::onExit()
{
}


// Finishes the thread and releases internal reference to it.
void    Thread::finishAndRelease()
{
    // Note: thread must be US.
    m_threadFlags &= (UInt32)~(OVR_THREAD_STARTED);
    m_threadFlags |= OVR_THREAD_FINISHED;

    // Release our reference; this is equivalent to 'delete this'
    // from the point of view of our thread.
    Release();
}



// *** ThreadList - used to track all created threads

class ThreadList : public NewOverrideBase
{
    //------------------------------------------------------------------------
    struct ThreadHashOp
    {
        size_t operator()(const Thread* ptr)
        {
            return (((size_t)ptr) >> 6) ^ (size_t)ptr;
        }
    };

    HashSet<Thread*, ThreadHashOp>        ThreadSet;
    Mutex                                 ThreadMutex;
    WaitCondition                         ThreadsEmpty;
    // Track the root thread that created us.
    pthread_t                             RootThreadId;

    static ThreadList* volatile pRunningThreads;

    void addThread(Thread *pthread)
    {
        Mutex::Locker lock(&ThreadMutex);
        ThreadSet.Add(pthread);
    }

    void removeThread(Thread *pthread)
    {
        Mutex::Locker lock(&ThreadMutex);
        ThreadSet.Remove(pthread);
        if (ThreadSet.GetSize() == 0)
            ThreadsEmpty.notify();
    }

    void finishAllThreads()
    {
        // Only original root thread can call this.
        OVR_ASSERT(pthread_self() == RootThreadId);

        Mutex::Locker lock(&ThreadMutex);
        while (ThreadSet.GetSize() != 0)
            ThreadsEmpty.wait(&ThreadMutex);
    }

public:

    ThreadList()
    {
        RootThreadId = pthread_self();
    }
    ~ThreadList() { }

    static void Init()
    {
        if (!pRunningThreads)
        {
            pRunningThreads = new ThreadList;
            OVR_ASSERT(pRunningThreads);
        }
    }

    static void AddRunningThread(Thread *pthread)
    {
        // Non-atomic creation ok since only the root thread
        if (!pRunningThreads)
        {
            pRunningThreads = new ThreadList;
            OVR_ASSERT(pRunningThreads);
        }
        pRunningThreads->addThread(pthread);
    }

    // NOTE: 'pthread' might be a dead pointer when this is
    // called so it should not be accessed; it is only used
    // for removal.
    static void RemoveRunningThread(Thread *pthread)
    {
        OVR_ASSERT(pRunningThreads);
        pRunningThreads->removeThread(pthread);
    }

    static void FinishAllThreads()
    {
        // This is ok because only root thread can wait for other thread finish.
        if (pRunningThreads)
        {
            pRunningThreads->finishAllThreads();
            delete pRunningThreads;
            pRunningThreads = 0;
        }
    }
};

// By default, we have no thread list.
ThreadList* volatile ThreadList::pRunningThreads = 0;

void Thread::InitThreadList()
{
	ThreadList::Init();
}

// FinishAllThreads - exposed publicly in Thread.
void Thread::FinishAllThreads()
{
    ThreadList::FinishAllThreads();
}

// *** Run override

int    Thread::_run()
{
    // Suspend us on start, if requested
    if (m_threadFlags & OVR_THREAD_START_SUSPENDED)
    {
        suspend();
        m_threadFlags &= (UInt32)~OVR_THREAD_START_SUSPENDED;
    }

    // Call the virtual run function
    m_exitCode = run();
    return m_exitCode;
}




// *** User overridables

bool    Thread::exitFlag() const
{
    return (m_threadFlags & OVR_THREAD_EXIT) != 0;
}

void    Thread::setExitFlag(bool exitFlag)
{
    // The below is atomic since ThreadFlags is AtomicInt.
    if (exitFlag)
        m_threadFlags |= OVR_THREAD_EXIT;
    else
        m_threadFlags &= (UInt32) ~OVR_THREAD_EXIT;
}


// Determines whether the thread was running and is now finished
bool    Thread::isFinished() const
{
    return (m_threadFlags & OVR_THREAD_FINISHED) != 0;
}
// Determines whether the thread is suspended
bool    Thread::isSuspended() const
{
    return m_suspendCount > 0;
}
// Returns current thread state
Thread::ThreadState Thread::threadState() const
{
    if (isSuspended())
        return Suspended;
    if (m_threadFlags & OVR_THREAD_STARTED)
        return Running;
    return NotRunning;
}

/*
static const char* mapsched_policy(int policy)
{
    switch(policy)
    {
    case SCHED_OTHER:
        return "SCHED_OTHER";
    case SCHED_RR:
        return "SCHED_RR";
    case SCHED_FIFO:
        return "SCHED_FIFO";

    }
    return "UNKNOWN";
}
    int policy;
    sched_param sparam;
    pthread_getschedparam(pthread_self(), &policy, &sparam);
    int max_prior = sched_get_priority_max(policy);
    int min_prior = sched_get_priority_min(policy);
    printf(" !!!! policy: %s, priority: %d, max priority: %d, min priority: %d\n", mapsched_policy(policy), sparam.sched_priority, max_prior, min_prior);
#include <stdio.h>
*/

// ***** Thread management

// The actual first function called on thread start
void* Thread_PthreadStartFn(void* phandle)
{
    Thread* pthread = (Thread*)phandle;
    int     result = pthread->_run();
    // Signal the thread as done and release it atomically.
    pthread->finishAndRelease();
    // At this point Thread object might be dead; however we can still pass
    // it to RemoveRunningThread since it is only used as a key there.
    ThreadList::RemoveRunningThread(pthread);
    return (void*) result;
}

int Thread::InitDefaultAttr = 0;
pthread_attr_t Thread::DefaultAttr;

/* static */
int Thread::GetOSPriority(ThreadPriority p)
{
#ifdef OVR_OS_PS3
	switch(p)
	{
		case Thread::CriticalPriority:		return 0;
		case Thread::HighestPriority:		return 300;
		case Thread::AboveNormalPriority:	return 600;
		case Thread::NormalPriority:		return 1000;
		case Thread::BelowNormalPriority:	return 1500;
		case Thread::LowestPriority:		return 2500;
		case Thread::IdlePriority:			return 3071;
		default:							return 1000;
	}
#elif ANDROID
	// pthread_attr_init() sets SCHED_NORMAL
	const int minPriority = sched_get_priority_min( SCHED_NORMAL );
	const int maxPriority = sched_get_priority_max( SCHED_NORMAL );
	switch(p)
	{
		case Thread::CriticalPriority:		return minPriority + ( maxPriority - minPriority ) * 7 / 8;
		case Thread::HighestPriority:		return minPriority + ( maxPriority - minPriority ) * 6 / 8;
		case Thread::AboveNormalPriority:	return minPriority + ( maxPriority - minPriority ) * 5 / 8;
		case Thread::NormalPriority:		return minPriority + ( maxPriority - minPriority ) * 4 / 8;
		case Thread::BelowNormalPriority:	return minPriority + ( maxPriority - minPriority ) * 3 / 8;
		case Thread::LowestPriority:		return minPriority + ( maxPriority - minPriority ) * 2 / 8;
		case Thread::IdlePriority:			return minPriority + ( maxPriority - minPriority ) * 1 / 8;
		default:							return minPriority + ( maxPriority - minPriority ) * 4 / 8;
	}
#else
    OVR_UNUSED(p);
    return -1;
#endif
}

bool Thread::start(ThreadState initialState)
{
    if (initialState == NotRunning)
    {
        return 0;
    }
    if (threadState() != NotRunning)
    {
        OVR_DEBUG_LOG(("Thread::Start failed - thread %p already running", this));
        return 0;
    }

    if (!InitDefaultAttr)
    {
        pthread_attr_init(&DefaultAttr);
        pthread_attr_setdetachstate(&DefaultAttr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&DefaultAttr, 128 * 1024);
        sched_param sparam;
        sparam.sched_priority = Thread::GetOSPriority(NormalPriority);
        pthread_attr_setschedparam(&DefaultAttr, &sparam);
        InitDefaultAttr = 1;
    }

    m_exitCode        = 0;
    m_suspendCount    = 0;
    m_threadFlags     = (initialState == Running) ? 0 : OVR_THREAD_START_SUSPENDED;

    // AddRef to us until the thread is finished
    AddRef();
    ThreadList::AddRunningThread(this);

    int result;
    if (m_stackSize != 128 * 1024 || m_priority != NormalPriority)
    {
        pthread_attr_t attr;

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_attr_setstacksize(&attr, m_stackSize);
        sched_param sparam;
        sparam.sched_priority = Thread::GetOSPriority(m_priority);
        pthread_attr_setschedparam(&attr, &sparam);
        result = pthread_create(&m_threadHandle, &attr, Thread_PthreadStartFn, this);
        pthread_attr_destroy(&attr);
    }
    else
    {
        result = pthread_create(&m_threadHandle, &DefaultAttr, Thread_PthreadStartFn, this);
    }

    if (result)
    {
        m_threadFlags = 0;
        Release();
        ThreadList::RemoveRunningThread(this);
        return 0;
    }
    return 1;
}


// Suspend the thread until resumed
bool    Thread::suspend()
{
    OVR_DEBUG_LOG(("Thread::Suspend - cannot suspend threads on this system"));
    return 0;
}

// Resumes currently suspended thread
bool    Thread::resume()
{
    return 0;
}


// Quits with an exit code
void    Thread::exit(int exitCode)
{
    // Can only exist the current thread
   // if (GetThread() != this)
   //     return;

    // Call the virtual OnExit function
    onExit();

    // Signal this thread object as done and release it's references.
    finishAndRelease();
    ThreadList::RemoveRunningThread(this);

    pthread_exit((void *) exitCode);
}

ThreadId GetCurrentThreadId()
{
    return (void*)pthread_self();
}

/* static */
int     Thread::cpuCount()
{
    return 1;
}

// *** Sleep functions

/* static */
bool    Thread::Sleep(unsigned secs)
{
    sleep(secs);
    return 1;
}
/* static */
bool    Thread::MSleep(unsigned msecs)
{
    usleep(msecs*1000);
    return 1;
}

void    Thread::setThreadName( const char* name )
{
#ifdef ANDROID
    int result = pthread_setname_np( m_threadHandle, name );
    if ( result != 0 )
    {
        __android_log_print( ANDROID_LOG_WARN, "OVR_Threads", "SetThreadName %s failed %s", name, strerror( result ) );
    }
#endif
}

#ifdef OVR_OS_PS3

sys_lwmutex_attribute_t Lock::LockAttr = { SYS_SYNC_PRIORITY, SYS_SYNC_RECURSIVE };

#endif

}

#endif  // OVR_ENABLE_THREADS
