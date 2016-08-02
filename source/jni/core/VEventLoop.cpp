#include "VEventLoop.h"

#include <VLog.h>
#include <VMutex.h>
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
        vAssert(capacity > 0);

        for (int i = 0; i < capacity; i++) {
            messages[i].sychronized = false;
        }
    }

    ~Private()
    {
        delete[] messages;
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
    VMutex mutex;
    VSemaphore posted;
    VSemaphore received;

    bool post(const VEvent &event, bool synchronized)
    {
        if (shutdown) {
            return false;
        }

        mutex.lock();
        if (tail - head >= capacity) {
            mutex.unlock();
            return false;
        }
        const int index = tail % capacity;
        messages[index].event = event;
        messages[index].sychronized = synchronized;
        tail++;
        mutex.lock();

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

        mutex.lock();
        if (tail - head >= capacity) {
            mutex.unlock();
            return false;
        }
        const int index = tail % capacity;
        messages[index].event = std::move(event);
        messages[index].sychronized = synchronized;
        tail++;
        mutex.unlock();

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
    d->mutex.lock();
    if (d->tail <= d->head) {
        d->mutex.unlock();
        return VEvent();
    }

    const int index = d->head % d->capacity;
    VEvent event = std::move(d->messages[index].event);
    if (d->messages[index].sychronized) {
        d->received.post();
    }
    d->messages[index].sychronized = false;
    d->head++;
    d->mutex.unlock();

    return event;
}

// Returns immediately if there is already a message in the queue.
void VEventLoop::wait()
{
    d->mutex.lock();
    if (d->tail > d->head) {
        d->mutex.unlock();
        return;
    }
    d->mutex.unlock();

    d->posted.wait();
}

void VEventLoop::clear()
{
    d->head = d->tail;
}

NV_NAMESPACE_END
