#pragma once

#include "vglobal.h"

#include <math.h>

NV_NAMESPACE_BEGIN

template<class T>
class VVect4
{
public:
    T x;
    T y;
    T z;
    T w;

    VVect4() : x(0), y(0), z(0), w(0) {}
    VVect4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    VVect4(T s) : x(s), y(s), z(s), w(s) { }
    VVect4(const VVect4 &src) : x(src.x), y(src.y), z(src.z), w(src.w) {}

    bool operator == (const VVect4 &vect) const { return x == vect.x && y == vect.y && z == vect.z && w == vect.w; }
    bool operator != (const VVect4 &vect) const { return x != vect.x || y != vect.y || z != vect.z || w != vect.w; }

    VVect4 operator + (const VVect4 &vect) const { return VVect4(x + vect.x, y + vect.y, z + vect.z, w + vect.w); }
    VVect4 &operator += (const VVect4 &vect) { x += vect.x; y += vect.y; z += vect.z; w += vect.w; return *this; }

    VVect4 operator - (const VVect4 &vect) const { return VVect4(x - vect.x, y - vect.y, z - vect.z, w - vect.w); }
    VVect4 &operator -= (const VVect4 &vect) { x -= vect.x; y -= vect.y; z -= vect.z; w -= vect.w; return *this; }

    VVect4 operator - () const { return VVect4(-x, -y, -z, -w); }

    // Scalar multiplication/division scales vector.
    VVect4 operator * (T factor) const { return VVect4(x * factor, y * factor, z * factor, w * factor); }
    VVect4 &operator *= (T factor) { x *= factor; y *= factor; z *= factor; w *= factor;return *this; }

    VVect4 operator / (T factor) const { T rcp = T(1) / factor; return VVect4(x * rcp, y * rcp, z * rcp, w * rcp); }
    VVect4 &operator /= (T factor) { T rcp = T(1) / factor; x *= rcp; y *= rcp; z *= rcp; w *= rcp; return *this; }

    // Multiply and divide operators do entry-wise VConstants
    VVect4 operator * (const VVect4 &vect) const { return VVect4(x * vect.x, y * vect.y, z * vect.z, w * vect.w); }
    VVect4 operator / (const VVect4 &vect) const { return VVect4(x / vect.x, y / vect.y, z / vect.z, w / vect.w); }

    T dotProduct(const VVect4 &vect) const { return x * vect.x + y * vect.y + z * vect.z + w * vect.w; }

    T lengthSquared() const { return (x * x + y * y + z * z + w * w); }
    T length() const { return sqrt(lengthSquared()); }

    // Determine if this a unit vector.
    bool isNormalized() const { return fabs(lengthSquared() - T(1)) < 1e-5; }

    // Normalize
    void normalize() { *this /= length(); }

    // Returns normalized (unit) version of the vector without modifying itself.
    VVect4 normalized() const { return *this / length(); }

    T *data() { return &x; }
    const T *data() const { return &x; }
};

typedef VVect4<float>  VVect4f;
typedef VVect4<double> VVect4d;
typedef VVect4<int>    VVect4i;

NV_NAMESPACE_END
