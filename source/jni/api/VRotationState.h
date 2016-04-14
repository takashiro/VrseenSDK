#pragma once

#include "VQuat.h"

NV_NAMESPACE_BEGIN

struct VRotationState : public VQuatf
{
    float gyroX;
    float gyroY;
    float gyroZ;
    long timestamp;
};

NV_NAMESPACE_END
