#pragma once

#include "vglobal.h"

#include "Allocator.h"
#include <string.h>


NV_NAMESPACE_BEGIN


//-----------------------------------------------------------------------------------
// ***** Container Allocator

// ContainerAllocator serves as a template argument for allocations done by
// containers, such as Array and Hash; replacing it could allow allocator
// substitution in containers.

class ContainerAllocatorBase
{
public:
    static void* Alloc(uint size)                { return OVR_ALLOC(size); }
    static void* Realloc(void* p, uint newSize)  { return OVR_REALLOC(p, newSize); }
    static void  Free(void *p)                    { OVR_FREE(p); }
};



//-----------------------------------------------------------------------------------
// ***** Constructors, Destructors, Copiers

// Plain Old Data - movable, no special constructors/destructor.
template<class T>
class ConstructorPOD
{
public:
    static void Construct(void *) {}
    static void Construct(void *p, const T& source)
    {
        *(T*)p = source;
    }

    // Same as above, but allows for a different type of constructor.
    template <class S>
    static void ConstructAlt(void *p, const S& source)
    {
        *(T*)p = source;
    }

    static void ConstructArray(void*, uint) {}

    static void ConstructArray(void* p, uint count, const T& source)
    {
        UByte *pdata = (UByte*)p;
        for (uint i=0; i< count; ++i, pdata += sizeof(T))
            *(T*)pdata = source;
    }

    static void ConstructArray(void* p, uint count, const T* psource)
    {
        memcpy(p, psource, sizeof(T) * count);
    }

    static void Destruct(T*) {}
    static void DestructArray(T*, uint) {}

    static void CopyArrayForward(T* dst, const T* src, uint count)
    {
        memmove(dst, src, count * sizeof(T));
    }

    static void CopyArrayBackward(T* dst, const T* src, uint count)
    {
        memmove(dst, src, count * sizeof(T));
    }

    static bool IsMovable() { return true; }
};


//-----------------------------------------------------------------------------------
// ***** ConstructorMov
//
// Correct C++ construction and destruction for movable objects
template<class T>
class ConstructorMov
{
public:
    static void Construct(void* p)
    {
        NervGear::Construct<T>(p);
    }

    static void Construct(void* p, const T& source)
    {
        NervGear::Construct<T>(p, source);
    }

    // Same as above, but allows for a different type of constructor.
    template <class S>
    static void ConstructAlt(void* p, const S& source)
    {
        NervGear::ConstructAlt<T,S>(p, source);
    }

    static void ConstructArray(void* p, uint count)
    {
        UByte* pdata = (UByte*)p;
        for (uint i=0; i< count; ++i, pdata += sizeof(T))
            Construct(pdata);
    }

    static void ConstructArray(void* p, uint count, const T& source)
    {
        UByte* pdata = (UByte*)p;
        for (uint i=0; i< count; ++i, pdata += sizeof(T))
            Construct(pdata, source);
    }

    static void ConstructArray(void* p, uint count, const T* psource)
    {
        UByte* pdata = (UByte*)p;
        for (uint i=0; i< count; ++i, pdata += sizeof(T))
            Construct(pdata, *psource++);
    }

    static void Destruct(T* p)
    {
        p->~T();
        OVR_UNUSED(p); // Suppress silly MSVC warning
    }

    static void DestructArray(T* p, uint count)
    {
        p += count - 1;
        for (uint i=0; i<count; ++i, --p)
            p->~T();
    }

    static void CopyArrayForward(T* dst, const T* src, uint count)
    {
        memmove(dst, src, count * sizeof(T));
    }

    static void CopyArrayBackward(T* dst, const T* src, uint count)
    {
        memmove(dst, src, count * sizeof(T));
    }

    static bool IsMovable() { return true; }
};


//-----------------------------------------------------------------------------------
// ***** ConstructorCPP
//
// Correct C++ construction and destruction for movable objects
template<class T>
class ConstructorCPP
{
public:
    static void Construct(void* p)
    {
        NervGear::Construct<T>(p);
    }

    static void Construct(void* p, const T& source)
    {
        NervGear::Construct<T>(p, source);
    }

    // Same as above, but allows for a different type of constructor.
    template <class S>
    static void ConstructAlt(void* p, const S& source)
    {
        NervGear::ConstructAlt<T,S>(p, source);
    }

    static void ConstructArray(void* p, uint count)
    {
        UByte* pdata = (UByte*)p;
        for (uint i=0; i< count; ++i, pdata += sizeof(T))
            Construct(pdata);
    }

    static void ConstructArray(void* p, uint count, const T& source)
    {
        UByte* pdata = (UByte*)p;
        for (uint i=0; i< count; ++i, pdata += sizeof(T))
            Construct(pdata, source);
    }

    static void ConstructArray(void* p, uint count, const T* psource)
    {
        UByte* pdata = (UByte*)p;
        for (uint i=0; i< count; ++i, pdata += sizeof(T))
            Construct(pdata, *psource++);
    }

    static void Destruct(T* p)
    {
        p->~T();
        OVR_UNUSED(p); // Suppress silly MSVC warning
    }

    static void DestructArray(T* p, uint count)
    {
        p += count - 1;
        for (uint i=0; i<count; ++i, --p)
            p->~T();
    }

    static void CopyArrayForward(T* dst, const T* src, uint count)
    {
        for(uint i = 0; i < count; ++i)
            dst[i] = src[i];
    }

    static void CopyArrayBackward(T* dst, const T* src, uint count)
    {
        for(uint i = count; i; --i)
            dst[i-1] = src[i-1];
    }

    static bool IsMovable() { return false; }
};


//-----------------------------------------------------------------------------------
// ***** Container Allocator with movement policy
//
// Simple wraps as specialized allocators
template<class T> struct ContainerAllocator_POD : ContainerAllocatorBase, ConstructorPOD<T> {};
template<class T> struct ContainerAllocator     : ContainerAllocatorBase, ConstructorMov<T> {};
template<class T> struct ContainerAllocator_CPP : ContainerAllocatorBase, ConstructorCPP<T> {};


NV_NAMESPACE_END
