/*
 * VLock.h
 *
 *  Created on: 2016年3月21日
 *      Author: yangkai
 */
#pragma once

#include "vglobal.h"
#include "Types.h"
#include <pthread.h>

NV_NAMESPACE_BEGIN
class VLock
{
    void    operator delete(void*) {}

    pthread_mutex_t m_mutex;

public:
    static pthread_mutexattr_t RecursiveAttr;
    static bool                RecursiveAttrInit;

    VLock (unsigned dummy = 0)
    {
        OVR_UNUSED(dummy);
        if (!RecursiveAttrInit)
        {
            pthread_mutexattr_init(&RecursiveAttr);
            pthread_mutexattr_settype(&RecursiveAttr, PTHREAD_MUTEX_RECURSIVE);
            RecursiveAttrInit = 1;
        }
        pthread_mutex_init(&m_mutex,&RecursiveAttr);
    }
    ~VLock ()                { pthread_mutex_destroy(&m_mutex); }
    inline void DoLock()    { pthread_mutex_lock(&m_mutex); }
    inline void Unlock()    { pthread_mutex_unlock(&m_mutex); }

public:
    // Locker class, used for automatic locking
    class VLocker
    {
    public:
        VLock *pVLock;
        inline VLocker(VLock *plock)
        { pVLock = plock; pVLock->DoLock(); }
        inline ~VLocker()
        { pVLock->Unlock();  }
    };
};
NV_NAMESPACE_END
