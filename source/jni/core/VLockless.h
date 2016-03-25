#pragma once

#include "vglobal.h"
#include <pthread.h>
#include "VAtomicInt.h"
#include "VCircularQueue.h"

// Define this to compile-in Lockless test logic
//#define OVR_LOCKLESS_TEST

NV_NAMESPACE_BEGIN


template <class E>
class VLockless
{
public:
    VLockless() : count(0), buffer(8){}

    E state() const
    {
        return buffer[count.exchangeAddSync(0)];
    }

    void setState(E state)
    {
        buffer.prepend(state);
        count = count.exchangeAddSync(1) & (int)(buffer.capacity() - 1);
    }

    mutable VAtomicInt count;
    VCircularQueue<E> buffer;

};


#ifdef OVR_LOCKLESS_TEST
void StartLocklessTest();
#endif


NV_NAMESPACE_END


