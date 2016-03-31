/************************************************************************************

Filename    :   OVR_DeviceHandle.cpp
Content     :   Implementation of device handle class
Created     :   February 5, 2013
Authors     :   Lee Cooper

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "DeviceHandle.h"
#include "VThread.h"

#include "DeviceImpl.h"

namespace NervGear {

//-------------------------------------------------------------------------------------
// ***** DeviceHandle

DeviceHandle::DeviceHandle(DeviceCreateDesc* impl) : m_pImpl(impl)
{
    if (m_pImpl)
        m_pImpl->AddRef();
}

DeviceHandle::DeviceHandle(const DeviceHandle& src) : m_pImpl(src.m_pImpl)
{
    if (m_pImpl)
        m_pImpl->AddRef();
}

DeviceHandle::~DeviceHandle()
{
    if (m_pImpl)
        m_pImpl->Release();
}

void DeviceHandle::operator = (const DeviceHandle& src)
{
    if (src.m_pImpl)
        src.m_pImpl->AddRef();
    if (m_pImpl)
        m_pImpl->Release();
    m_pImpl = src.m_pImpl;
}

DeviceBase* DeviceHandle::getDeviceAddRef() const
{
    if (m_pImpl && m_pImpl->pDevice)
    {
        m_pImpl->pDevice->AddRef();
        return m_pImpl->pDevice;
    }
    return NULL;
}

// Returns true, if the handle contains the same device ptr
// as specified in the parameter.
bool DeviceHandle::isDevice(DeviceBase* pdev) const
{
    return (pdev && m_pImpl && m_pImpl->pDevice) ?
        m_pImpl->pDevice == pdev : false;
}

DeviceType  DeviceHandle::type() const
{
    return m_pImpl ? m_pImpl->Type : Device_None;
}

bool DeviceHandle::getDeviceInfo(DeviceInfo* info) const
{
    return m_pImpl ? m_pImpl->GetDeviceInfo(info) : false;
}
bool DeviceHandle::isAvailable() const
{
    // This isn't "atomically safe", but the function only returns the
    // recent state that may change.
    return m_pImpl ? (m_pImpl->Enumerated && m_pImpl->pLock->pManager) : false;
}

bool DeviceHandle::isCreated() const
{
    return m_pImpl ? (m_pImpl->pDevice != 0) : false;
}

DeviceBase* DeviceHandle::createDevice()
{
    if (!m_pImpl)
        return 0;

    DeviceBase*            device = 0;
    Ptr<DeviceManagerImpl> manager= 0;

    // Since both manager and device pointers can only be destroyed during a lock,
    // hold it while checking for availability.
    // AddRef to manager so that it doesn't get released on us.
    {
        VLock::Locker deviceLockScope(m_pImpl->GetLock());

        if (m_pImpl->pDevice)
        {
            m_pImpl->pDevice->AddRef();
            return m_pImpl->pDevice;
        }
        manager = m_pImpl->GetManagerImpl();
    }

    if (manager)
    {
        if (manager->threadId() != NervGear::VThread::currentThreadId())
        {
            // Queue up a CreateDevice request. This fills in '&device' with AddRefed value,
            // or keep it at null.
            manager->threadQueue()->PushCallAndWaitResult(
                manager.GetPtr(), &DeviceManagerImpl::CreateDevice_MgrThread,
                &device, m_pImpl, (DeviceBase*)0);
        }
        else
            device = manager->CreateDevice_MgrThread(m_pImpl, (DeviceBase*)0);
    }
    return device;
}

void DeviceHandle::clear()
{
    if (m_pImpl)
    {
        m_pImpl->Release();
        m_pImpl = 0;
    }
}

bool DeviceHandle::enumerateNext(const DeviceEnumerationArgs& args)
{
    if (type() == Device_None)
        return false;

    Ptr<DeviceManagerImpl> managerKeepAlive;
    VLock::Locker           lockScope(m_pImpl->GetLock());

    DeviceCreateDesc* next = m_pImpl;
    VList<DeviceCreateDesc*>* pointToVList = m_pImpl->pointToVList;
    // If manager was destroyed, we get removed from the list.
    if (m_pImpl->pointToVList->isEmpty())
        return false;

    managerKeepAlive = next->GetManagerImpl();
    vAssert(managerKeepAlive);

    do {
        next = pointToVList->getNextByContent(next);

        if (next == nullptr) {
            m_pImpl->Release();
            m_pImpl = 0;
            return false;
        }

    } while(!args.MatchRule(next->Type, next->Enumerated));

    next->AddRef();
    m_pImpl->Release();
    m_pImpl = next;

    return true;
}

} // namespace NervGear

