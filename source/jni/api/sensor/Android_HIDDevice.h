#pragma once

#include "vglobal.h"

#include "HIDDevice.h"
#include "Android_DeviceManager.h"

#include <jni.h>

NV_NAMESPACE_BEGIN

namespace Android {

class HIDDeviceManager;

enum eDeviceMode
{
	DEVICE_MODE_UNKNOWN,
	DEVICE_MODE_READ,
	DEVICE_MODE_READ_WRITE,
	DEVICE_MODE_WRITE,
	DEVICE_MODE_MAX
};

//-------------------------------------------------------------------------------------
// ***** Android HIDDevice

class HIDDevice : public NervGear::HIDDevice, public DeviceManagerThread::Notifier
{
private:
    friend class HIDDeviceManager;

public:
    HIDDevice(HIDDeviceManager* manager);

    // This is a minimal constructor used during enumeration for us to pass
    // a HIDDevice to the visit function (so that it can query feature reports).
    HIDDevice(HIDDeviceManager* manager, int deviceHandle);

    virtual ~HIDDevice();

    bool HIDInitialize(const VString& path);
    void HIDShutdown();

    virtual bool SetFeatureReport(UByte* data, UInt32 length);
	virtual bool GetFeatureReport(UByte* data, UInt32 length);

    // DeviceManagerThread::Notifier
    void onEvent(int i, int fd);
    double onTicks(double tickSeconds);

    bool OnDeviceNotification(	MessageType messageType,
                            	HIDDeviceDesc* devDesc,
                            	bool* error);

    bool OnDeviceAddedNotification(	const VString& devNodePath,
                         	 	 	HIDDeviceDesc* devDesc,
                         	 	 	bool* error);

private:
    bool initDeviceInfo();
    bool openDevice();
    void closeDevice();
    void closeDeviceOnIOError();
    bool setupDevicePluggedInNotification();

    bool                    InMinimalMode;
    HIDDeviceManager*       HIDManager;

    int                     Device;     // File handle to the device.
    VString                  DevNodePath;
    eDeviceMode             DeviceMode;

    HIDDeviceDesc           DevDesc;

    enum { ReadBufferSize = 96 };
    UByte                   ReadBuffer[ReadBufferSize];

    UInt16                  InputReportBufferLength;
    UInt16                  OutputReportBufferLength;
    UInt16                  FeatureReportBufferLength;
};


//-------------------------------------------------------------------------------------
// ***** Android HIDDeviceManager

class HIDDeviceManager : public NervGear::HIDDeviceManager, public DeviceManagerThread::Notifier
{
	friend class HIDDevice;

public:
    HIDDeviceManager(Android::DeviceManager* manager);
    virtual ~HIDDeviceManager();

    virtual bool Initialize();
    virtual void Shutdown();

    virtual bool Enumerate(HIDEnumerateVisitor* enumVisitor);
    virtual NervGear::HIDDevice* Open(const VString& path);

    // Fills HIDDeviceDesc by using the path.
    // Returns 'true' if successful, 'false' otherwise.
    bool GetHIDDeviceDesc(const VString& path, HIDDeviceDesc* pdevDesc) const;

    static HIDDeviceManager* CreateInternal(DeviceManager* manager);

    // DeviceManagerThread::Notifier - OnTicks is used to initiate poll for new devices.
    void onEvent(int /*i*/, int /*fd*/) {}
    double onTicks(double tickSeconds);

private:
    bool initializeManager();

    bool initVendorProduct(int deviceHandle, HIDDeviceDesc* desc) const;
    bool getFullDesc(int deviceHandle, const VString& devNodePath, HIDDeviceDesc* desc) const;

    bool getStringProperty(const VString& devNodePath, const char* propertyName, VString* pResult) const;
    bool getPath(int deviceHandle, const VString& devNodePath, VString* pPath) const;

    bool AddNotificationDevice(HIDDevice* device);
    bool RemoveNotificationDevice(HIDDevice* device);


    void scanForDevices(bool firstScan = false);
    void getCurrentDevices(Array<VString>* deviceList);
    void removeDevicePath(HIDDevice* device);

    DeviceManager*        	DevManager;

    Array<HIDDevice*>     	NotificationDevices;
    Array<VString>			ScannedDevicePaths;
    double 					TimeToPollForDevicesSeconds;
};

}

NV_NAMESPACE_END
