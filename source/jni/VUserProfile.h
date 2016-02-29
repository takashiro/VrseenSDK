#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

class VUserProfile
{
public:
    VUserProfile() :
        ipd(0.0640f),
        eyeHeight(1.6750f),
        headModelDepth(0.0805f),
        headModelHeight(0.0750f)
    {
    }
     
    float ipd;
    float eyeHeight;
    float headModelDepth;
    float headModelHeight;

    void load();
    void save();
};

NV_NAMESPACE_END
