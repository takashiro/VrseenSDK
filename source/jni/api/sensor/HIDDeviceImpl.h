#pragma once

#include "vglobal.h"

//#include "Device.h"
#include "DeviceImpl.h"

NV_NAMESPACE_BEGIN

//-------------------------------------------------------------------------------------
class HIDDeviceCreateDesc : public DeviceCreateDesc
{
public:
    HIDDeviceCreateDesc(DeviceFactory* factory, DeviceType type, const HIDDeviceDesc& hidDesc)
        : DeviceCreateDesc(factory, type), HIDDesc(hidDesc) { }
    HIDDeviceCreateDesc(const HIDDeviceCreateDesc& other)
        : DeviceCreateDesc(other.pFactory, other.Type), HIDDesc(other.HIDDesc) { }

    virtual bool MatchDeviceByPath(const VString& path)
    {
        // should it be case insensitive?
        return HIDDesc.Path.CompareNoCase(path) == 0;
    }

    HIDDeviceDesc HIDDesc;
};

//-------------------------------------------------------------------------------------
template<class B>
class HIDDeviceImpl : public DeviceImpl<B>, public HIDDevice::HIDHandler
{
public:
    HIDDeviceImpl(HIDDeviceCreateDesc* createDesc, DeviceBase* parent)
     :  DeviceImpl<B>(createDesc, parent)
    {
    }

    // HIDDevice::Handler interface.
    virtual void OnDeviceMessage(HIDDeviceMessageType messageType)
    {
        MessageType handlerMessageType;
        switch (messageType) {
            case HIDDeviceMessage_DeviceAdded:
                handlerMessageType           = Message_DeviceAdded;
                DeviceImpl<B>::ConnectedFlag = true;
                break;

            case HIDDeviceMessage_DeviceRemoved:
                handlerMessageType           = Message_DeviceRemoved;
                DeviceImpl<B>::ConnectedFlag = false;
                break;

            default: OVR_ASSERT(0); return;
        }

        // Do device notification.
        {
            Lock::Locker scopeLock(this->HandlerRef.GetLock());

            if (this->HandlerRef.GetHandler())
            {
                MessageDeviceStatus status(handlerMessageType, this, DeviceHandle(this->pCreateDesc));
                this->HandlerRef.GetHandler()->onMessage(status);
            }
        }

        // Do device manager notification.
        DeviceManagerImpl*   manager = this->GetManagerImpl();
        switch (handlerMessageType) {
            case Message_DeviceAdded:
                manager->CallOnDeviceAdded(this->pCreateDesc);
                break;

            case Message_DeviceRemoved:
                manager->CallOnDeviceRemoved(this->pCreateDesc);
                break;

            default:;
        }
    }

    virtual bool  initialize(DeviceBase* parent)
    {
        // Open HID device.
        HIDDeviceDesc&		hidDesc = *getHIDDesc();
        HIDDeviceManager*   pManager = GetHIDDeviceManager();


        HIDDevice* device = pManager->Open(hidDesc.Path);
        if (!device)
        {
            return false;
        }

        InternalDevice = *device;
        InternalDevice->SetHandler(this);

        // AddRef() to parent, forcing chain to stay alive.
        DeviceImpl<B>::pParent = parent;

        return true;
    }

    virtual void shutdown()
    {
        InternalDevice->SetHandler(NULL);

        // Remove the handler, if any.
        this->HandlerRef.SetHandler(0);

        DeviceImpl<B>::pParent.Clear();
    }

    DeviceManager* GetDeviceManager()
    {
        return DeviceImpl<B>::pCreateDesc->GetManagerImpl();
    }

    HIDDeviceManager* GetHIDDeviceManager()
    {
        return DeviceImpl<B>::pCreateDesc->GetManagerImpl()->GetHIDDeviceManager();
    }


    struct WriteData
    {
        enum { BufferSize = 64 };
        UByte Buffer[64];
        uint Size;

        WriteData(UByte* data, uint size) : Size(size)
        {
            OVR_ASSERT(size <= BufferSize);
            memcpy(Buffer, data, size);
        }
    };

    bool SetFeatureReport(UByte* data, UInt32 length)
    {
        WriteData writeData(data, length);

        // Push call with wait.
        bool result = false;

		ThreadCommandQueue* pQueue = this->GetManagerImpl()->threadQueue();
        if (!pQueue->PushCallAndWaitResult(this, &HIDDeviceImpl::setFeatureReport, &result, writeData))
            return false;

        return result;
    }

    bool setFeatureReport(const WriteData& data)
    {
        return InternalDevice->SetFeatureReport((UByte*) data.Buffer, (UInt32) data.Size);
    }

    bool GetFeatureReport(UByte* data, UInt32 length)
    {
        bool result = false;

		ThreadCommandQueue* pQueue = this->GetManagerImpl()->threadQueue();
        if (!pQueue->PushCallAndWaitResult(this, &HIDDeviceImpl::getFeatureReport, &result, data, length))
            return false;

        return result;
    }

    bool getFeatureReport(UByte* data, UInt32 length)
    {
        return InternalDevice->GetFeatureReport(data, length);
    }

protected:
    HIDDevice* GetInternalDevice() const
    {
        return InternalDevice;
    }

    HIDDeviceDesc* getHIDDesc() const
    { return &getCreateDesc()->HIDDesc; }

    HIDDeviceCreateDesc* getCreateDesc() const
    { return (HIDDeviceCreateDesc*) &(*DeviceImpl<B>::pCreateDesc); }

private:
    Ptr<HIDDevice> InternalDevice;
};

NV_NAMESPACE_END
