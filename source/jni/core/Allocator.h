#pragma once

#include "vglobal.h"
#include "Types.h"

#include <new>

//------------------------------------------------------------------------
// ***** Macros to redefine class new/delete operators

// Types specifically declared to allow disambiguation of address in
// class member operator new.

#define OVR_MEMORY_REDEFINE_NEW_IMPL(class_name, check_delete)                          \
    void*   operator new(uint sz)                                                      \
    { void *p = OVR_ALLOC_DEBUG(sz, __FILE__, __LINE__); return p; }                                              \
    void*   operator new(uint sz, const char* file, int line)                          \
    { void* p = OVR_ALLOC_DEBUG(sz, file, line); OVR_UNUSED2(file, line); return p; }   \
    void    operator delete(void *p)                                                    \
    { check_delete(class_name, p); OVR_FREE(p); }                                       \
    void    operator delete(void *p, const char*, int)                                  \
    { check_delete(class_name, p); OVR_FREE(p); }

#define OVR_MEMORY_DEFINE_PLACEMENT_NEW                                                 \
    void*   operator new        (UPInt n, void *ptr)    { OVR_UNUSED(n); return ptr; }  \
    void    operator delete     (void *ptr, void *ptr2) { OVR_UNUSED2(ptr,ptr2); }


#define OVR_MEMORY_CHECK_DELETE_NONE(class_name, p)

// Redefined all delete/new operators in a class without custom memory initialization
#define OVR_MEMORY_REDEFINE_NEW(class_name) \
    OVR_MEMORY_REDEFINE_NEW_IMPL(class_name, OVR_MEMORY_CHECK_DELETE_NONE)


NV_NAMESPACE_BEGIN

//-----------------------------------------------------------------------------------
// ***** Construct / Destruct

// Construct/Destruct functions are useful when new is redefined, as they can
// be called instead of placement new constructors.


template <class T>
OVR_FORCE_INLINE T*  Construct(void *p)
{
    return ::new(p) T;
}

template <class T>
OVR_FORCE_INLINE T*  Construct(void *p, const T& source)
{
    return ::new(p) T(source);
}

// Same as above, but allows for a different type of constructor.
template <class T, class S>
OVR_FORCE_INLINE T*  ConstructAlt(void *p, const S& source)
{
    return ::new(p) T(source);
}

template <class T, class S1, class S2>
OVR_FORCE_INLINE T*  ConstructAlt(void *p, const S1& src1, const S2& src2)
{
    return ::new(p) T(src1, src2);
}

template <class T>
OVR_FORCE_INLINE void ConstructArray(void *p, uint count)
{
    UByte *pdata = (UByte*)p;
    for (uint i=0; i< count; ++i, pdata += sizeof(T))
    {
        Construct<T>(pdata);
    }
}

template <class T>
OVR_FORCE_INLINE void ConstructArray(void *p, uint count, const T& source)
{
    UByte *pdata = (UByte*)p;
    for (uint i=0; i< count; ++i, pdata += sizeof(T))
    {
        Construct<T>(pdata, source);
    }
}

template <class T>
OVR_FORCE_INLINE void Destruct(T *pobj)
{
    pobj->~T();
    OVR_UNUSED1(pobj); // Fix incorrect 'unused variable' MSVC warning.
}

template <class T>
OVR_FORCE_INLINE void DestructArray(T *pobj, uint count)
{
    for (uint i=0; i<count; ++i, ++pobj)
        pobj->~T();
}


//-----------------------------------------------------------------------------------
// ***** Allocator

// Allocator defines a memory allocation interface that developers can override
// to to provide memory for OVR; an instance of this class is typically created on
// application startup and passed into System or System constructor.
//
//
// Users implementing this interface must provide three functions: Alloc, Free,
// and Realloc. Implementations of these functions must honor the requested alignment.
// Although arbitrary alignment requests are possible, requested alignment will
// typically be small, such as 16 bytes or less.

class Allocator
{
    friend class System;
public:

    // *** Standard Alignment Alloc/Free

    // Allocate memory of specified size with default alignment.
    // Alloc of size==0 will allocate a tiny block & return a valid pointer;
    // this makes it suitable for new operator.
    virtual void*   Alloc(uint size) = 0;
    // Same as Alloc, but provides an option of passing debug data.
    virtual void*   AllocDebug(uint size, const char* file, unsigned line)
    { OVR_UNUSED2(file, line); return Alloc(size); }

    // Reallocate memory block to a new size, copying data if necessary. Returns the pointer to
    // new memory block, which may be the same as original pointer. Will return 0 if reallocation
    // failed, in which case previous memory is still valid.
    // Realloc to decrease size will never fail.
    // Realloc of pointer == 0 is equivalent to Alloc
    // Realloc to size == 0, shrinks to the minimal size, pointer remains valid and requires Free().
    virtual void*   Realloc(void* p, uint newSize) = 0;

