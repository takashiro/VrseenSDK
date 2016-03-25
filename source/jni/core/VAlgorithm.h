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
template <typename T> inline void Swap(T &a, T &b)
{
    swap(a, b);
}

template <typename T> inline const T Min(const T a, const T b)
{
    return min(a, b);
}

template <typename T> inline const T Max(const T a, const T b)
{
    return max(a, b);
}

template <typename T> inline const T Clamp(const T v, const T minVal, const T maxVal)
{
    return max(minVal, min(v, maxVal));
}

template <typename T> inline int Chop(T f)
{
    return int(f);
}

template <typename T> inline T Lerp(T a, T b, T f)
{
    return (b - a) * f + a;
}

template <typename T> inline const T Abs(const T v)
{
    return abs(v);
}

template <typename T>   inline const T PMin(const T a, const T b)
{
    assert(sizeof(T) == sizeof(uint));
    return (a < b) ? a : b;
}
template <typename T>   inline const T PMax(const T a, const T b)
{
    assert(sizeof(T) == sizeof(uint));
    return (b < a) ? a : b;
}

//-----------------------------------------------------------------------------------
// ***** OperatorLess
//
template<class T> struct OperatorLess
{
    static bool Compare(const T& a, const T& b)
    {
        return a < b;
    }
};


//-----------------------------------------------------------------------------------
// ***** QuickSortSliced
//
// Sort any part of any array: plain, Array, ArrayPaged, ArrayUnsafe.
// The range is specified with start, end, where "end" is exclusive!
// The comparison predicate must be specified.
template<class Array, class Less>
void QuickSortSliced(Array& arr, uint start, uint end, Less less)
{
    enum
    {
        Threshold = 9
    };

    if(end - start <  2) return;

    int  stack[80];
    int* top   = stack;
    int  base  = (int)start;
    int  limit = (int)end;

    for(;;)
    {
        int len = limit - base;
        int i, j, pivot;

        if(len > Threshold)
        {
            // we use base + len/2 as the pivot
            pivot = base + len / 2;
            Swap(arr[base], arr[pivot]);

            i = base + 1;
            j = limit - 1;

            // now ensure that *i <= *base <= *j
            if(less(arr[j],    arr[i])) Swap(arr[j],    arr[i]);
            if(less(arr[base], arr[i])) Swap(arr[base], arr[i]);
            if(less(arr[j], arr[base])) Swap(arr[j], arr[base]);

            for(;;)
            {
                do i++; while( less(arr[i], arr[base]) );
                do j--; while( less(arr[base], arr[j]) );

                if( i > j )
                {
                    break;
                }

                Swap(arr[i], arr[j]);
            }

            Swap(arr[base], arr[j]);

            // now, push the largest sub-array
            if(j - base > limit - i)
            {
                top[0] = base;
                top[1] = j;
                base   = i;
            }
            else
            {
                top[0] = i;
                top[1] = limit;
                limit  = j;
            }
            top += 2;
        }
        else
        {
            // the sub-array is small, perform insertion sort
            j = base;
            i = j + 1;

            for(; i < limit; j = i, i++)
            {
                for(; less(arr[j + 1], arr[j]); j--)
                {
                    Swap(arr[j + 1], arr[j]);
                    if(j == base)
                    {
                        break;
                    }
                }
            }
            if(top > stack)
            {
                top  -= 2;
                base  = top[0];
                limit = top[1];
            }
            else
            {
                break;
            }
        }
    }
}


//-----------------------------------------------------------------------------------
// ***** QuickSortSliced
//
// Sort any part of any array: plain, Array, ArrayPaged, ArrayUnsafe.
// The range is specified with start, end, where "end" is exclusive!
// The data type must have a defined "<" operator.
template<class Array>
void QuickSortSliced(Array& arr, uint start, uint end)
{
    typedef typename Array::ValueType ValueType;
    QuickSortSliced(arr, start, end, OperatorLess<ValueType>::Compare);
}

