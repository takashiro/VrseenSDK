#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

template<class T>
class VSize
{
public:
    T width;
    T height;

    VSize()
        : width(0)
        , height(0)
    {}

    VSize(T width, T height)
        : width(width)
        , height(height)
    {}

    bool operator == (const VSize &size) const { return width == size.width && height == size.height; }
    bool operator != (const VSize &size) const { return width != size.width || height != size.height; }

    VSize operator + (const VSize &size) const { return VSize(width + size.width, height + size.height); }
    VSize &operator += (const VSize& b) { width += b.width; height += b.height; return *this; }

    VSize operator - (const VSize &size) const { return VSize(width - size.width, height - size.height); }
    VSize &operator -= (const VSize& b) { width -= b.width; height -= b.height; return *this; }

    VSize operator - () const { return VSize(-width, -height); }

    VSize operator * (const VSize &size) const { return VSize(width * size.width, height * size.height); }
    VSize &operator *= (const VSize &size) { width *= size.width; height *= size.height; return *this; }

    VSize operator / (const VSize &size) const { return VSize(width / size.width, height / size.height); }
    VSize &operator /= (const VSize &size) { width /= size.width; height /= size.height; return *this; }

    // Scalar multiplication/division scales both components.
    VSize operator * (T factor) const { return VSize(width * factor, height * factor); }
    VSize &operator *= (T factor) { width *= factor; height *= factor; return *this; }

    VSize operator / (T divisor) const { return VSize(width / divisor, height / divisor); }
    VSize &operator/= (T divisor) { width /= divisor; height /= divisor; return *this; }

    T area() const { return width * height; }
};

NV_NAMESPACE_END
