#pragma once

#include "vglobal.h"

#include <math.h>

NV_NAMESPACE_BEGIN

template<class T>
class V3Vect
{
public:
    T x;
    T y;
    T z;

    V3Vect() : x(0), y(0), z(0) {}
    V3Vect(T x, T y, T z = 0) : x(x), y(y), z(z) {}
    V3Vect(T value) : x(value), y(value), z(value) {}
    V3Vect(const V3Vect &src) : x(src.x), y(src.y), z(src.z) {}

    bool operator == (const V3Vect &vect) const { return x == vect.x && y == vect.y && z == vect.z; }
    bool operator != (const V3Vect &vect) const { return x != vect.x || y != vect.y || z != vect.z; }

    V3Vect operator + (const V3Vect &vect) const { return V3Vect(x + vect.x, y + vect.y, z + vect.z); }
    V3Vect &operator += (const V3Vect &vect) { x += vect.x; y += vect.y; z += vect.z; return *this; }

    V3Vect operator - (const V3Vect &vect) const { return V3Vect(x - vect.x, y - vect.y, z - vect.z); }
    V3Vect &operator -= (const V3Vect &vect) { x -= vect.x; y -= vect.y; z -= vect.z; return *this; }

    V3Vect operator - () const { return V3Vect(-x, -y, -z); }

    // Scalar multiplication/division scales vector.
    V3Vect operator * (T factor) const { return V3Vect(x * factor, y * factor, z * factor); }
    V3Vect &operator *= (T factor) { x *= factor; y *= factor; z *= factor; return *this; }

    V3Vect operator / (T divisor) const { T rcp = T(1)/divisor; return V3Vect(x * rcp, y * rcp, z * rcp); }
    V3Vect &operator /= (T divisor) { T rcp = T(1)/divisor; x *= rcp; y *= rcp; z *= rcp; return *this; }

    // Multiply and divide operators do entry-wise VConstants
    V3Vect operator * (const V3Vect &vect) const { return V3Vect(x * vect.x, y * vect.y, z * vect.z); }
    V3Vect operator / (const V3Vect &vect) const { return V3Vect(x / vect.x, y / vect.y, z / vect.z); }

    T dotProduct(const V3Vect &vect) const { return x * vect.x + y * vect.y + z * vect.z; }
    V3Vect crossProduct(const V3Vect &vect) const { return V3Vect(y * vect.z - z * vect.y, z * vect.x - x * vect.z, x * vect.y - y * vect.x); }

    // Returns the angle from this vector to b, in radians.
    T angleTo(const V3Vect& b) const
    {
        T divisor = lengthSquared() * b.lengthSquared();
        T result = acos(dotProduct(b) / sqrt(divisor));
        return result;
    }

    T lengthSquared() const { return x * x + y * y + z * z; }
    T length() const { return sqrt(lengthSquared()); }

    // Returns squared distance between two points represented by vectors.
    T distanceSquaredTo(const V3Vect &vect) const { return (*this - vect).lengthSquared(); }

    // Returns distance between two points represented by vectors.
    T distanceTo(const V3Vect &vect) const { return (*this - vect).length(); }

    // Determine if this a unit vector.
    bool isNormalized() const { return fabs(lengthSquared() - T(1)) < 1e-5; }

    // Normalize
    void normalize() { *this /= length(); }

    // Returns normalized (unit) version of the vector without modifying itself.
    V3Vect normalized() const { return *this / length(); }

    // Linearly interpolates from this vector to another.
    // Factor should be between 0.0 and 1.0, with 0 giving full value to this.
    V3Vect lerp(const V3Vect &vect, T factor) const { return *this * (T(1) - factor) + vect * factor; }

    // Projects this vector onto the argument; in other words,
    // A.Project(B) returns projection of vector A onto B.
    V3Vect projectTo(const V3Vect& b) const
    {
        T l2 = b.lengthSquared();
        return b * (dotProduct(b) / l2);
    }

    // Projects this vector onto a plane defined by a normal vector
    V3Vect projectToPlane(const V3Vect &normal) const { return *this - projectTo(normal); }
};

typedef V3Vect<float> V3Vectf;
typedef V3Vect<double> V3Vectd;
typedef V3Vect<int> V3Vecti;

NV_NAMESPACE_END