// Same as corresponding G_QuickSortSliced but with checking array limits to avoid
// crash in the case of wrong comparator functor.
template<class Array, class Less>
bool QuickSortSlicedSafe(Array& arr, uint start, uint end, Less less)
{
    enum
    {
        Threshold = 9
    };

    if(end - start <  2) return true;

    int  stack[80];
    int* top   = stack;
    int  base  = (int)start;
    int  limit = (int)end;

    for(;;)
    {
        int len = limit - base;
        int i, j, pivot;

        if(len > Threshold)
        {
            // we use base + len/2 as the pivot
            pivot = base + len / 2;
            Swap(arr[base], arr[pivot]);

            i = base + 1;
            j = limit - 1;

            // now ensure that *i <= *base <= *j
            if(less(arr[j],    arr[i])) Swap(arr[j],    arr[i]);
            if(less(arr[base], arr[i])) Swap(arr[base], arr[i]);
            if(less(arr[j], arr[base])) Swap(arr[j], arr[base]);

            for(;;)
            {
                do
                {
                    i++;
                    if (i >= limit)
                        return false;
                } while( less(arr[i], arr[base]) );
                do
                {
                    j--;
                    if (j < 0)
                        return false;
                } while( less(arr[base], arr[j]) );

                if( i > j )
                {
                    break;
                }

                Swap(arr[i], arr[j]);
            }

            Swap(arr[base], arr[j]);

            // now, push the largest sub-array
            if(j - base > limit - i)
            {
                top[0] = base;
                top[1] = j;
                base   = i;
            }
            else
            {
                top[0] = i;
                top[1] = limit;
                limit  = j;
            }
            top += 2;
        }
        else
        {
            // the sub-array is small, perform insertion sort
            j = base;
            i = j + 1;

            for(; i < limit; j = i, i++)
            {
                for(; less(arr[j + 1], arr[j]); j--)
                {
                    Swap(arr[j + 1], arr[j]);
                    if(j == base)
                    {
                        break;
                    }
                }
            }
            if(top > stack)
            {
                top  -= 2;
                base  = top[0];
                limit = top[1];
            }
            else
            {
                break;
            }
        }
    }
    return true;
}

template<class Array>
bool QuickSortSlicedSafe(Array& arr, uint start, uint end)
{
    typedef typename Array::ValueType ValueType;
    return QuickSortSlicedSafe(arr, start, end, OperatorLess<ValueType>::Compare);
}

//-----------------------------------------------------------------------------------
// ***** QuickSort
//
// Sort an array Array, ArrayPaged, ArrayUnsafe.
// The array must have GetSize() function.
// The comparison predicate must be specified.
template<class Array, class Less>
void QuickSort(Array& arr, Less less)
{
    QuickSortSliced(arr, 0, arr.GetSize(), less);
}

// checks for boundaries
template<class Array, class Less>
bool QuickSortSafe(Array& arr, Less less)
{
    return QuickSortSlicedSafe(arr, 0, arr.GetSize(), less);
}


//-----------------------------------------------------------------------------------
// ***** QuickSort
//
// Sort an array Array, ArrayPaged, ArrayUnsafe.
// The array must have GetSize() function.
// The data type must have a defined "<" operator.
template<class Array>
void QuickSort(Array& arr)
{
    typedef typename Array::ValueType ValueType;
    QuickSortSliced(arr, 0, arr.size(), OperatorLess<ValueType>::Compare);
}

template<class Array>
bool QuickSortSafe(Array& arr)
{
    typedef typename Array::ValueType ValueType;
    return QuickSortSlicedSafe(arr, 0, arr.GetSize(), OperatorLess<ValueType>::Compare);
}

//-----------------------------------------------------------------------------------
// ***** InsertionSortSliced
//
// Sort any part of any array: plain, Array, ArrayPaged, ArrayUnsafe.
// The range is specified with start, end, where "end" is exclusive!
// The comparison predicate must be specified.
// Unlike Quick Sort, the Insertion Sort works much slower in average,
// but may be much faster on almost sorted arrays. Besides, it guarantees
// that the elements will not be swapped if not necessary. For example,
// an array with all equal elements will remain "untouched", while
// Quick Sort will considerably shuffle the elements in this case.
template<class Array, class Less>
void InsertionSortSliced(Array& arr, uint start, uint end, Less less)
{
    uint j = start;
    uint i = j + 1;
    uint limit = end;

    for(; i < limit; j = i, i++)
    {
        for(; less(arr[j + 1], arr[j]); j--)
        {
            Swap(arr[j + 1], arr[j]);
            if(j <= start)
            {
                break;
            }
        }
    }
}


//-----------------------------------------------------------------------------------
// ***** InsertionSortSliced
//
// Sort any part of any array: plain, Array, ArrayPaged, ArrayUnsafe.
// The range is specified with start, end, where "end" is exclusive!
// The data type must have a defined "<" operator.
template<class Array>
void InsertionSortSliced(Array& arr, uint start, uint end)
{
    typedef typename Array::ValueType ValueType;
    InsertionSortSliced(arr, start, end, OperatorLess<ValueType>::Compare);
}

