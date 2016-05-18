#pragma once

#include "VAtomicInt.h"

NV_NAMESPACE_BEGIN

template <class E>
class VLockless
{
public:
    VLockless()
        : m_begin(0)
        , m_end(0)
    {
    }

    E state() const
    {
        E state;
        int begin, end, final;
        forever {
            end = m_end.exchangeAddSync(0);
            state = m_slot[end & 1];
            begin = m_begin.exchangeAddSync(0);

            if (end == begin) {
                return state;
            }

            state = m_slot[(begin & 1) ^ 1];

            final = m_begin.exchangeAddSync(0);
            if (final == begin) {
                return state;
            }
        }
        return state;
    }

    void setState(E state)
    {
        const int slot = m_begin.exchangeAddSync(1) & 1;
        m_slot[slot ^ 1] = state;
        m_end.exchangeAddSync(1);
    }

private:
    mutable VAtomicInt m_begin;
    mutable VAtomicInt m_end;
    E m_slot[2];
};

NV_NAMESPACE_END
