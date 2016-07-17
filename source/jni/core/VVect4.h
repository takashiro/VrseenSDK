#pragma once

#include "vglobal.h"

#include <math.h>

NV_NAMESPACE_BEGIN

template<class T>
class V4Vect
{
public:
    T x;
    T y;
    T z;
    T w;

    V4Vect() : x(0), y(0), z(0), w(0) {}
    V4Vect(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    V4Vect(T s) : x(s), y(s), z(s), w(s) { }
    V4Vect(const V4Vect &src) : x(src.x), y(src.y), z(src.z), w(src.w) {}

    bool operator == (const V4Vect &vect) const { return x == vect.x && y == vect.y && z == vect.z && w == vect.w; }
    bool operator != (const V4Vect &vect) const { return x != vect.x || y != vect.y || z != vect.z || w != vect.w; }

    V4Vect operator + (const V4Vect &vect) const { return V4Vect(x + vect.x, y + vect.y, z + vect.z, w + vect.w); }
    V4Vect &operator += (const V4Vect &vect) { x += vect.x; y += vect.y; z += vect.z; w += vect.w; return *this; }

    V4Vect operator - (const V4Vect &vect) const { return V4Vect(x - vect.x, y - vect.y, z - vect.z, w - vect.w); }
    V4Vect &operator -= (const V4Vect &vect) { x -= vect.x; y -= vect.y; z -= vect.z; w -= vect.w; return *this; }

    V4Vect operator - () const { return V4Vect(-x, -y, -z, -w); }

    // Scalar multiplication/division scales vector.
    V4Vect operator * (T factor) const { return V4Vect(x * factor, y * factor, z * factor, w * factor); }
    V4Vect &operator *= (T factor) { x *= factor; y *= factor; z *= factor; w *= factor;return *this; }

    V4Vect operator / (T factor) const { T rcp = T(1) / factor; return V4Vect(x * rcp, y * rcp, z * rcp, w * rcp); }
    V4Vect &operator /= (T factor) { T rcp = T(1) / factor; x *= rcp; y *= rcp; z *= rcp; w *= rcp; return *this; }

    // Multiply and divide operators do entry-wise VConstants
    V4Vect operator * (const V4Vect &vect) const { return V4Vect(x * vect.x, y * vect.y, z * vect.z, w * vect.w); }
    V4Vect operator / (const V4Vect &vect) const { return V4Vect(x / vect.x, y / vect.y, z / vect.z, w / vect.w); }

    T dotProduct(const V4Vect &vect) const { return x * vect.x + y * vect.y + z * vect.z + w * vect.w; }

    T lengthSquared() const { return (x * x + y * y + z * z + w * w); }
    T length() const { return sqrt(lengthSquared()); }

    // Determine if this a unit vector.
    bool isNormalized() const { return fabs(lengthSquared() - T(1)) < 1e-5; }

    // Normalize
    void normalize() { *this /= length(); }

    // Returns normalized (unit) version of the vector without modifying itself.
    V4Vect normalized() const { return *this / length(); }

    T *data() { return &x; }
    const T *data() const { return &x; }
};

typedef V4Vect<float>  V4Vectf;
typedef V4Vect<double> V4Vectd;
typedef V4Vect<int>    V4Vecti;

NV_NAMESPACE_END
