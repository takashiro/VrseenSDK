/************************************************************************************

PublicHeader:   OVR.h
Filename    :   OVR_DeviceHandle.h
Content     :   Handle to a device that was enumerated
Created     :   February 5, 2013
Authors     :   Lee Cooper

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#ifndef OVR_DeviceHandle_h
#define OVR_DeviceHandle_h

#include "DeviceConstants.h"

namespace NervGear {

class DeviceBase;
class DeviceInfo;

// Internal
class DeviceCreateDesc;
class DeviceEnumerationArgs;


//-------------------------------------------------------------------------------------
// ***** DeviceHandle

// DeviceHandle references a specific device that was enumerated; it can be assigned
// directly from DeviceEnumerator.
//
// Devices represented by DeviceHandle are not necessarily created or available.
// A device may become unavailable if, for example, it its unplugged. If the device
// is available, it can be created by calling CreateDevice.
//

class DeviceHandle
{
	friend class DeviceManager;
	friend class DeviceManagerImpl;
    template<class B> friend class HIDDeviceImpl;

public:
    DeviceHandle() : m_pImpl(0) { }
	DeviceHandle(const DeviceHandle& src);
	~DeviceHandle();

	void operator = (const DeviceHandle& src);

    bool operator == (const DeviceHandle& other) const { return m_pImpl == other.m_pImpl; }
    bool operator != (const DeviceHandle& other) const { return m_pImpl != other.m_pImpl; }

	// operator bool() returns true if Handle/Enumerator points to a valid device.
    operator bool () const   { return type() != Device_None; }

    // Returns existing device, or NULL if !IsCreated. The returned ptr is
    // addref-ed.
    DeviceBase* getDeviceAddRef() const;
    DeviceType  type() const;
    bool        getDeviceInfo(DeviceInfo* info) const;
    bool        isAvailable() const;
    bool        isCreated() const;
    // Returns true, if the handle contains the same device ptr
    // as specified in the parameter.
    bool        isDevice(DeviceBase*) const;

	// Creates a device, or returns AddRefed pointer if one is already created.
	// New devices start out with RefCount of 1.
    DeviceBase* createDevice();

    // Creates a device, or returns AddRefed pointer if one is already created.
    // New devices start out with RefCount of 1. DeviceT is used to cast the
    // DeviceBase* to a concreete type.
    template <class DeviceT>
    DeviceT* createDeviceTyped() const
    {
        return static_cast<DeviceT*>(DeviceHandle(*this).createDevice());
    }

	// Resets the device handle to uninitialized state.
    void        clear();

protected:
	explicit DeviceHandle(DeviceCreateDesc* impl);
	bool     enumerateNext(const DeviceEnumerationArgs& args);
    DeviceCreateDesc* m_pImpl;
};

} // namespace NervGear

#endif