//-----------------------------------------------------------------------------------
// ***** InsertionSort
//
// Sort an array Array, ArrayPaged, ArrayUnsafe.
// The array must have GetSize() function.
// The comparison predicate must be specified.

template<class Array, class Less>
void InsertionSort(Array& arr, Less less)
{
    InsertionSortSliced(arr, 0, arr.GetSize(), less);
}

//-----------------------------------------------------------------------------------
// ***** InsertionSort
//
// Sort an array Array, ArrayPaged, ArrayUnsafe.
// The array must have GetSize() function.
// The data type must have a defined "<" operator.
template<class Array>
void InsertionSort(Array& arr)
{
    typedef typename Array::ValueType ValueType;
    InsertionSortSliced(arr, 0, arr.GetSize(), OperatorLess<ValueType>::Compare);
}

//-----------------------------------------------------------------------------------
// ***** Median
// Returns a median value of the input array.
// Caveats: partially sorts the array, returns a reference to the array element
// TBD: This needs to be optimized and generalized
//
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
        Swap(arr[j], arr[min]);
    }
    return arr[mid];
}

//-----------------------------------------------------------------------------------
// ***** LowerBoundSliced
//
template<class Array, class Value, class Less>
uint LowerBoundSliced(const Array& arr, uint start, uint end, const Value& val, Less less)
{
    int first = (int)start;
    int len   = (int)(end - start);
    int half;
    int middle;

    while(len > 0)
    {
        half = len >> 1;
        middle = first + half;
        if(less(arr[middle], val))
        {
            first = middle + 1;
            len   = len - half - 1;
        }
        else
        {
            len = half;
        }
    }
    return (uint)first;
}


//-----------------------------------------------------------------------------------
// ***** LowerBoundSliced
//
template<class Array, class Value>
uint LowerBoundSliced(const Array& arr, uint start, uint end, const Value& val)
{
    return LowerBoundSliced(arr, start, end, val, OperatorLess<Value>::Compare);
}

//-----------------------------------------------------------------------------------
// ***** LowerBoundSized
//
template<class Array, class Value>
uint LowerBoundSized(const Array& arr, uint size, const Value& val)
{
    return LowerBoundSliced(arr, 0, size, val, OperatorLess<Value>::Compare);
}

//-----------------------------------------------------------------------------------
// ***** LowerBound
//
template<class Array, class Value, class Less>
uint LowerBound(const Array& arr, const Value& val, Less less)
{
    return LowerBoundSliced(arr, 0, arr.GetSize(), val, less);
}


//-----------------------------------------------------------------------------------
// ***** LowerBound
//
template<class Array, class Value>
uint LowerBound(const Array& arr, const Value& val)
{
    return LowerBoundSliced(arr, 0, arr.GetSize(), val, OperatorLess<Value>::Compare);
}



//-----------------------------------------------------------------------------------
// ***** UpperBoundSliced
//
template<class Array, class Value, class Less>
uint UpperBoundSliced(const Array& arr, uint start, uint end, const Value& val, Less less)
{
    int first = (int)start;
    int len   = (int)(end - start);
    int half;
    int middle;

    while(len > 0)
    {
        half = len >> 1;
        middle = first + half;
        if(less(val, arr[middle]))
        {
            len = half;
        }
        else
        {
            first = middle + 1;
            len   = len - half - 1;
        }
    }
    return (uint)first;
}


//-----------------------------------------------------------------------------------
// ***** UpperBoundSliced
//
template<class Array, class Value>
uint UpperBoundSliced(const Array& arr, uint start, uint end, const Value& val)
{
    return UpperBoundSliced(arr, start, end, val, OperatorLess<Value>::Compare);
}


//-----------------------------------------------------------------------------------
// ***** UpperBoundSized
//
template<class Array, class Value>
uint UpperBoundSized(const Array& arr, uint size, const Value& val)
{
    return UpperBoundSliced(arr, 0, size, val, OperatorLess<Value>::Compare);
}


//-----------------------------------------------------------------------------------
// ***** UpperBound
//
template<class Array, class Value, class Less>
uint UpperBound(const Array& arr, const Value& val, Less less)
{
    return UpperBoundSliced(arr, 0, arr.GetSize(), val, less);
}


