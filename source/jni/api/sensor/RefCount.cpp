/************************************************************************************

Filename    :   OVR_RefCount.cpp
Content     :   Reference counting implementation
Created     :   September 19, 2012
Notes       :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#include "RefCount.h"
#include "Log.h"
#include "VLog.h"

namespace NervGear {

#ifdef OVR_CC_ARM
void* ReturnArg0(void* p)
{
    return p;
}
#endif

// ***** Reference Count Base implementation

RefCountImplCore::~RefCountImplCore()
{
    // RefCount can be either 1 or 0 here.
    //  0 if Release() was properly called.
    //  1 if the object was declared on stack or as an aggregate.
    vAssert(m_refCount.load() <= 1);
}

#ifdef OVR_BUILD_DEBUG
void RefCountImplCore::reportInvalidDelete(void *pmem)
{
    OVR_DEBUG_LOG(
        ("Invalid delete call on ref-counted object at %p. Please use Release()", pmem));
    vAssert(0);
}
#endif

RefCountNTSImplCore::~RefCountNTSImplCore()
{
    // RefCount can be either 1 or 0 here.
    //  0 if Release() was properly called.
    //  1 if the object was declared on stack or as an aggregate.
    vAssert(m_refCount.load() <= 1);
}

#ifdef OVR_BUILD_DEBUG
void RefCountNTSImplCore::reportInvalidDelete(void *pmem)
{
    OVR_DEBUG_LOG(
        ("Invalid delete call on ref-counted object at %p. Please use Release()", pmem));
    vAssert(0);
}
#endif


// *** Thread-Safe RefCountImpl

void    RefCountImpl::AddRef()
{
    m_refCount.exchangeAddNoSync(1);
}
void    RefCountImpl::Release()
{
    if ((m_refCount.exchangeAddNoSync(-1) - 1) == 0) {
        delete this;
    }
}

// *** Thread-Safe RefCountVImpl w/virtual AddRef/Release

void    RefCountVImpl::AddRef()
{
    m_refCount.exchangeAddNoSync(1);
}
void    RefCountVImpl::Release()
{
    if ((m_refCount.exchangeAddNoSync(-1) - 1) == 0) {
            delete this;
    }
}

// *** NON-Thread-Safe RefCountImpl

void    RefCountNTSImpl::Release() const
{
    m_refCount--;
    if (m_refCount.load() == 0)
        delete this;
}


} // OVR
