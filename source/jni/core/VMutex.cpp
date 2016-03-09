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

static pthread_mutexattr_t RecursiveAttr;
static bool RecursiveAttrInit = 0;

VMutex::VMutex(bool recursive)
    : d(new Private)
{
    d->recursive = recursive;
    d->lockCount = 0;

    if (d->recursive) {
        if (!RecursiveAttrInit) {
            pthread_mutexattr_init(&RecursiveAttr);
            pthread_mutexattr_settype(&RecursiveAttr, PTHREAD_MUTEX_RECURSIVE);
            RecursiveAttrInit = 1;
        }
        pthread_mutex_init(&d->mutex, &RecursiveAttr);
    } else {
        pthread_mutex_init(&d->mutex, 0);
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
