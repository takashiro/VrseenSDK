#pragma once

#include "vglobal.h"
#include "VQuat.h"

NV_NAMESPACE_BEGIN

class VRotationSensor
{
public:
    struct State : public VQuatf
    {
        float gyroX;
        float gyroY;
        float gyroZ;
        long timestamp;
    };

    static VRotationSensor *instance();
    ~VRotationSensor();

    void setState(const State &state);
    State state() const;

    State predictState(double timestamp) const;

private:
    VRotationSensor();

    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VRotationSensor)
};

NV_NAMESPACE_END
