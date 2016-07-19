#pragma once

#include "VVect3.h"

NV_NAMESPACE_BEGIN

template<class T>
class VRect3
{
public:
    VVect3<T> start;
    VVect3<T> end;

    VRect3()
    {
    }

    VRect3(T x1, T y1, T z1, T x2, T y2, T z2)
        : start(x1, y1, z1)
        , end(x2, y2, z2)
    {
    }

    VRect3(const VVect3<T> &start, const VVect3<T> &end)
        : start(start)
        , end(end)
    {
    }

    VVect3<T> center() const { return (start + end) / 2; }

    VVect3<T> size() const { return end - start; }
};

typedef VRect3<float> VRect3f;
typedef VRect3<double> VRect3d;
typedef VRect3<int> VRect3i;

NV_NAMESPACE_END
