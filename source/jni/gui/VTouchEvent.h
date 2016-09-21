#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

struct VTouchEvent
{
    enum Action{
        Down,
        Up,
        Move,
        Cancel,
        Outside,
        PointerDown,
        PointerUp,
        HoverMove,
        Scroll,
        HoverEnter,
        HoverExit
    };

    Action action;
    float x;
    float y;
};

NV_NAMESPACE_END
