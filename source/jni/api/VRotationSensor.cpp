#include "VRotationSensor.h"
#include "VLockless.h"

NV_NAMESPACE_BEGIN

struct VRotationSensor::Private
{
    VLockless<VRotationSensor::State> state;
};

void VRotationSensor::setState(const VRotationSensor::State &state)
{
    d->state.setState(state);
}

VRotationSensor::State VRotationSensor::state() const
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

VRotationSensor::State VRotationSensor::predictState(double time) const
{
    //@to-do: implement this
    NV_UNUSED(time);
    return d->state.state();
}

VRotationSensor::VRotationSensor()
    : d(new Private)
{
}

NV_NAMESPACE_END
