/*
 * VLock.h
 *
 *  Created on: 2016年3月21日
 *      Author: yangkai
 */
#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

class VLock
{
public:
    class Locker
    {
    public:
        inline Locker(VLock *plock) { m_lock = plock; m_lock->lock(); }
        inline ~Locker() { m_lock->unlock();  }

    private:
        VLock *m_lock;
    };

    VLock();
    ~VLock();

    void lock();
    void unlock();

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VLock)
};

NV_NAMESPACE_END