//-----------------------------------------------------------------------------------
// ***** UpperBound
//
template<class Array, class Value>
uint UpperBound(const Array& arr, const Value& val)
{
    return UpperBoundSliced(arr, 0, arr.GetSize(), val, OperatorLess<Value>::Compare);
}


//-----------------------------------------------------------------------------------
// ***** ReverseArray
//
template<class Array> void ReverseArray(Array& arr)
{
    int from = 0;
    int to   = arr.GetSize() - 1;
    while(from < to)
    {
        Swap(arr[from], arr[to]);
        ++from;
        --to;
    }
}


// ***** AppendArray
//
template<class CDst, class CSrc>
void AppendArray(CDst& dst, const CSrc& src)
{
    uint i;
    for(i = 0; i < src.GetSize(); i++)
        dst.PushBack(src[i]);
}

//-----------------------------------------------------------------------------------
// ***** MergeArray
//
template<class CDst, class CSrc, class Less>
void MergeArray( CDst & dst, const CSrc & src, Less less )
{
    uint dstIndex = dst.GetSize();
    uint srcIndex = src.GetSize();
    uint finalIndex = dstIndex + srcIndex;
    dst.Resize( finalIndex );
    while ( srcIndex > 0 )
    {
        if ( dstIndex > 0 && less( src[srcIndex - 1], dst[dstIndex - 1] ) )
        {
            Swap( dst[finalIndex - 1], dst[dstIndex - 1] );
            dstIndex--;
        }
        else
        {
            dst[finalIndex - 1] = src[srcIndex - 1];
            srcIndex--;
        }
        finalIndex--;
    }
}

template<class CDst, class CSrc>
void MergeArray( CDst & dst, const CSrc & src )
{
    typedef typename CDst::ValueType ValueType;
    MergeArray( dst, src, OperatorLess<ValueType>::Compare );
}

//-----------------------------------------------------------------------------------
// ***** ArrayAdaptor
//
// A simple adapter that provides the GetSize() method and overloads
// operator []. Used to wrap plain arrays in QuickSort and such.
template<class T> class ArrayAdaptor
{
public:
    typedef T ValueType;
    ArrayAdaptor() : Data(0), Size(0) {}
    ArrayAdaptor(T* ptr, uint size) : Data(ptr), Size(size) {}
    uint GetSize() const { return Size; }
    const T& operator [] (uint i) const { return Data[i]; }
          T& operator [] (uint i)       { return Data[i]; }
private:
    T*      Data;
    uint   Size;
};


//-----------------------------------------------------------------------------------
// ***** GConstArrayAdaptor
//
// A simple const adapter that provides the GetSize() method and overloads
// operator []. Used to wrap plain arrays in LowerBound and such.
template<class T> class ConstArrayAdaptor
{
public:
    typedef T ValueType;
    ConstArrayAdaptor() : Data(0), Size(0) {}
    ConstArrayAdaptor(const T* ptr, uint size) : Data(ptr), Size(size) {}
    uint GetSize() const { return Size; }
    const T& operator [] (uint i) const { return Data[i]; }
private:
    const T* Data;
    uint    Size;
};



//-----------------------------------------------------------------------------------
extern const UByte UpperBitTable[256];
extern const UByte LowerBitTable[256];



//-----------------------------------------------------------------------------------
inline UByte UpperBit(uint val)
{
#ifndef OVR_64BIT_POINTERS

    if (val & 0xFFFF0000)
    {
        return (val & 0xFF000000) ?
            UpperBitTable[(val >> 24)       ] + 24:
            UpperBitTable[(val >> 16) & 0xFF] + 16;
    }
    return (val & 0xFF00) ?
        UpperBitTable[(val >> 8) & 0xFF] + 8:
        UpperBitTable[(val     ) & 0xFF];

#else

    if (val & 0xFFFFFFFF00000000)
    {
        if (val & 0xFFFF000000000000)
        {
            return (val & 0xFF00000000000000) ?
                UpperBitTable[(val >> 56)       ] + 56:
                UpperBitTable[(val >> 48) & 0xFF] + 48;
        }
        return (val & 0xFF0000000000) ?
            UpperBitTable[(val >> 40) & 0xFF] + 40:
            UpperBitTable[(val >> 32) & 0xFF] + 32;
    }
    else
    {
        if (val & 0xFFFF0000)
        {
            return (val & 0xFF000000) ?
                UpperBitTable[(val >> 24)       ] + 24:
                UpperBitTable[(val >> 16) & 0xFF] + 16;
        }
        return (val & 0xFF00) ?
            UpperBitTable[(val >> 8) & 0xFF] + 8:
            UpperBitTable[(val     ) & 0xFF];
    }

#endif
}

