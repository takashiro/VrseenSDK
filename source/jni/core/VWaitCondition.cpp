#include "VWaitCondition.h"
#include "VMutex.h"
#include "VLog.h"

#include <errno.h>
#include <pthread.h>

NV_NAMESPACE_BEGIN

struct VWaitCondition::Private
{
    pthread_mutex_t mutex;
    pthread_cond_t condition;
};

VWaitCondition::VWaitCondition()
    : d(new Private)
{
    pthread_mutex_init(&d->mutex, 0);
    pthread_cond_init(&d->condition, 0);
}

VWaitCondition::~VWaitCondition()
{
    pthread_mutex_destroy(&d->mutex);
    pthread_cond_destroy(&d->condition);
    delete d;
}

bool VWaitCondition::wait(VMutex *mutex, uint delay)
{
    bool result = true;
    uint lockCount = mutex->lockCount();

    // Mutex must have been locked
    if (lockCount == 0) {
        return 0;
    }

    pthread_mutex_lock(&d->mutex);

    // Finally, release a mutex or semaphore
    if (mutex->isRecursive()) {
        // Release the recursive mutex N times
        mutex->setLockCount(0);
        for(uint i = 0; i < lockCount; i++) {
            mutex->_unlock();
        }
    } else {
        mutex->setLockCount(0);
        mutex->_unlock();
    }

    // Note that there is a gap here between mutex.Unlock() and Wait().
    // The other mutex protects this gap.

    if (delay == Infinite) {
        pthread_cond_wait(&d->condition, &d->mutex);
    } else {
        timespec ts;

        struct timeval tv;
        gettimeofday(&tv, 0);

        ts.tv_sec = tv.tv_sec + (delay / 1000);
        ts.tv_nsec = (tv.tv_usec + (delay % 1000) * 1000) * 1000;

        if (ts.tv_nsec > 999999999) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000;
        }
        int r = pthread_cond_timedwait(&d->condition, &d->mutex, &ts);
        vAssert(r == 0 || r == ETIMEDOUT);
        if (r) {
            result = false;
        }
    }

    pthread_mutex_unlock(&d->mutex);

    // Reaquire the mutex
    for(uint i = 0; i < lockCount; i++) {
        mutex->lock();
    }

    // Return the result
    return result;
}

// Notify a condition, releasing the least object in a queue
void VWaitCondition::notify()
{
    pthread_mutex_lock(&d->mutex);
    pthread_cond_signal(&d->condition);
    pthread_mutex_unlock(&d->mutex);
}

// Notify a condition, releasing all objects waiting
void VWaitCondition::notifyAll()
{
    pthread_mutex_lock(&d->mutex);
    pthread_cond_broadcast(&d->condition);
    pthread_mutex_unlock(&d->mutex);
}

NV_NAMESPACE_END
