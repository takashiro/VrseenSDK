#include "VRotationSensor.h"

NV_NAMESPACE_BEGIN

VRotationState &VRotationState::operator =(const VQuatf &orientation)
{
    w = orientation.w;
    x = orientation.x;
    y = orientation.y;
    z = orientation.z;
    return *this;
}

NV_NAMESPACE_END
