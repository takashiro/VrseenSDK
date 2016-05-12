#pragma once

#include "vglobal.h"
#include "VAtomicInt.h"
#include "VCircularQueue.h"

NV_NAMESPACE_BEGIN

template <class E>
class VLockless
{
//public:
//    VLockless() : m_count(0), m_buffer(8){}

//    E state() const
//    {
//        return m_buffer[m_count.exchangeAddSync(0)];
//    }

//    void setState(E state)
//    {
//        m_buffer.append(state);
//        m_count = m_count.exchangeAddSync(1) & (int)(m_buffer.capacity() - 1);
//    }

//private:
//    mutable VAtomicInt m_count;
//    VCircularQueue<E> m_buffer;
public:
    VLockless() : m_updatebegin(0), m_updateend(0){}

    E state() const
    {
        E state;
        int begin, end, final;

        for(;;)
        {
            end = m_updateend.exchangeAddSync(0);
            state = m_slot[end & 1];
            begin = m_updatebegin.exchangeAddSync(0);

            if (end == begin)
            {
                return state;
            }

            state = m_slot[(begin & 1) ^ 1];

            final = m_updatebegin.exchangeAddSync(0);
            if (final == begin)
            {
                return state;
            }
        }
        return state;
    }

    void setState(E state)
    {
        const int slot = m_updatebegin.exchangeAddSync(1) & 1;
        m_slot[slot ^ 1] = state;
        m_updateend.exchangeAddSync(1);
    }


    private:
        mutable VAtomicInt m_updatebegin;
        mutable VAtomicInt   m_updateend;
        E                      m_slot[2];

};

NV_NAMESPACE_END
