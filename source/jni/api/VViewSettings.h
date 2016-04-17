#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

class VViewSettings
{
public:
    VViewSettings()
        : interpupillaryDistance(0.0640f)
        , eyeHeight(1.6750f)
        , headModelDepth(0.0805f)
        , headModelHeight(0.0750f)
    {
    }

    float interpupillaryDistance;
    float eyeHeight;
    float headModelDepth;
    float headModelHeight;
};

NV_NAMESPACE_END
