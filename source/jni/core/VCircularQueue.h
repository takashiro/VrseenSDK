#pragma once

#include "VDeque.h"

NV_NAMESPACE_BEGIN

template <class E>
class VCircularQueue : public VDeque<E>
{
    typedef VDeque<E> ParentType;

public:
    VCircularQueue(uint capacity = 500) : m_capacity(capacity) { }

    bool isFull() const { return ParentType::size() >= m_capacity; }

    uint capacity() const { return m_capacity; }

    void prepend(const E &element)
    {
        if (isFull()) {
            ParentType::takeLast();
        }
        ParentType::prepend(element);
    }

    void append(const E &element)
    {
        if (isFull()) {
            ParentType::removeFirst();
        }
        ParentType::append(element);
    }

private:
    uint m_capacity;
};

NV_NAMESPACE_END
