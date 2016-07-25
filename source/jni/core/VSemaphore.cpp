#include "VSemaphore.h"

#include <semaphore.h>

NV_NAMESPACE_BEGIN

struct VSemaphore::Private
{
    sem_t sem;
};

VSemaphore::VSemaphore(uint value)
    : d(new Private)
{
    sem_init(&d->sem, 0, value);
}

VSemaphore::~VSemaphore()
{
    sem_destroy(&d->sem);
    delete d;
}

bool VSemaphore::wait()
{
    return sem_wait(&d->sem) == 0;
}

bool VSemaphore::post()
{
    return sem_post(&d->sem) == 0;
}

int VSemaphore::available() const
{
    int value;
    sem_getvalue(&d->sem, &value);
    return value;
}

NV_NAMESPACE_END
