/************************************************************************************

Filename    :   OVR_DeviceImpl.h
Content     :   Partial back-end independent implementation of Device interfaces
Created     :   October 10, 2012
Authors     :   Michael Antonov

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "DeviceImpl.h"
#include "Log.h"

#include "DeviceImpl.h"
#include "SensorDeviceImpl.h"
#include "Profile.h"
#include <new>
namespace NervGear {

//-------------------------------------------------------------------------------------
// ***** MessageHandler

// Threading notes:
// The OnMessage() handler and SetMessageHandler are currently synchronized
// through a separately stored shared Lock object to avoid calling the handler
// from background thread while it's being removed.

static VLock MessageHandlerSharedLock;


class MessageHandlerImpl
{
public:
    MessageHandlerImpl()
        : pLock(&MessageHandlerSharedLock)
    {
    }
    ~MessageHandlerImpl()
    {
        pLock = nullptr;
    }

    static MessageHandlerImpl* FromHandler(MessageHandler* handler)
    { return (MessageHandlerImpl*)&handler->Internal; }
    static const MessageHandlerImpl* FromHandler(const MessageHandler* handler)
    { return (const MessageHandlerImpl*)&handler->Internal; }

    // This lock is held while calling a handler and when we are applied/
    // removed from a device.
    VLock*                     pLock;
    // List of device we are applied to.
    VList<MessageHandlerRef*>   UseList;
};


MessageHandlerRef::MessageHandlerRef(DeviceBase* device)
    : pLock(&MessageHandlerSharedLock), pDevice(device), pHandler(0)
{
}

MessageHandlerRef::~MessageHandlerRef()
{
    {
        VLock::Locker lockScope(pLock);
        if (pHandler)
        {
            pHandler = 0;
            this->pointToVList->remove(this);
        }
    }
    pLock = nullptr;
}

void MessageHandlerRef::SetHandler(MessageHandler* handler)
{
    vAssert(!handler ||
               MessageHandlerImpl::FromHandler(handler)->pLock == pLock);
    VLock::Locker lockScope(pLock);
    SetHandler_NTS(handler);
}

void MessageHandlerRef::SetHandler_NTS(MessageHandler* handler)
{
    if (pHandler != handler)
    {
        if (pHandler) {
            this->pointToVList->remove(this);
        }
        pHandler = handler;

        if (handler)
        {
            MessageHandlerImpl* handlerImpl = MessageHandlerImpl::FromHandler(handler);
            this->pointToVList = &(handlerImpl->UseList);
            handlerImpl->UseList.append(this);
        }
        // TBD: Call notifier on device?
    }
}


MessageHandler::MessageHandler()
{
    static_assert(sizeof(Internal) > sizeof(MessageHandlerImpl), "MessageHandlerImple size is weird");
    ::new((MessageHandlerImpl*)Internal) MessageHandlerImpl;
}

MessageHandler::~MessageHandler()
{
    MessageHandlerImpl* handlerImpl = MessageHandlerImpl::FromHandler(this);
    {
        VLock::Locker lockedScope(handlerImpl->pLock);
        vAssert_LOG(handlerImpl->UseList.isEmpty(),
            ("~MessageHandler %p - Handler still active; call RemoveHandlerFromDevices", this));
    }

    handlerImpl->~MessageHandlerImpl();
}

bool MessageHandler::IsHandlerInstalled() const
{
    const MessageHandlerImpl* handlerImpl = MessageHandlerImpl::FromHandler(this);
    VLock::Locker lockedScope(handlerImpl->pLock);
    return handlerImpl->UseList.isEmpty() != true;
}


void MessageHandler::RemoveHandlerFromDevices()
{
    MessageHandlerImpl* handlerImpl = MessageHandlerImpl::FromHandler(this);
    VLock::Locker lockedScope(handlerImpl->pLock);

    while(!handlerImpl->UseList.isEmpty())
    {
        MessageHandlerRef* use = handlerImpl->UseList.first();
        use->SetHandler_NTS(0);
    }
}

VLock* MessageHandler::GetHandlerLock() const
{
    const MessageHandlerImpl* handlerImpl = MessageHandlerImpl::FromHandler(this);
    return handlerImpl->pLock;
}


//-------------------------------------------------------------------------------------
// ***** DeviceBase


// Delegate relevant implementation to DeviceRectord to avoid re-implementation in
// every derived Device.
void DeviceBase::AddRef()
{
    getDeviceCommon()->DeviceAddRef();
}
void DeviceBase::Release()
{
    getDeviceCommon()->DeviceRelease();
}
DeviceBase* DeviceBase::GetParent() const
{
    return getDeviceCommon()->pParent.GetPtr();
}
DeviceManager* DeviceBase::GetManager() const
{
    return getDeviceCommon()->pCreateDesc->GetManagerImpl();
}

void DeviceBase::SetMessageHandler(MessageHandler* handler)
{
    getDeviceCommon()->HandlerRef.SetHandler(handler);
}
MessageHandler* DeviceBase::GetMessageHandler() const
{
    return getDeviceCommon()->HandlerRef.GetHandler();
}

DeviceType DeviceBase::GetType() const
{
    return getDeviceCommon()->pCreateDesc->Type;
}

bool DeviceBase::getDeviceInfo(DeviceInfo* info) const
{
    return getDeviceCommon()->pCreateDesc->GetDeviceInfo(info);
    //info->Name[0] = 0;
    //return false;
}

// Returns true if device is connected and usable
bool DeviceBase::IsConnected()
{
    return getDeviceCommon()->ConnectedFlag;
}

// returns the MessageHandler's lock
VLock* DeviceBase::GetHandlerLock() const
{
    return getDeviceCommon()->HandlerRef.GetLock();
}

// Derive DeviceManagerCreateDesc to provide abstract function implementation.
class DeviceManagerCreateDesc : public DeviceCreateDesc
{
public:
    DeviceManagerCreateDesc(DeviceFactory* factory)
        : DeviceCreateDesc(factory, Device_Manager) { }

    // We don't need there on Manager since it isn't assigned to DeviceHandle.
    virtual DeviceCreateDesc* Clone() const                        { return 0; }
    virtual MatchResult MatchDevice(const DeviceCreateDesc&,
                                    DeviceCreateDesc**) const      { return Match_None; }
    virtual DeviceBase* NewDeviceInstance()                        { return 0; }
    virtual bool        GetDeviceInfo(DeviceInfo*) const           { return false; }
};

//-------------------------------------------------------------------------------------
// ***** DeviceManagerImpl

DeviceManagerImpl::DeviceManagerImpl()
    : DeviceImpl<NervGear::DeviceManager>(CreateManagerDesc(), 0)
      //,DeviceCreateDescList(pCreateDesc ? pCreateDesc->pLock : 0)
{
    if (pCreateDesc)
    {
        pCreateDesc->pLock->pManager = this;
    }
}

DeviceManagerImpl::~DeviceManagerImpl()
{
    // Shutdown must've been called.
    vAssert(!pCreateDesc->pDevice);

    // Remove all factories
    while(!Factories.isEmpty())
    {
        DeviceFactory* factory = Factories.first();
        factory->RemovedFromManager();
        factory->pointToVList->remove(factory);
    }
}

DeviceCreateDesc* DeviceManagerImpl::CreateManagerDesc()
{
    DeviceCreateDesc* managerDesc = new DeviceManagerCreateDesc(0);
    if (managerDesc)
    {
        managerDesc->pLock = *new DeviceManagerLock;
    }
    return managerDesc;
}

bool DeviceManagerImpl:: initialize(DeviceBase* parent)
{
    NV_UNUSED(parent);
    if (!pCreateDesc || !pCreateDesc->pLock)
		return false;

    pProfileManager = *ProfileManager::Create();

    return true;
}

void DeviceManagerImpl::shutdown()
{
    // Remove all device descriptors from list while the lock is held.
    // Some descriptors may survive longer due to handles.
    while(!Devices.isEmpty())
    {
        DeviceCreateDesc* devDesc = Devices.first();
        vAssert(!devDesc->pDevice); // Manager shouldn't be dying while Device exists.
        devDesc->Enumerated = false;
        devDesc->pointToVList->remove(devDesc);
        if (devDesc->handleCount.load() == 0)
        {
            delete devDesc;
        }
    }
    Devices.clear();

    // These must've been cleared by caller.
    vAssert(pCreateDesc->pDevice == 0);
    vAssert(pCreateDesc->pLock->pManager == 0);

    pProfileManager.Clear();
}


// Callbacks for DeviceCreation/Release
DeviceBase* DeviceManagerImpl::CreateDevice_MgrThread(DeviceCreateDesc* createDesc, DeviceBase* parent)
{
    // Calls to DeviceManagerImpl::CreateDevice are enqueued with wait while holding pManager,
    // so 'this' must remain valid.
    vAssert(createDesc->pLock->pManager);

    VLock::Locker devicesLock(GetLock());

    // If device already exists, just AddRef to it.
    if (createDesc->pDevice)
    {
        createDesc->pDevice->AddRef();
        return createDesc->pDevice;
    }

    if (!parent)
        parent = this;

    DeviceBase* device = createDesc->NewDeviceInstance();

    if (device)
    {
        if (device->getDeviceCommon()-> initialize(parent))
        {
           createDesc->pDevice = device;
        }
        else
        {
            // Don't go through Release() to avoid PushCall behaviour,
            // as it is not needed here.
            delete device;
            device = 0;
        }
    }

    return device;
}

Void DeviceManagerImpl::ReleaseDevice_MgrThread(DeviceBase* device)
{
    // descKeepAlive will keep ManagerLock object alive as well,
    // allowing us to exit gracefully.
    Ptr<DeviceCreateDesc>  descKeepAlive;
    VLock::Locker           devicesLock(GetLock());
    DeviceCommon*          devCommon = device->getDeviceCommon();

    while(1)
    {
        VAtomicInt::Type refCount = devCommon->refCount.load();
        VAtomicInt::Type temp = 1;

        if (refCount > 1)
        {
            if (devCommon->refCount.compare_exchange_weak(refCount, refCount-1))
            {
                // We decreented from initial count higher then 1;
                // nothing else to do.
                return 0;
            }
        }
        else if (devCommon->refCount.compare_exchange_weak(temp, 0))
        {
            // { 1 -> 0 } decrement succeded. Destroy this device.
            break;
        }
    }

    // At this point, may be releasing the device manager itself.
    // This does not matter, however, since shutdown logic is the same
    // in both cases. DeviceManager::Shutdown with begin shutdown process for
    // the internal manager thread, which will eventually destroy itself.
    // TBD: Clean thread shutdown.
    descKeepAlive = devCommon->pCreateDesc;
    descKeepAlive->pDevice = 0;
    devCommon->shutdown();
    delete device;
    return 0;
}



Void DeviceManagerImpl::EnumerateAllFactoryDevices()
{
    // 1. Mark matching devices as NOT enumerated.
    // 2. Call factory to enumerate all HW devices, adding any device that
    //    was not matched.
    // 3. Remove non-matching devices.

    VLock::Locker deviceLock(GetLock());

    // 1.
    for (DeviceCreateDesc* devDesc:Devices) {
        devDesc->Enumerated = false;
    }

    // 2.
    for (DeviceFactory* factory:Factories) {
        EnumerateFactoryDevices(factory);
    }

    // 3.
    for (DeviceCreateDesc* devDesc:Devices) {
        // In case 'devDesc' gets removed.
        // Note, device might be not enumerated since it is opened and
        // in use! Do NOT notify 'device removed' in this case (!AB)
        if (!devDesc->Enumerated)
        {
            // This deletes the devDesc for HandleCount == 0 due to Release in DeviceHandle.
            CallOnDeviceRemoved(devDesc);

            /*
            if (devDesc->HandleCount == 0)
            {
                // Device must be dead if it ever existed, since it AddRefs to us.
                // ~DeviceCreateDesc removes its node from list.
                vAssert(!devDesc->pDevice);
                delete devDesc;
            }
            */
        }
    }

    return 0;
}

