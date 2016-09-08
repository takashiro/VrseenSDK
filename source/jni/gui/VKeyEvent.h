#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

struct VKeyEvent
{
    int key;
    int repeat;

    VKeyEvent()
        : key(0)
        , repeat(0)
    {
    }
};

NV_NAMESPACE_END
