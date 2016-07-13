#pragma once

#include "VRotationState.h"

NV_NAMESPACE_BEGIN

class VRotationSensor
{
public:
    static VRotationSensor *instance();
    ~VRotationSensor();

    void setState(const VRotationState &state);
    VRotationState state() const;

    void recenterYaw();
    void setYaw(float newYaw);

    VRotationState predictState(double timestamp) const;

private:
    VRotationSensor();

    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VRotationSensor)
};

NV_NAMESPACE_END