Ptr<DeviceCreateDesc> DeviceManagerImpl::AddDevice_NeedsLock(
    const DeviceCreateDesc& createDesc)
{
    // If found, mark as enumerated and we are done.
    DeviceCreateDesc* descCandidate = 0;

    for (DeviceCreateDesc* devDesc:Devices) {
        DeviceCreateDesc::MatchResult mr = devDesc->MatchDevice(createDesc, &descCandidate);
        if (mr == DeviceCreateDesc::Match_Found)
        {
            devDesc->Enumerated = true;
            if (!devDesc->pDevice)
                CallOnDeviceAdded(devDesc);
            return devDesc;
        }
    }

    // Update candidate (this may involve writing fields to HMDDevice createDesc).
    if (descCandidate)
    {
        bool newDevice = false;
        if (descCandidate->UpdateMatchedCandidate(createDesc, &newDevice))
        {
            descCandidate->Enumerated = true;
            if (!descCandidate->pDevice || newDevice)
                CallOnDeviceAdded(descCandidate);
            return descCandidate;
        }
    }

    // If not found, add new device.
    //  - This stores a new descriptor with
    //    {pDevice = 0, HandleCount = 1, Enumerated = true}
    DeviceCreateDesc* desc = createDesc.Clone();
    desc->pLock = pCreateDesc->pLock;
    desc->pointToVList = &Devices;
    Devices.append(desc);
    desc->Enumerated = true;

    CallOnDeviceAdded(desc);

    return desc;
}

