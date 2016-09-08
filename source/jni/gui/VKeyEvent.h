#pragma once

#include "VMatrix4.h"

NV_NAMESPACE_BEGIN

struct VKeyEvent
{
    int key;
    int repeat;
    VMatrix4f viewMatrix;

    VKeyEvent()
        : key(0)
        , repeat(0)
    {
    }
};

NV_NAMESPACE_END
