/*
 * VCircularQueueSync.h
 *
 *  Created on: 2016年5月11日
 *      Author: yangkai
 */

#pragma once

#include <VMutex.h>

NV_NAMESPACE_BEGIN

template <class E>
class VCircularQueueSync
{
public:
    VCircularQueueSync(uint capacity = 500)
        : m_capacity(capacity)
        , m_count(0)
        , m_front(0)
        , m_tail(0)
        , m_data(new E[m_capacity])
        , m_mutex(false)
    {
    }

    ~VCircularQueueSync()
    {
        delete[] m_data;
    }

    bool isEmpty() const { return m_front == m_tail; }

    bool isFull() const { return m_count >= m_capacity; }

    uint capacity() const { return m_capacity; }

    void prepend(const E &element)
    {
        m_mutex.lock();
        m_front = (m_capacity + m_front - 1) % m_capacity;
        m_data[m_front] = element;
        if (m_count >= m_capacity) {
            m_tail = m_front;
        } else {
            m_count++;
        }
        m_mutex.unlock();
    }

    void append(const E &element)
    {
        m_mutex.lock();
        m_data[m_tail] = element;
        m_tail = (m_capacity + m_tail + 1) % m_capacity;
        if (m_count >= m_capacity) {
            m_front = m_tail;
        }
        else {
            m_count++;
        }
        m_mutex.unlock();
    }

    int size() const { return m_count; }

    const E &at(uint index) const { return m_data[(m_front + index) % m_capacity]; }
    E &operator[](uint index) { return m_data[(m_front + index) % m_capacity]; }
    const E &operator[] (uint index) const { return m_data[(m_front + index) % m_capacity]; }

    void clear()
    {
        m_count = 0;
        m_front = m_tail = 0;
    }

private:
    uint m_capacity;
    int m_count;
    int m_front;
    int m_tail;
    E *m_data;
    VMutex m_mutex;
};

NV_NAMESPACE_END
