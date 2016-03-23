/*
 * VLock.cpp
 *
 *  Created on: 2016年3月21日
 *      Author: yangkai
 */

#include "VLock.h"

#include <pthread.h>

NV_NAMESPACE_BEGIN

static pthread_mutexattr_t RecursiveAttr = 0;
static bool RecursiveAttrInit;

struct VLock::Private
{
    pthread_mutex_t mutex;
};

VLock::VLock()
    : d(new Private)
{
    if (!RecursiveAttrInit)
    {
        pthread_mutexattr_init(&RecursiveAttr);
        pthread_mutexattr_settype(&RecursiveAttr, PTHREAD_MUTEX_RECURSIVE);
        RecursiveAttrInit = 1;
    }
    pthread_mutex_init(&d->mutex,&RecursiveAttr);
}

VLock::~VLock()
{
    pthread_mutex_destroy(&d->mutex);
    delete d;
}

void VLock::lock()
{
    pthread_mutex_lock(&d->mutex);
}

void VLock::unlock()
{
    pthread_mutex_unlock(&d->mutex);
}

NV_NAMESPACE_END
