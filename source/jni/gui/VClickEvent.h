#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

struct VClickEvent
{
    int key;
    int repeat;

    VClickEvent()
        : key(0)
        , repeat(0)
    {
    }
};

NV_NAMESPACE_END
