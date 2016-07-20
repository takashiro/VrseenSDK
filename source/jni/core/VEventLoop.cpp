#include "VEventLoop.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include <VSemaphore.h>

NV_NAMESPACE_BEGIN

struct VEventLoop::Private
{
    Private(int capacity)
        : shutdown(false)
        , capacity(capacity)
        , messages(new Node[capacity])
        , head(0)
        , tail(0)
    {
        assert(capacity > 0);

        for (int i = 0; i < capacity; i++) {
            messages[i].sychronized = false;
        }

        pthread_mutexattr_t	attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
        pthread_mutex_init(&mutex, &attr);
        pthread_mutexattr_destroy(&attr);
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
    pthread_mutex_t mutex;
    VSemaphore posted;
    VSemaphore received;

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
        pthread_mutex_unlock(&mutex);

        posted.post();
        if (synchronized) {
            received.wait();
        }

        return true;
    }

    bool post(VEvent &&event, bool synchronized)
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
        messages[index].event = std::move(event);
        messages[index].sychronized = synchronized;
        tail++;
        pthread_mutex_unlock(&mutex);

        posted.post();
        if (synchronized) {
            received.wait();
        }

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

    pthread_mutex_destroy(&d->mutex);

    delete d;
}

void VEventLoop::quit()
{
    d->shutdown = true;
}

void VEventLoop::post(const VEvent &event)
{
    d->post(event, false);
}

void VEventLoop::post(VEvent &&event)
{
    d->post(std::move(event), false);
}

void VEventLoop::post(const VString &command, const VVariant &data)
{
    VEvent event(command);
    event.data = data;
    d->post(std::move(event), false);
}

void VEventLoop::post(const VString &command, VVariant &&data)
{
    VEvent event(command);
    event.data = std::move(data);
    d->post(std::move(event), false);
}

void VEventLoop::post(const char *command)
{
    VEvent event(command);
    d->post(std::move(event), false);
}

void VEventLoop::post(const VVariant::Function &func)
{
    VEvent event;
    event.data = func;
    d->post(std::move(event), false);
}

void VEventLoop::send(const VEvent &event)
{
    d->post(event, true);
}

void VEventLoop::send(VEvent &&event)
{
    d->post(std::move(event), true);
}

void VEventLoop::send(const VString &command, const VVariant &data)
{
    VEvent event(command);
    event.data = data;
    d->post(std::move(event), true);
}

void VEventLoop::send(const VString &command, VVariant &&data)
{
    VEvent event(command);
    event.data = std::move(data);
    d->post(std::move(event), true);
}

void VEventLoop::send(const char *command)
{
    VEvent event(command);
    d->post(std::move(event), true);
}

void VEventLoop::send(const VVariant::Function &func)
{
    VEvent event;
    event.data = func;
    d->post(std::move(event), true);
}

VEvent VEventLoop::next()
{
    pthread_mutex_lock(&d->mutex);
    if (d->tail <= d->head) {
        pthread_mutex_unlock(&d->mutex);
        return VEvent();
    }

    const int index = d->head % d->capacity;
    VEvent event = std::move(d->messages[index].event);
    if (d->messages[index].sychronized) {
        d->received.post();
    }
    d->messages[index].sychronized = false;
    d->head++;
    pthread_mutex_unlock(&d->mutex);

    return event;
}

// Returns immediately if there is already a message in the queue.
void VEventLoop::wait()
{
    pthread_mutex_lock(&d->mutex);
    if (d->tail > d->head) {
        pthread_mutex_unlock(&d->mutex);
        return;
    }
    pthread_mutex_unlock(&d->mutex);

    d->posted.wait();
}

void VEventLoop::clear()
{
    d->head = d->tail;
}

NV_NAMESPACE_END
