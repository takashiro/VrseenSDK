#include "VMutex.h"
#include "VLog.h"

#include <pthread.h>

NV_NAMESPACE_BEGIN

struct VMutex::Private
{
    pthread_mutex_t mutex;
    bool recursive;
    uint lockCount;
    pthread_t locker;
};

VMutex::VMutex(bool recursive)
    : d(new Private)
{
    d->recursive = recursive;
    d->lockCount = 0;

    if (d->recursive) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&d->mutex, &attr);
        pthread_mutexattr_destroy(&attr);
    } else {
        pthread_mutex_init(&d->mutex, NULL);
    }
}

VMutex::~VMutex()
{
    pthread_mutex_destroy(&d->mutex);
    delete d;
}

void VMutex::lock()
{
    while (pthread_mutex_lock(&d->mutex));
    d->lockCount++;
    d->locker = pthread_self();
}

bool VMutex::tryLock()
{
    if (!pthread_mutex_trylock(&d->mutex)) {
        d->lockCount++;
        d->locker = pthread_self();
        return true;
    }
    return false;
}

void VMutex::unlock()
{
    vAssert(pthread_self() == d->locker && d->lockCount > 0);
    d->lockCount--;
    pthread_mutex_unlock(&d->mutex);
}

uint VMutex::lockCount() const
{
    return d->lockCount;
}

bool VMutex::isRecursive() const
{
    return d->recursive;
}

void VMutex::setLockCount(uint count)
{
    d->lockCount = count;
}

void VMutex::_unlock()
{
    pthread_mutex_unlock(&d->mutex);
}

NV_NAMESPACE_END
