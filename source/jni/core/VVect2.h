#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

template<class T>
class VVect2
{
public:
    T x;
    T y;

    VVect2() : x(0), y(0) {}
    VVect2(T x, T y) : x(x), y(y) {}
    VVect2(T s) : x(s), y(s) {}
    VVect2(const VVect2 &src) : x(src.x), y(src.y) { }

    bool operator == (const VVect2 &vect) const { return x == vect.x && y == vect.y; }
    bool operator != (const VVect2 &vect) const { return x != vect.x || y != vect.y; }

    VVect2 operator + (const VVect2 &vect) const { return VVect2(x + vect.x, y + vect.y); }
    VVect2 &operator += (const VVect2 &vect) { x += vect.x; y += vect.y; return *this; }

    VVect2 operator - (const VVect2 &vect) const { return VVect2(x - vect.x, y - vect.y); }
    VVect2 &operator -= (const VVect2 &vect) { x -= vect.x; y -= vect.y; return *this; }

    VVect2 operator - () const { return VVect2(-x, -y); }

    // Scalar multiplication/division scales vector.
    VVect2 operator * (T factor) const { return VVect2(x * factor, y * factor); }
    VVect2 &operator *= (T factor) { x *= factor; y *= factor; return *this; }

    VVect2 operator / (T divisor) const { return VVect2(x / divisor, y / divisor); }
    VVect2& operator /= (T factor) { x /= factor; y /= factor; return *this; }

    // Multiply and divide operators do entry-wise VConstants. Used Dot() for dot product.
    VVect2 operator * (const VVect2 &vect) const { return VVect2(x * vect.x,  y * vect.y); }
    VVect2 operator / (const VVect2 &vect) const { return VVect2(x / vect.x,  y / vect.y); }

    T dotProduct(const VVect2 &vect) const { return x * vect.x + y * vect.y; }

    T angleTo(const VVect2 &vect) const
    {
        T div = lengthSquared() * vect.lengthSquared();
        T result = acos((dotProduct(vect)) / sqrt(div));
        return result;
    }

    T lengthSquared() const { return x * x + y * y; }
    T length() const { return sqrt(lengthSquared()); }

    // Returns squared distance between two points represented by vectors.
    T distanceSquaredTo(const VVect2 &vect) const { return (*this - vect).lengthSquared(); }

    // Returns distance between two points represented by vectors.
    T distanceTo(const VVect2 &vect) const { return (*this - vect).length(); }

    // Linearly interpolates from this vector to another.
    // Factor should be between 0.0 and 1.0, with 0 giving full value to this.
    VVect2 lerp(const VVect2 &vect, T factor) const { return *this * (T(1) - factor) + vect * factor; }

    // Projects this vector onto the argument; in other words,
    // A.Project(B) returns projection of vector A onto B.
    VVect2 projectTo(const VVect2 &vect) const
    {
        T length = vect.lengthSquared();
        return vect * (dotProduct(vect) / length);
    }
};


typedef VVect2<float> VVect2f;
typedef VVect2<double> VVect2d;
typedef VVect2<int> VVect2i;

NV_NAMESPACE_END
