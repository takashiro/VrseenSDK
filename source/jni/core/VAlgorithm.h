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

template <typename T> inline const T Max(const T a, const T b)
{
    return max(a, b);
}

template <typename T> inline const T Clamp(const T v, const T minVal, const T maxVal)
{
    return max(minVal, min(v, maxVal));
}

template <typename T> inline T Lerp(T a, T b, T f)
{
    return (b - a) * f + a;
}

template <typename T> inline const T Abs(const T v)
{
    return abs(v);
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

//-----------------------------------------------------------------------------------
extern const UByte UpperBitTable[256];
extern const UByte LowerBitTable[256];

//-----------------------------------------------------------------------------------
// ******* Byte Order Conversions
namespace ByteUtil {

    // *** Swap Byte Order
    template< typename _type_ >
    inline _type_ SwapOrder( const _type_ & value )
    {
        _type_ bytes = value;
        char * b = (char *)& bytes;
        if ( sizeof( _type_ ) == 1 )
        {
        }
        else if ( sizeof( _type_ ) == 2 )
        {
            swap( b[0], b[1] );
        }
        else if ( sizeof( _type_ ) == 4 )
        {
            swap( b[0], b[3] );
            swap( b[1], b[2] );
        }
        else if ( sizeof( _type_ ) == 8 )
        {
            swap( b[0], b[7] );
            swap( b[1], b[6] );
            swap( b[2], b[5] );
            swap( b[3], b[4] );
        }
        else
        {
            assert( false );
        }
        return bytes;
    }

    // *** Byte-order conversion

#if (OVR_BYTE_ORDER == OVR_LITTLE_ENDIAN)
    // Little Endian to System (LE)
    template< typename _type_ >
    inline _type_ LEToSystem( _type_ v ) { return v; }

    // Big Endian to System (LE)
    template< typename _type_ >
    inline _type_ BEToSystem( _type_ v ) { return SwapOrder( v ); }

    // System (LE) to Little Endian
    template< typename _type_ >
    inline _type_ SystemToLE( _type_ v ) { return v; }

    // System (LE) to Big Endian
    template< typename _type_ >
    inline _type_ SystemToBE( _type_ v ) { return SwapOrder( v ); }

#elif (OVR_BYTE_ORDER == OVR_BIG_ENDIAN)
    // Little Endian to System (BE)
    template< typename _type_ >
    inline _type_ LEToSystem( _type_ v ) { return SwapOrder( v ); }

    // Big Endian to System (BE)
    template< typename _type_ >
    inline _type_ BEToSystem( _type_ v ) { return v; }

    // System (BE) to Little Endian
    template< typename _type_ >
    inline _type_ SystemToLE( _type_ v ) { return SwapOrder( v ); }

    // System (BE) to Big Endian
    template< typename _type_ >
    inline _type_ SystemToBE( _type_ v ) { return v; }

#else
    #error "OVR_BYTE_ORDER must be defined to OVR_LITTLE_ENDIAN or OVR_BIG_ENDIAN"
#endif

} // namespace ByteUtil



// Used primarily for hardware interfacing such as sensor reports, firmware, etc.
// Reported data is all little-endian.
inline UInt16 DecodeUInt16(const UByte* buffer)
{
    return ByteUtil::LEToSystem ( *(const UInt16*)buffer );
}

inline SInt16 DecodeSInt16(const UByte* buffer)
{
    return ByteUtil::LEToSystem ( *(const SInt16*)buffer );
}

inline UInt32 DecodeUInt32(const UByte* buffer)
{
    return ByteUtil::LEToSystem ( *(const UInt32*)buffer );
}

inline SInt32 DecodeSInt32(const UByte* buffer)
{
    return ByteUtil::LEToSystem ( *(const SInt32*)buffer );
}

inline float DecodeFloat(const UByte* buffer)
{
    union {
        UInt32 U;
        float  F;
    };

    U = DecodeUInt32(buffer);
    return F;
}

inline void EncodeUInt16(UByte* buffer, UInt16 val)
{
    *(UInt16*)buffer = ByteUtil::SystemToLE ( val );
}

inline void EncodeSInt16(UByte* buffer, SInt16 val)
{
    *(SInt16*)buffer = ByteUtil::SystemToLE ( val );
}

inline void EncodeUInt32(UByte* buffer, UInt32 val)
{
    *(UInt32*)buffer = ByteUtil::SystemToLE ( val );
}

inline void EncodeSInt32(UByte* buffer, SInt32 val)
{
    *(SInt32*)buffer = ByteUtil::SystemToLE ( val );
}

inline void EncodeFloat(UByte* buffer, float val)
{
    union {
        UInt32 U;
        float  F;
    };

    F = val;
    EncodeUInt32(buffer, U);
}

}
NV_NAMESPACE_END
