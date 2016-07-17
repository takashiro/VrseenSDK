#pragma once

#include "vglobal.h"

#include <math.h>

NV_NAMESPACE_BEGIN

template<class T>
class VVect3
{
public:
    T x;
    T y;
    T z;

    VVect3() : x(0), y(0), z(0) {}
    VVect3(T x, T y, T z = 0) : x(x), y(y), z(z) {}
    VVect3(T value) : x(value), y(value), z(value) {}
    VVect3(const VVect3 &src) : x(src.x), y(src.y), z(src.z) {}

    bool operator == (const VVect3 &vect) const { return x == vect.x && y == vect.y && z == vect.z; }
    bool operator != (const VVect3 &vect) const { return x != vect.x || y != vect.y || z != vect.z; }

    VVect3 operator + (const VVect3 &vect) const { return VVect3(x + vect.x, y + vect.y, z + vect.z); }
    VVect3 &operator += (const VVect3 &vect) { x += vect.x; y += vect.y; z += vect.z; return *this; }

    VVect3 operator - (const VVect3 &vect) const { return VVect3(x - vect.x, y - vect.y, z - vect.z); }
    VVect3 &operator -= (const VVect3 &vect) { x -= vect.x; y -= vect.y; z -= vect.z; return *this; }

    VVect3 operator - () const { return VVect3(-x, -y, -z); }

    // Scalar multiplication/division scales vector.
    VVect3 operator * (T factor) const { return VVect3(x * factor, y * factor, z * factor); }
    VVect3 &operator *= (T factor) { x *= factor; y *= factor; z *= factor; return *this; }

    VVect3 operator / (T divisor) const { T rcp = T(1)/divisor; return VVect3(x * rcp, y * rcp, z * rcp); }
    VVect3 &operator /= (T divisor) { T rcp = T(1)/divisor; x *= rcp; y *= rcp; z *= rcp; return *this; }

    // Multiply and divide operators do entry-wise VConstants
    VVect3 operator * (const VVect3 &vect) const { return VVect3(x * vect.x, y * vect.y, z * vect.z); }
    VVect3 operator / (const VVect3 &vect) const { return VVect3(x / vect.x, y / vect.y, z / vect.z); }

    T dotProduct(const VVect3 &vect) const { return x * vect.x + y * vect.y + z * vect.z; }
    VVect3 crossProduct(const VVect3 &vect) const { return VVect3(y * vect.z - z * vect.y, z * vect.x - x * vect.z, x * vect.y - y * vect.x); }

    // Returns the angle from this vector to b, in radians.
    T angleTo(const VVect3& b) const
    {
        T divisor = lengthSquared() * b.lengthSquared();
        T result = acos(dotProduct(b) / sqrt(divisor));
        return result;
    }

    T lengthSquared() const { return x * x + y * y + z * z; }
    T length() const { return sqrt(lengthSquared()); }

    // Returns squared distance between two points represented by vectors.
    T distanceSquaredTo(const VVect3 &vect) const { return (*this - vect).lengthSquared(); }

    // Returns distance between two points represented by vectors.
    T distanceTo(const VVect3 &vect) const { return (*this - vect).length(); }

    // Determine if this a unit vector.
    bool isNormalized() const { return fabs(lengthSquared() - T(1)) < 1e-5; }

    // Normalize
    void normalize() { *this /= length(); }

    // Returns normalized (unit) version of the vector without modifying itself.
    VVect3 normalized() const { return *this / length(); }

    // Linearly interpolates from this vector to another.
    // Factor should be between 0.0 and 1.0, with 0 giving full value to this.
    VVect3 lerp(const VVect3 &vect, T factor) const { return *this * (T(1) - factor) + vect * factor; }

    // Projects this vector onto the argument; in other words,
    // A.Project(B) returns projection of vector A onto B.
    VVect3 projectTo(const VVect3& b) const
    {
        T l2 = b.lengthSquared();
        return b * (dotProduct(b) / l2);
    }

    // Projects this vector onto a plane defined by a normal vector
    VVect3 projectToPlane(const VVect3 &normal) const { return *this - projectTo(normal); }
};

typedef VVect3<float> VVect3f;
typedef VVect3<double> VVect3d;
typedef VVect3<int> VVect3i;

NV_NAMESPACE_END
