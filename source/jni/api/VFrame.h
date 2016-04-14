#pragma once

#include "VInput.h"
#include "VRotationState.h"

NV_NAMESPACE_BEGIN

struct VFrame
{
    VFrame()
        : id(0)
        , deltaSeconds( 0.0f )
    {}

    longlong id;

    VRotationState pose;
    VInput input;
    float deltaSeconds;
};

NV_NAMESPACE_END