Ptr<DeviceCreateDesc> DeviceManagerImpl::FindDevice(
    const VString& path,
    DeviceType deviceType)
{
    VLock::Locker deviceLock(GetLock());
    for (DeviceCreateDesc* devDesc:Devices) {
        if ((deviceType == Device_None || deviceType == devDesc->Type) &&
            devDesc->MatchDeviceByPath(path))
            return devDesc;
    }
    return NULL;
}

Ptr<DeviceCreateDesc> DeviceManagerImpl::FindHIDDevice(const HIDDeviceDesc& hidDevDesc)
{
    VLock::Locker deviceLock(GetLock());
    for (DeviceCreateDesc* devDesc:Devices) {
        if (devDesc->MatchHIDDevice(hidDevDesc))
            return devDesc;
    }
    return NULL;
}

void DeviceManagerImpl::DetectHIDDevice(const HIDDeviceDesc& hidDevDesc)
{
    VLock::Locker deviceLock(GetLock());
    for (DeviceFactory* factory:Factories) {
        if (factory->DetectHIDDevice(this, hidDevDesc))
            break;
    }

}

// Enumerates devices for a particular factory.
Void DeviceManagerImpl::EnumerateFactoryDevices(DeviceFactory* factory)
{

    class FactoryEnumerateVisitor : public DeviceFactory::EnumerateVisitor
    {
        DeviceManagerImpl* pManager;
    public:
        FactoryEnumerateVisitor(DeviceManagerImpl* manager)
            : pManager(manager) { }

        virtual void Visit(const DeviceCreateDesc& createDesc)
        {
            pManager->AddDevice_NeedsLock(createDesc);
        }
    };

    FactoryEnumerateVisitor newDeviceVisitor(this);
    factory->EnumerateDevices(newDeviceVisitor);


    return 0;
}


