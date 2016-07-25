#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

template<class T>
class VRect2
{
public:
    T x;
    T y;

    T width;
    T height;

    VRect2() {}

    VRect2(T x, T y, T width, T height)
        : x(x)
        , y(y)
        , width(width)
        , height(height)
    {}

    bool operator == (const VRect2 &rect) const { return x == rect.x && y == rect.y && width == rect.width && height == rect.height; }
    bool operator != (const VRect2 &rect) const { return !operator == (rect); }
};

typedef VRect2<float> VRect2f;
typedef VRect2<double> VRect2d;
typedef VRect2<int> VRect2i;

NV_NAMESPACE_END
