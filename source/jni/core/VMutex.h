#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

class VMutex
{
public:
    class Locker
    {
    public:
        Locker(VMutex *mutex)
            : m_mutex(mutex)
        {
            m_mutex = mutex;
            m_mutex->lock();
        }

        ~Locker() { m_mutex->unlock(); }

    private:
        VMutex *m_mutex;
    };

    VMutex(bool recursive = true);
    ~VMutex();

    void lock();
    bool tryLock();
    void unlock();

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VMutex)
};

NV_NAMESPACE_END