    // Frees memory allocated by Alloc/Realloc.
    // Free of null pointer is valid and will do nothing.
    virtual void    Free(void *p) = 0;


    // *** Standard Alignment Alloc/Free

    // Allocate memory of specified alignment.
    // Memory allocated with AllocAligned MUST be freed with FreeAligned.
    // Default implementation will delegate to Alloc/Free after doing rounding.
    virtual void*   AllocAligned(uint size, uint align);
    // Frees memory allocated with AllocAligned.
    virtual void    FreeAligned(void* p);

    // Returns the pointer to the current globally installed Allocator instance.
    // This pointer is used for most of the memory allocations.
    static Allocator* GetInstance() { return pInstance; }


protected:
    // onSystemShutdown is called on the allocator during System::Shutdown.
    // At this point, all allocations should've been freed.
    virtual void    onSystemShutdown() { }

public:
    static  void    setInstance(Allocator* palloc)
    {
        OVR_ASSERT((pInstance == 0) || (palloc == 0));
        pInstance = palloc;
    }

private:

    static Allocator* pInstance;
};



//------------------------------------------------------------------------
// ***** Allocator_SingletonSupport

// Allocator_SingletonSupport is a Allocator wrapper class that implements
// the InitSystemSingleton static function, used to create a global singleton
// used for the System default argument initialization.
//
// End users implementing custom Allocator interface don't need to make use of this base
// class; they can just create an instance of their own class on stack and pass it to System.

template<class D>
class Allocator_SingletonSupport : public Allocator
{
    struct AllocContainer
    {
        uint Data[(sizeof(D) + sizeof(uint)-1) / sizeof(uint)];
        bool  Initialized;
        AllocContainer() : Initialized(0) { }
    };

    AllocContainer* pContainer;

public:
    Allocator_SingletonSupport() : pContainer(0) { }

    // Creates a singleton instance of this Allocator class used
    // on OVR_DEFAULT_ALLOCATOR during System initialization.
    static  D*  InitSystemSingleton()
    {
        static AllocContainer Container;
        OVR_ASSERT(Container.Initialized == false);

        Allocator_SingletonSupport<D> *presult = Construct<D>((void*)Container.Data);
        presult->pContainer   = &Container;
        Container.Initialized = true;
        return (D*)presult;
    }

protected:
    virtual void onSystemShutdown()
    {
        Allocator::onSystemShutdown();
        if (pContainer)
        {
            pContainer->Initialized = false;
            Destruct((D*)this);
            pContainer = 0;
        }
    }
};

//------------------------------------------------------------------------
// ***** Default Allocator

// This allocator is created and used if no other allocator is installed.
// Default allocator delegates to system malloc.

class DefaultAllocator : public Allocator_SingletonSupport<DefaultAllocator>
{
public:
    virtual void*   Alloc(uint size);
    virtual void*   AllocDebug(uint size, const char* file, unsigned line);
    virtual void*   Realloc(void* p, uint newSize);
    virtual void    Free(void *p);
};


//------------------------------------------------------------------------
// ***** Memory Allocation Macros

// These macros should be used for global allocation. In the future, these
// macros will allows allocation to be extended with debug file/line information
// if necessary.

#define OVR_REALLOC(p,s)        NV_NAMESPACE::Allocator::GetInstance()->Realloc((p),(s))
#define OVR_FREE(p)             NV_NAMESPACE::Allocator::GetInstance()->Free((p))
#define OVR_ALLOC_ALIGNED(s,a)  NV_NAMESPACE::Allocator::GetInstance()->AllocAligned((s),(a))
#define OVR_FREE_ALIGNED(p)     NV_NAMESPACE::Allocator::GetInstance()->FreeAligned((p))

#ifdef OVR_BUILD_DEBUG
#define OVR_ALLOC(s)            NV_NAMESPACE::Allocator::GetInstance()->AllocDebug((s), __FILE__, __LINE__)
#define OVR_ALLOC_DEBUG(s,f,l)  NV_NAMESPACE::Allocator::GetInstance()->AllocDebug((s), f, l)
#else
#define OVR_ALLOC(s)            NV_NAMESPACE::Allocator::GetInstance()->Alloc((s))
#define OVR_ALLOC_DEBUG(s,f,l)  NV_NAMESPACE::Allocator::GetInstance()->Alloc((s))
#endif

//------------------------------------------------------------------------

// Base class that overrides the new and delete operators.
// Deriving from this class, even as a multiple base, incurs no space overhead.
class NewOverrideBase
{
public:

    // Redefine all new & delete operators.
    OVR_MEMORY_REDEFINE_NEW(NewOverrideBase)
};


} // OVR


// Redefine operator 'new' if necessary.
#if defined(OVR_DEFINE_NEW)
#define new OVR_DEFINE_NEW
#endif
