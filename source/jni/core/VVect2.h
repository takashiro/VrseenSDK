#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

template<class T>
class V2Vect
{
public:
    T x;
    T y;

    V2Vect() : x(0), y(0) {}
    V2Vect(T x, T y) : x(x), y(y) {}
    V2Vect(T s) : x(s), y(s) {}
    V2Vect(const V2Vect &src) : x(src.x), y(src.y) { }

    bool operator == (const V2Vect &vect) const { return x == vect.x && y == vect.y; }
    bool operator != (const V2Vect &vect) const { return x != vect.x || y != vect.y; }

    V2Vect operator + (const V2Vect &vect) const { return V2Vect(x + vect.x, y + vect.y); }
    V2Vect &operator += (const V2Vect &vect) { x += vect.x; y += vect.y; return *this; }

    V2Vect operator - (const V2Vect &vect) const { return V2Vect(x - vect.x, y - vect.y); }
    V2Vect &operator -= (const V2Vect &vect) { x -= vect.x; y -= vect.y; return *this; }

    V2Vect operator - () const { return V2Vect(-x, -y); }

    // Scalar multiplication/division scales vector.
    V2Vect operator * (T factor) const { return V2Vect(x * factor, y * factor); }
    V2Vect &operator *= (T factor) { x *= factor; y *= factor; return *this; }

    V2Vect operator / (T divisor) const { return V2Vect(x / divisor, y / divisor); }
    V2Vect& operator /= (T factor) { x /= factor; y /= factor; return *this; }

    // Multiply and divide operators do entry-wise VConstants. Used Dot() for dot product.
    V2Vect operator * (const V2Vect &vect) const { return V2Vect(x * vect.x,  y * vect.y); }
    V2Vect operator / (const V2Vect &vect) const { return V2Vect(x / vect.x,  y / vect.y); }

    T dotProduct(const V2Vect &vect) const { return x * vect.x + y * vect.y; }

    T angleTo(const V2Vect &vect) const
    {
        T div = lengthSquared() * vect.lengthSquared();
        T result = acos((dotProduct(vect)) / sqrt(div));
        return result;
    }

    T lengthSquared() const { return x * x + y * y; }
    T length() const { return sqrt(lengthSquared()); }

    // Returns squared distance between two points represented by vectors.
    T distanceSquaredTo(const V2Vect &vect) const { return (*this - vect).lengthSquared(); }

    // Returns distance between two points represented by vectors.
    T distanceTo(const V2Vect &vect) const { return (*this - vect).length(); }

    // Linearly interpolates from this vector to another.
    // Factor should be between 0.0 and 1.0, with 0 giving full value to this.
    V2Vect lerp(const V2Vect &vect, T factor) const { return *this * (T(1) - factor) + vect * factor; }

    // Projects this vector onto the argument; in other words,
    // A.Project(B) returns projection of vector A onto B.
    V2Vect projectTo(const V2Vect &vect) const
    {
        T length = vect.lengthSquared();
        return vect * (dotProduct(vect) / length);
    }
};


typedef V2Vect<float> V2Vectf;
typedef V2Vect<double> V2Vectd;
typedef V2Vect<int> V2Vecti;

NV_NAMESPACE_END
