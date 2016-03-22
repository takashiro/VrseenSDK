#include "VSignal.h"
#include "VMutex.h"
#include "VWaitCondition.h"

NV_NAMESPACE_BEGIN

struct VSignal::Private
{
    // Event state, its mutex and the wait condition
    volatile bool state;
    volatile bool temporary;
    mutable VMutex stateMutex;
    VWaitCondition stateWaitCondition;

    void updateState(bool newState, bool newTemp, bool mustNotify)
    {
        VMutex::Locker lock(&stateMutex);
        state       = newState;
        temporary   = newTemp;
        if (mustNotify)
            stateWaitCondition.notifyAll();
    }
};

VSignal::VSignal(bool state)
    : d(new Private)
{
    d->state = state;
    d->temporary = false;
}

VSignal::~VSignal()
{
    delete d;
}

bool VSignal::wait(uint delay)
{
    VMutex::Locker lock(&d->stateMutex);

    // Do the correct amount of waiting
    if (delay == Infinite) {
        while(!d->state) {
            d->stateWaitCondition.wait(&d->stateMutex);
        }
    } else if (delay) {
        if (!d->state) {
            d->stateWaitCondition.wait(&d->stateMutex, delay);
        }
    }

    bool state = d->state;
    // Take care of temporary 'pulsing' of a state
    if (d->temporary) {
        d->temporary = false;
        d->state = false;
    }
    return state;
}

void VSignal::set()
{
    d->updateState(true, false, true);
}

void VSignal::reset()
{
    d->updateState(false, false, false);
}

void VSignal::pulse()
{
    d->updateState(true, true, true);
}

NV_NAMESPACE_END