DeviceEnumerator<> DeviceManagerImpl::enumerateDevicesEx(const DeviceEnumerationArgs& args)
{
    VLock::Locker deviceLock(GetLock());

    if (Devices.isEmpty())
        return DeviceEnumerator<>();

    DeviceCreateDesc*  firstDeviceDesc = Devices.first();
    DeviceEnumerator<> e = enumeratorFromHandle(DeviceHandle(firstDeviceDesc), args);

    if (!args.MatchRule(firstDeviceDesc->Type, firstDeviceDesc->Enumerated))
    {
        e.Next();
    }

    return e;
}

//-------------------------------------------------------------------------------------
// ***** DeviceCommon

void DeviceCommon::DeviceAddRef()
{
    refCount++;
}

void DeviceCommon::DeviceRelease()
{
    while(1)
    {
        VAtomicInt::Type refCount = this->refCount.load();
        vAssert(refCount > 0);

        if (refCount == 1)
        {
            DeviceManagerImpl*  manager = pCreateDesc->GetManagerImpl();
            ThreadCommandQueue* queue   = manager->threadQueue();

            // Enqueue ReleaseDevice for {1 -> 0} transition with no wait.
            // We pass our reference ownership into the queue to destroy.
            // It's in theory possible for another thread to re-steal our device reference,
            // but that is checked for atomically in DeviceManagerImpl::ReleaseDevice.
            if (!queue->PushCall(manager, &DeviceManagerImpl::ReleaseDevice_MgrThread,
                                          pCreateDesc->pDevice))
            {
                // PushCall shouldn't fail because background thread runs while manager is
                // alive and we are holding Manager alive through pParent chain.
                vAssert(false);
            }

            // Warning! At his point everything, including manager, may be dead.
            break;
        }
        else if (this->refCount.compare_exchange_weak(refCount, refCount-1))
        {
            break;
        }
    }
}