//-----------------------------------------------------------------------------------
inline UByte LowerBit(uint val)
{
#ifndef OVR_64BIT_POINTERS

    if (val & 0xFFFF)
    {
        return (val & 0xFF) ?
            LowerBitTable[ val & 0xFF]:
            LowerBitTable[(val >> 8) & 0xFF] + 8;
    }
    return (val & 0xFF0000) ?
            LowerBitTable[(val >> 16) & 0xFF] + 16:
            LowerBitTable[(val >> 24) & 0xFF] + 24;

#else

    if (val & 0xFFFFFFFF)
    {
        if (val & 0xFFFF)
        {
            return (val & 0xFF) ?
                LowerBitTable[ val & 0xFF]:
                LowerBitTable[(val >> 8) & 0xFF] + 8;
        }
        return (val & 0xFF0000) ?
                LowerBitTable[(val >> 16) & 0xFF] + 16:
                LowerBitTable[(val >> 24) & 0xFF] + 24;
    }
    else
    {
        if (val & 0xFFFF00000000)
        {
             return (val & 0xFF00000000) ?
                LowerBitTable[(val >> 32) & 0xFF] + 32:
                LowerBitTable[(val >> 40) & 0xFF] + 40;
        }
        return (val & 0xFF000000000000) ?
            LowerBitTable[(val >> 48) & 0xFF] + 48:
            LowerBitTable[(val >> 56) & 0xFF] + 56;
    }

#endif
}



// ******* Special (optimized) memory routines
// Note: null (bad) pointer is not tested
class MemUtil
{
public:

    // Memory compare
    static int      Cmp  (const void* p1, const void* p2, uint byteCount)      { return memcmp(p1, p2, byteCount); }
    static int      Cmp16(const void* p1, const void* p2, uint int16Count);
    static int      Cmp32(const void* p1, const void* p2, uint int32Count);
    static int      Cmp64(const void* p1, const void* p2, uint int64Count);
};

// ** Inline Implementation

inline int MemUtil::Cmp16(const void* p1, const void* p2, uint int16Count)
{
    SInt16*  pa  = (SInt16*)p1;
    SInt16*  pb  = (SInt16*)p2;
    unsigned ic  = 0;
    if (int16Count == 0)
        return 0;
    while (pa[ic] == pb[ic])
        if (++ic==int16Count)
            return 0;
    return pa[ic] > pb[ic] ? 1 : -1;
}
inline int MemUtil::Cmp32(const void* p1, const void* p2, uint int32Count)
{
    SInt32*  pa  = (SInt32*)p1;
    SInt32*  pb  = (SInt32*)p2;
    unsigned ic  = 0;
    if (int32Count == 0)
        return 0;
    while (pa[ic] == pb[ic])
        if (++ic==int32Count)
            return 0;
    return pa[ic] > pb[ic] ? 1 : -1;
}
inline int MemUtil::Cmp64(const void* p1, const void* p2, uint int64Count)
{
    SInt64*  pa  = (SInt64*)p1;
    SInt64*  pb  = (SInt64*)p2;
    unsigned ic  = 0;
    if (int64Count == 0)
        return 0;
    while (pa[ic] == pb[ic])
        if (++ic==int64Count)
            return 0;
    return pa[ic] > pb[ic] ? 1 : -1;
}

// ** End Inline Implementation


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
            Swap( b[0], b[1] );
        }
        else if ( sizeof( _type_ ) == 4 )
        {
            Swap( b[0], b[3] );
            Swap( b[1], b[2] );
        }
        else if ( sizeof( _type_ ) == 8 )
        {
            Swap( b[0], b[7] );
            Swap( b[1], b[6] );
            Swap( b[2], b[5] );
            Swap( b[3], b[4] );
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

// Converts an 8-bit binary-coded decimal
inline SByte DecodeBCD(UByte byte)
{
    UByte digit1 = (byte >> 4) & 0x0f;
    UByte digit2 = byte & 0x0f;
    int decimal = digit1 * 10 + digit2;   // maximum value = 99
    return (SByte)decimal;
}

}
NV_NAMESPACE_END
