#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

template<class T>
class VRect
{
public:
    T x;
    T y;

    T width;
    T height;

    VRect() {}

    VRect(T x, T y, T width, T height)
        : x(x)
        , y(y)
        , width(width)
        , height(height)
    {}

    bool operator == (const VRect &rect) const { return x == rect.x && y == rect.y && width == rect.width && height == rect.height; }
    bool operator != (const VRect &rect) const { return !operator == (rect); }
};

NV_NAMESPACE_END
