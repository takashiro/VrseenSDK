#pragma once

#include "vglobal.h"
#include "VAtomicInt.h"
#include "VCircularQueue.h"

NV_NAMESPACE_BEGIN

template <class E>
class VLockless
{
public:
    VLockless() : m_count(0), m_buffer(8){}

    E state() const
    {
        return m_buffer[m_count.exchangeAddSync(0)];
    }

    void setState(E state)
    {
        m_buffer.prepend(state);
        m_count = m_count.exchangeAddSync(1) & (int)(m_buffer.capacity() - 1);
    }

private:
    mutable VAtomicInt m_count;
    VCircularQueue<E> m_buffer;
};

NV_NAMESPACE_END
