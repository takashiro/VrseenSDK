#include "VEventLoop.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "VLog.h"

NV_NAMESPACE_BEGIN

struct VEventLoop::Private
{
    Private(int capacity):
            shutdown( false ),
            capacity( capacity ),
            messages( new Node[ capacity ] ),
            head( 0 ),
            tail( 0 ),
            synced( false )
    {
        assert( capacity > 0 );

        for ( int i = 0; i < capacity; i++ )
        {
            messages[i].sychronized = false;
        }

        pthread_mutexattr_t	attr;
        pthread_mutexattr_init( &attr );
        pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_ERRORCHECK );
        pthread_mutex_init( &mutex, &attr );
        pthread_mutexattr_destroy( &attr );
        pthread_cond_init( &posted, NULL );
        pthread_cond_init( &received, NULL );
    }

    bool shutdown;
    const int capacity;

    struct Node
    {
        VEvent event;
        bool sychronized;
    };

    Node *messages;

    volatile int head;
    volatile int tail;
    bool synced;
    pthread_mutex_t mutex;
    pthread_cond_t posted;
    pthread_cond_t received;

    bool post(const VEvent &event, bool synchronized)
    {
        if (shutdown) {
            return false;
        }

        pthread_mutex_lock(&mutex);
        if (tail - head >= capacity) {
            pthread_mutex_unlock(&mutex);
            return false;
        }
        const int index = tail % capacity;
        messages[index].event = event;
        messages[index].sychronized = synchronized;
        tail++;
        pthread_cond_signal(&posted);
        if (synchronized) {
            pthread_cond_wait(&received, &mutex);
        }
        pthread_mutex_unlock(&mutex);

        return true;
    }
};

VEventLoop::VEventLoop(int capacity)
    : d(new Private(capacity))
{
}

VEventLoop::~VEventLoop()
{
    // Free the queue itself.
    delete[] d->messages;

    pthread_mutex_destroy( &d->mutex );
    pthread_cond_destroy( &d->posted );
    pthread_cond_destroy( &d->received );

    delete d;
}

void VEventLoop::quit()
{
    vInfo("VMessageQueue shutdown");
    d->shutdown = true;
}

void VEventLoop::post(const VEvent &event)
{
    d->post(event, false);
}

void VEventLoop::post(const VString &command, const VVariant &data)
{
    VEvent event(command);
    event.data = data;
    d->post(event, false);
}

void VEventLoop::post(const char *command)
{
    VEvent event(command);
    d->post(event, false);
}

void VEventLoop::post(const VVariant &data)
{
    VEvent event;
    event.data = data;
    d->post(event, false);
}

void VEventLoop::post(const VVariant::Function &func)
{
    VEvent event;
    event.data = func;
    d->post(event, false);
}

void VEventLoop::send(const VEvent &event)
{
    d->post(event, true);
}

void VEventLoop::send(const VString &command, const VVariant &data)
{
    VEvent event(command);
    event.data = data;
    d->post(event, true);
}

void VEventLoop::send(const char *command)
{
    VEvent event(command);
    d->post(event, true);
}

void VEventLoop::send(const VVariant &data)
{
    VEvent event;
    event.data = data;
    d->post(event, true);
}

void VEventLoop::send(const VVariant::Function &func)
{
    VEvent event;
    event.data = func;
    d->post(event, true);
}

VEvent VEventLoop::next()
{
    if (d->synced) {
        pthread_cond_signal(&d->received);
        d->synced = false;
    }

    pthread_mutex_lock(&d->mutex);
    if (d->tail <= d->head) {
        pthread_mutex_unlock(&d->mutex);
        return VEvent();
    }

    const int index = d->head % d->capacity;
    VEvent event = d->messages[index].event;
    d->synced = d->messages[index].sychronized;
    d->messages[index].sychronized = false;
    d->head++;
    pthread_mutex_unlock(&d->mutex);

    return event;
}

// Returns immediately if there is already a message in the queue.
void VEventLoop::wait()
{
    if ( d->synced )
    {
        pthread_cond_signal( &d->received );
        d->synced = false;
    }

    pthread_mutex_lock( &d->mutex );
    if ( d->tail > d->head )
    {
        pthread_mutex_unlock( &d->mutex );
        return;
    }

    pthread_cond_wait( &d->posted, &d->mutex );
    pthread_mutex_unlock( &d->mutex );
}

void VEventLoop::clear()
{
    d->head = d->tail;
}

NV_NAMESPACE_END
