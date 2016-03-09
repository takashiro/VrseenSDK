/************************************************************************************

PublicHeader:   None
Filename    :   OVR_ThreadCommandQueue.cpp
Content     :   Command queue for operations executed on a thread
Created     :   October 29, 2012

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#include "ThreadCommandQueue.h"

namespace NervGear {


//------------------------------------------------------------------------
// ***** CircularBuffer

// CircularBuffer is a FIFO buffer implemented in a single block of memory,
// which allows writing and reading variable-size data chucks. Write fails
// if buffer is full.

class CircularBuffer
{
    enum {
        AlignSize = 16,
        AlignMask = AlignSize - 1
    };

    UByte*  pBuffer;
    uint   Size;
    uint   Tail;   // Byte offset of next item to be popped.
    uint   Head;   // Byte offset of where next push will take place.
    uint   End;    // When Head < Tail, this is used instead of Size.

    inline uint roundUpSize(uint size)
    { return (size + AlignMask) & ~(uint)AlignMask; }

public:

    CircularBuffer(uint size)
        : Size(size), Tail(0), Head(0), End(0)
    {
        pBuffer = (UByte*)OVR_ALLOC_ALIGNED(roundUpSize(size), AlignSize);
    }
    ~CircularBuffer()
    {
        // For ThreadCommands, we must consume everything before shutdown.
        OVR_ASSERT(IsEmpty());
        OVR_FREE_ALIGNED(pBuffer);
    }

    bool    IsEmpty() const { return (Head == Tail); }

    // Allocates a state block of specified size and advances pointers,
    // returning 0 if buffer is full.
    UByte*  Write(uint size);

    // Returns a pointer to next available data block; 0 if none available.
    UByte*  ReadBegin()
    { return (Head != Tail) ? (pBuffer + Tail) : 0; }
    // Consumes data of specified size; this must match size passed to Write.
    void    ReadEnd(uint size);
};


// Allocates a state block of specified size and advances pointers,
// returning 0 if buffer is full.
UByte* CircularBuffer::Write(uint size)
{
    UByte* p = 0;

    size = roundUpSize(size);
    // Since this is circular buffer, always allow at least one item.
    OVR_ASSERT(size < Size/2);

    if (Head >= Tail)
    {
        OVR_ASSERT(End == 0);

        if (size <= (Size - Head))
        {
            p    = pBuffer + Head;
            Head += size;
        }
        else if (size < Tail)
        {
            p    = pBuffer;
            End  = Head;
            Head = size;
            OVR_ASSERT(Head != Tail);
        }
    }
    else
    {
        OVR_ASSERT(End != 0);

        if ((Tail - Head) > size)
        {
            p    = pBuffer + Head;
            Head += size;
            OVR_ASSERT(Head != Tail);
        }
    }

    return p;
}

void CircularBuffer::ReadEnd(uint size)
{
    OVR_ASSERT(Head != Tail);
    size = roundUpSize(size);

    Tail += size;
    if (Tail == End)
    {
        Tail = End = 0;
    }
    else if (Tail == Head)
    {
        OVR_ASSERT(End == 0);
        Tail = Head = 0;
    }
}


//-------------------------------------------------------------------------------------
// ***** ThreadCommand

ThreadCommand::PopBuffer::~PopBuffer()
{
    if (Size)
        Destruct<ThreadCommand>(toCommand());
}

void ThreadCommand::PopBuffer::InitFromBuffer(void* data)
{
    ThreadCommand* cmd = (ThreadCommand*)data;
    OVR_ASSERT(cmd->Size <= MaxSize);

    if (Size)
        Destruct<ThreadCommand>(toCommand());
    Size = cmd->Size;
    memcpy(Buffer, (void*)cmd, Size);
}

void ThreadCommand::PopBuffer::Execute()
{
    ThreadCommand* command = toCommand();
    OVR_ASSERT(command);
    command->Execute();
    if (NeedsWait())
        GetEvent()->PulseEvent();
}

//-------------------------------------------------------------------------------------

class ThreadCommandQueueImpl : public NewOverrideBase
{
    typedef ThreadCommand::NotifyEvent NotifyEvent;
    friend class ThreadCommandQueue;

public:

    ThreadCommandQueueImpl(ThreadCommandQueue* queue) :
		pQueue(queue),
		ExitEnqueued(false),
		ExitProcessed(false),
		CommandBuffer(2048)
    {
    }
    ~ThreadCommandQueueImpl();


    bool PushCommand(const ThreadCommand& command);
    bool PopCommand(ThreadCommand::PopBuffer* popBuffer);


    // ExitCommand is used by notify us that Thread is shutting down.
    struct ExitCommand : public ThreadCommand
    {
        ThreadCommandQueueImpl* pImpl;

        ExitCommand(ThreadCommandQueueImpl* impl, bool wait)
            : ThreadCommand(sizeof(ExitCommand), wait, true), pImpl(impl) { }

        virtual void Execute() const
        {
            Lock::Locker lock(&pImpl->QueueLock);
            pImpl->ExitProcessed = true;
        }
        virtual ThreadCommand* CopyConstruct(void* p) const
        { return Construct<ExitCommand>(p, *this); }
    };


    NotifyEvent* AllocNotifyEvent_NTS()
    {
        NotifyEvent* p = AvailableEvents.first();

        if (!AvailableEvents.isNull(p))
            p->removeNode();
        else
            p = new NotifyEvent;
        return p;
    }

    void         FreeNotifyEvent_NTS(NotifyEvent* p)
    {
        AvailableEvents.append(p);
    }

    void        FreeNotifyEvents_NTS()
    {
        while(!AvailableEvents.isEmpty())
        {
            NotifyEvent* p = AvailableEvents.first();
            p->removeNode();
            delete p;
        }
    }

    ThreadCommandQueue* pQueue;
    Lock                QueueLock;
    volatile bool       ExitEnqueued;
    volatile bool       ExitProcessed;
    List<NotifyEvent>   AvailableEvents;
    List<NotifyEvent>   BlockedProducers;
    CircularBuffer      CommandBuffer;
};



ThreadCommandQueueImpl::~ThreadCommandQueueImpl()
{
    Lock::Locker lock(&QueueLock);
    OVR_ASSERT(BlockedProducers.isEmpty());
    FreeNotifyEvents_NTS();
}

bool ThreadCommandQueueImpl::PushCommand(const ThreadCommand& command)
{
    ThreadCommand::NotifyEvent* completeEvent = 0;
    ThreadCommand::NotifyEvent* queueAvailableEvent = 0;

    // Repeat  writing command into buffer until it is available.
    do {

        { // Lock Scope
            Lock::Locker lock(&QueueLock);

            if (queueAvailableEvent)
            {
                FreeNotifyEvent_NTS(queueAvailableEvent);
                queueAvailableEvent = 0;
            }

            // Don't allow any commands after PushExitCommand() is called.
            if (ExitEnqueued && !command.ExitFlag)
                return false;


            bool   bufferWasEmpty = CommandBuffer.IsEmpty();
            UByte* buffer = CommandBuffer.Write(command.GetSize());
            if  (buffer)
            {
                ThreadCommand* c = command.CopyConstruct(buffer);
                if (c->NeedsWait())
                    completeEvent = c->pEvent = AllocNotifyEvent_NTS();
                // Signal-waker consumer when we add data to buffer.
                if (bufferWasEmpty)
                    pQueue->onPushNonEmptyLocked();
                break;
            }

            queueAvailableEvent = AllocNotifyEvent_NTS();
            BlockedProducers.append(queueAvailableEvent);
        } // Lock Scope

        queueAvailableEvent->Wait();

    } while(1);

    // Command was enqueued, wait if necessary.
    if (completeEvent)
    {
        completeEvent->Wait();
        Lock::Locker lock(&QueueLock);
        FreeNotifyEvent_NTS(completeEvent);
    }

    return true;
}


// Pops the next command from the thread queue, if any is available.
bool ThreadCommandQueueImpl::PopCommand(ThreadCommand::PopBuffer* popBuffer)
{
    Lock::Locker lock(&QueueLock);

    UByte* buffer = CommandBuffer.ReadBegin();
    if (!buffer)
    {
        // Notify thread while in lock scope, enabling initialization of wait.
        pQueue->onPopEmptyLocked();
        return false;
    }

    popBuffer->InitFromBuffer(buffer);
    CommandBuffer.ReadEnd(popBuffer->GetSize());

    if (!BlockedProducers.isEmpty())
    {
        ThreadCommand::NotifyEvent* queueAvailableEvent = BlockedProducers.first();
        queueAvailableEvent->removeNode();
        queueAvailableEvent->PulseEvent();
        // Event is freed later by waiter.
    }
    return true;
}


//-------------------------------------------------------------------------------------

ThreadCommandQueue::ThreadCommandQueue()
{
    pImpl = new ThreadCommandQueueImpl(this);
}
ThreadCommandQueue::~ThreadCommandQueue()
{
    delete pImpl;
}

bool ThreadCommandQueue::PushCommand(const ThreadCommand& command)
{
    return pImpl->PushCommand(command);
}

bool ThreadCommandQueue::PopCommand(ThreadCommand::PopBuffer* popBuffer)
{
    return pImpl->PopCommand(popBuffer);
}

void ThreadCommandQueue::PushExitCommand(bool wait)
{
    // Exit is processed in two stages:
    //  - First, ExitEnqueued flag is set to block further commands from queuing up.
    //  - Second, the actual exit call is processed on the consumer thread, flushing
    //    any prior commands.
    //    IsExiting() only returns true after exit has flushed.
    {
        Lock::Locker lock(&pImpl->QueueLock);
        if (pImpl->ExitEnqueued)
            return;
        pImpl->ExitEnqueued = true;
    }

    PushCommand(ThreadCommandQueueImpl::ExitCommand(pImpl, wait));
}

bool ThreadCommandQueue::IsExiting() const
{
    return pImpl->ExitProcessed;
}


} // namespace NervGear