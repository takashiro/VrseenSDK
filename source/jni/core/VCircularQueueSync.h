/*
 * VCircularQueueSync.h
 *
 *  Created on: 2016年5月11日
 *      Author: yangkai
 */

#pragma once

#include "pthread.h"
#include "vglobal.h"
#include "assert.h"
NV_NAMESPACE_BEGIN

template <class E>
class VCircularQueueSync
{
public:
    VCircularQueueSync(uint capacity = 500) :
        m_capacity(capacity),
        count(0),
        front(0),
        tail(0),
        data(new E[m_capacity])
    {
        assert(0 == pthread_mutex_init(&m_mutex, nullptr));
    }

    ~VCircularQueueSync()
    {
        assert(0 == pthread_mutex_destroy(&m_mutex));
        delete [] data;
    }

    bool isFull() const
    {
        assert(0 == pthread_mutex_lock(&m_mutex));
        bool val = count >= m_capacity;
        assert(0 == pthread_mutex_unlock(&m_mutex));
        return  val;
    }

    uint capacity() const { return m_capacity; }

    void prepend(const E &element)
    {
        assert(0 == pthread_mutex_lock(&m_mutex));
        front = (m_capacity + front - 1) % m_capacity;
        data[front] = element;
        if (count >= m_capacity) {
            tail = front;
        }
        else {
            count++;
        }
        assert(0 == pthread_mutex_unlock(&m_mutex));
    }

    void append(const E &element)
    {
        assert(0 == pthread_mutex_lock(&m_mutex));
        data[tail] = element;
        tail = (m_capacity + tail + 1) % m_capacity;
        if (count >= m_capacity) {
            front = tail;
        }
        else {
            count++;
        }
        assert(0 == pthread_mutex_unlock(&m_mutex));
    }

    int size() const
    {
        return count;
    }

    E & get(int index) const
    {
        return data[(front + index) % m_capacity];
    }

private:
    uint m_capacity;
    int count;
    int front;
    int tail;
    E * data;
    pthread_mutex_t m_mutex;
};

NV_NAMESPACE_END
