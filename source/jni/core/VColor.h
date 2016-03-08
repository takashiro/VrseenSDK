#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

struct VColor
{
    uchar red;
    uchar green;
    uchar blue;
    uchar alpha;

    VColor()
        : red(0)
        , green(0)
        , blue(0)
        , alpha(0)
    {
    }

    VColor(uchar red, uchar green, uchar blue, uchar alpha = 0xFF)
        : red(red)
        , green(green)
        , blue(blue)
        , alpha(alpha)
    {
    }

    VColor(uint hex)
        : red((uchar) (hex >> 16))
        , green((uchar) (hex >> 8))
        , blue((uchar) hex)
        , alpha((uchar) (hex >> 24))
    {
    }

    bool operator == (const VColor &color) const { return red == color.red && green == color.green && blue == color.blue && alpha == color.alpha; }
};

NV_NAMESPACE_END
