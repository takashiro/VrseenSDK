#include "VRotationSensor.h"
#include "VLockless.h"

NV_NAMESPACE_BEGIN

struct VRotationSensor::Private
{
    VLockless<VRotationState> state;
};

void VRotationSensor::setState(const VRotationState &state)
{
    d->state.setState(state);
}

VRotationState VRotationSensor::state() const
{
    return d->state.state();
}

VRotationSensor *VRotationSensor::instance()
{
    static VRotationSensor sensor;
    return &sensor;
}

VRotationSensor::~VRotationSensor()
{
    delete d;
}

VRotationState VRotationSensor::predictState(double timestamp) const
{
    //@to-do: implement this
    VRotationState state = this->state();
    state.timestamp = timestamp;
    return state;
}

VRotationSensor::VRotationSensor()
    : d(new Private)
{
}

NV_NAMESPACE_END
