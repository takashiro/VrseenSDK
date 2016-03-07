#include "System.h"
#include "Threads.h"
#include "Timer.h"

#include "VLog.h"

namespace NervGear {

// *****  NervGear::System Implementation

// Initializes System core, installing allocator.
void System::Init(Log* log, Allocator *palloc)
{
    if (!Allocator::GetInstance()) {
        Log::SetGlobalLog(log);
        Timer::InitializeTimerSystem();
        Allocator::setInstance(palloc);
#ifdef OVR_ENABLE_THREADS
        Thread::InitThreadList();
#endif
    } else {
        vFatal("System::Init failed - duplicate call.");
    }
}

void System::Destroy()
{
    if (Allocator::GetInstance()) {
#ifdef OVR_ENABLE_THREADS
        // Wait for all threads to finish; this must be done so that memory
        // allocator and all destructors finalize correctly.
        Thread::FinishAllThreads();
#endif

        // Shutdown heap and destroy SysAlloc singleton, if any.
        Allocator::GetInstance()->onSystemShutdown();
        Allocator::setInstance(0);

        Timer::ShutdownTimerSystem();
        Log::SetGlobalLog(Log::GetDefaultLog());
    } else {
        vFatal("System::Destroy failed - System not initialized.");
    }
}

// Returns 'true' if system was properly initialized.
bool System::IsInitialized()
{
    return Allocator::GetInstance() != 0;
}

} // OVR

