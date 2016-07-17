#pragma once

#include "VQuat.h"

NV_NAMESPACE_BEGIN

struct VRotationState : public VQuatf
{
    VVect3f gyro;
    double timestamp;

    VRotationState &operator = (const VQuatf &orientation);
};

NV_NAMESPACE_END
