/************************************************************************************

Filename    :   OVR_Allocator.cpp
Content     :   Installable memory allocator implementation
Created     :   September 19, 2012
Notes       :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#include "Allocator.h"
#ifdef OVR_OS_MAC
 #include <stdlib.h>
#else
 #include <malloc.h>
#endif

namespace NervGear {

//-----------------------------------------------------------------------------------
// ***** Allocator

Allocator* Allocator::pInstance = 0;

// Default AlignedAlloc implementation will delegate to Alloc/Free after doing rounding.
void* Allocator::AllocAligned(uint size, uint align)
{
    OVR_ASSERT((align & (align-1)) == 0);
    align = (align > sizeof(uint)) ? align : sizeof(uint);
    uint p = (uint)Alloc(size+align);
    uint aligned = 0;
    if (p)
    {
        aligned = (uint(p) + align-1) & ~(align-1);
        if (aligned == p)
            aligned += align;
        *(((uint*)aligned)-1) = aligned-p;
    }
    return (void*)aligned;
}

void Allocator::FreeAligned(void* p)
{
    uint src = uint(p) - *(((uint*)p)-1);
    Free((void*)src);
}


//------------------------------------------------------------------------
// ***** Default Allocator

// This allocator is created and used if no other allocator is installed.
// Default allocator delegates to system malloc.

void* DefaultAllocator::Alloc(uint size)
{
    return malloc(size);
}
void* DefaultAllocator::AllocDebug(uint size, const char* file, unsigned line)
{
#if defined(OVR_CC_MSVC) && defined(_CRTDBG_MAP_ALLOC)
    return _malloc_dbg(size, _NORMAL_BLOCK, file, line);
#else
    OVR_UNUSED2(file, line);
    return malloc(size);
#endif
}

void* DefaultAllocator::Realloc(void* p, uint newSize)
{
    return realloc(p, newSize);
}
void DefaultAllocator::Free(void *p)
{
    return free(p);
}


} // OVR
