/*
 * VAlgorithm.h
 *
 *  Created on: 2016年3月22日
 *      Author: yangkai
 */
#pragma once
#include <utility>
#include <algorithm>
#include <cmath>
#include <assert.h>
#include "vglobal.h"
#include "types.h"

using namespace std;

NV_NAMESPACE_BEGIN

namespace VAlgorithm {

template <typename T> inline const T Clamp(const T v, const T minVal, const T maxVal)
{
    return max(minVal, min(v, maxVal));
}

template<class Array>
typename Array::ValueType& Median(Array& arr)
{
    uint count = arr.size();
    uint mid = (count - 1) / 2;
    assert(count > 0);

    for (uint j = 0; j <= mid; j++)
    {
        uint min = j;
        for (uint k = j + 1; k < count; k++)
            if (arr[k] < arr[min])
                min = k;
        swap(arr[j], arr[min]);
    }
    return arr[mid];
}

}
NV_NAMESPACE_END
