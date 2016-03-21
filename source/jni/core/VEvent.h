#pragma once

#include "VString.h"
#include "VJson.h"

NV_NAMESPACE_BEGIN

struct VEvent
{
    VEvent(const VString &name)
        : name(name)
    {
    }

    VString name;
    VJson data;
};

NV_NAMESPACE_END
