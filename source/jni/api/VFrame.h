#pragma once

#include "VInput.h"
#include "VRotationSensor.h"

NV_NAMESPACE_BEGIN

struct VFrame
{
    VFrame()
        : id(0)
        , deltaSeconds( 0.0f )
    {}

    longlong id;

    VRotationSensor::State pose;
    VInput input;
    float deltaSeconds;
};

NV_NAMESPACE_END

