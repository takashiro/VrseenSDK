/*
 * VCircularQueueSync.h
 *
 *  Created on: 2016年5月11日
 *      Author: yangkai
 */

#pragma once

#include "VDeque.h"
#include "pthread.h"

NV_NAMESPACE_BEGIN

template <class E>
class VCircularQueueSync : public VDeque<E>
{
    typedef VDeque<E> ParentType;

public:
    VCircularQueueSync(uint capacity = 500) : m_capacity(capacity)
    {
        assert(0 == pthread_mutex_init(&m_mutex, nullptr));
    }

    ~VCircularQueueSync()
    {
        assert(0 == pthread_mutex_destroy(&m_mutex));
    }
    bool isFull() const
    {
        assert(0 == pthread_mutex_lock(&m_mutex));
        bool val = ParentType::size() >= m_capacity;
        assert(0 == pthread_mutex_unlock(&m_mutex));
        return  val;
    }

    uint capacity() const { return m_capacity; }

    void prepend(const E &element)
    {
        assert(0 == pthread_mutex_lock(&m_mutex));
        while (ParentType::size() >= m_capacity) {
            ParentType::takeLast();
        }
        ParentType::prepend(element);
        assert(0 == pthread_mutex_unlock(&m_mutex));
    }

    void append(const E &element)
    {
        assert(0 == pthread_mutex_lock(&m_mutex));
        while (ParentType::size() >= m_capacity) {
            ParentType::removeFirst();
        }
        ParentType::append(element);
        assert(0 == pthread_mutex_unlock(&m_mutex));
    }

private:
    uint m_capacity;
    pthread_mutex_t m_mutex;
};

NV_NAMESPACE_END