//-------------------------------------------------------------------------------------
// ***** DeviceCreateDesc


void DeviceCreateDesc::AddRef()
{
    // Technically, HandleCount { 0 -> 1 } transition can only happen during Lock,
    // but we leave this to caller to worry about (happens during enumeration).
    handleCount++;
}

void DeviceCreateDesc::Release()
{
    while(1)
    {
        VAtomicInt::Type handleCount = this->handleCount;
        // HandleCount must obviously be >= 1, since we are releasing it.
        vAssert(handleCount > 0);

        // {1 -> 0} transition may cause us to be destroyed, so require a lock.
        if (handleCount == 1)
        {
            Ptr<DeviceManagerLock>  lockKeepAlive;
            VLock::Locker            deviceLockScope(GetLock());

            if (!this->handleCount.compare_exchange_weak(handleCount, 0))
                continue;

            vAssert(pDevice == 0);

            // Destroy *this if the manager was destroyed already, or Enumerated
            // is false (device no longer available).
            if (!GetManagerImpl() || !Enumerated)
            {
                lockKeepAlive = pLock;

                // Remove from manager list (only matters for !Enumerated).
                delete this;
            }

            // Available DeviceCreateDesc may survive with { HandleCount == 0 },
            // in case it might be enumerated again later.
            break;
        }
        else if (this->handleCount.compare_exchange_weak(handleCount, handleCount-1))
        {
            break;
        }
    }
}

HMDDevice* HMDDevice::Disconnect(SensorDevice* psensor)
{
    if (!psensor)
        return NULL;

    NervGear::DeviceManager* manager = GetManager();
    if (manager)
    {
        //DeviceManagerImpl* mgrImpl = static_cast<DeviceManagerImpl*>(manager);
        Ptr<DeviceCreateDesc> desc = getDeviceCommon()->pCreateDesc;
        if (desc)
        {
            class Visitor : public DeviceFactory::EnumerateVisitor
            {
                Ptr<DeviceCreateDesc> Desc;
            public:
                Visitor(DeviceCreateDesc* desc) : Desc(desc) {}
                virtual void Visit(const DeviceCreateDesc& createDesc)
                {
                    VLock::Locker lock(Desc->GetLock());
                    Desc->UpdateMatchedCandidate(createDesc);
                }
            } visitor(desc);
            //SensorDeviceImpl* sImpl = static_cast<SensorDeviceImpl*>(psensor);

            SensorDisplayInfoImpl displayInfo;

            if (psensor->GetFeatureReport(displayInfo.Buffer, SensorDisplayInfoImpl::PacketSize))
            {
                displayInfo.Unpack();

                // If we got display info, try to match / create HMDDevice as well
                // so that sensor settings give preference.
                if (displayInfo.DistortionType & SensorDisplayInfoImpl::Mask_BaseFmt)
                {
                    SensorDeviceImpl::EnumerateHMDFromSensorDisplayInfo(displayInfo, visitor);
                }
            }
        }
    }
    return this;
}

bool  HMDDevice::IsDisconnected() const
{
    NervGear::HMDInfo info;
    getDeviceInfo(&info);
    // if strlen(info.DisplayDeviceName) == 0 then
    // this HMD is 'fake' (created using sensor).
    return (strlen(info.DisplayDeviceName) == 0);
}


} // namespace NervGear
