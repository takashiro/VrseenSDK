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

    VColor(uint argb)
        : red((uchar) (argb >> 16))
        , green((uchar) (argb >> 8))
        , blue((uchar) argb)
        , alpha((uchar) (argb >> 24))
    {
    }

    bool operator == (const VColor &color) const { return red == color.red && green == color.green && blue == color.blue && alpha == color.alpha; }

    enum Format
    {
        COLOR_565,
        COLOR_5551,     // single bit alpha useful for overlay planes
        COLOR_4444,
        COLOR_8888,
        COLOR_8888_sRGB
    };

};

NV_NAMESPACE_END
