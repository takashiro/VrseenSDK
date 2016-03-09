#include "System.h"
#include "VThread.h"
#include "VTimer.h"

#include "VLog.h"

namespace NervGear {

// *****  NervGear::System Implementation

// Initializes System core, installing allocator.
void System::Init(Log* log, Allocator *palloc)
{
    if (!Allocator::GetInstance()) {
        Log::SetGlobalLog(log);
        Allocator::setInstance(palloc);
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
        VThread::FinishAllThreads();
#endif

        // Shutdown heap and destroy SysAlloc singleton, if any.
        Allocator::GetInstance()->onSystemShutdown();
        Allocator::setInstance(0);

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

