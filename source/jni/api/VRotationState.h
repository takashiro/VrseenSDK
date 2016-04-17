#pragma once

#include "VQuat.h"

NV_NAMESPACE_BEGIN

struct VRotationState : public VQuatf
{
    V3Vectf gyro;
    longlong timestamp;
};

NV_NAMESPACE_END
