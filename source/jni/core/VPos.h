#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

template<typename T>
class VPos
{
public:
    T x;
    T y;
    T z;

    VPos()
        : x(0)
        , y(0)
        , z(0)
    {
    }

    VPos(T x, T y, T z)
        : x(x)
        , y(y)
        , z(z)
    {
    }

    VPos &operator+=(const VPos &pos)
    {
        x += pos.x;
        y += pos.y;
        z += pos.z;
        return *this;
    }

    VPos &operator-=(const VPos &pos)
    {
        x -= pos.x;
        y -= pos.y;
        z -= pos.z;
        return *this;
    }

    VPos &operator*=(T factor)
    {
        x *= factor;
        y *= factor;
        z *= factor;
        return *this;
    }

    VPos &operator/=(T divisor)
    {
        x /= divisor;
        y /= divisor;
        z /= divisor;
        return *this;
    }

    friend bool operator==(const VPos &p1, const VPos &p2)
    {
        return p1.x == p2.x && p1.y == p2.y && p1.z == p2.z;
    }

    friend bool operator!=(const VPos &p1, const VPos &p2)
    {
        return p1.x != p2.x || p1.y != p2.y || p1.z != p2.z;
    }

    friend VPos operator+(const VPos &p1, const VPos &p2)
    {
        VPos result = p1;
        result += p2;
        return result;
    }

    friend VPos operator-(const VPos &p1, const VPos &p2)
    {
        VPos result = p1;
        result -= p2;
        return result;
    }

    friend VPos operator-(const VPos &pos)
    {
        return VPos(-pos.x, -pos.y, -pos.z);
    }

    friend VPos operator*(const VPos &pos, vreal factor)
    {
        VPos result = pos;
        result *= factor;
        return result;
    }

    friend VPos operator*(vreal factor, const VPos &pos)
    {
        return pos * factor;
    }

    friend VPos operator/(const VPos &pos, vreal divisor)
    {
        VPos result = pos;
        result /= divisor;
        return result;
    }
};

typedef VPos<vreal> VPosF;

NV_NAMESPACE_END
