/************************************************************************************
Filename    :   OVR_Android_HIDDevice.h
Content     :   Android HID device implementation.
Created     :   July 10, 2013
Authors     :   Brant Lewis

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "Android_HIDDevice.h"

#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include "HIDDeviceImpl.h"
#include <dirent.h>
#include <string.h>
#include <linux/types.h>
#include <sys/stat.h>

#include <jni.h>

// Definitions from linux/hidraw.h

#define HID_MAX_DESCRIPTOR_SIZE		4096

struct hidraw_report_descriptor {
    __u32 size;
    __u8 value[HID_MAX_DESCRIPTOR_SIZE];
};

struct hidraw_devinfo {
    __u32 bustype;
    __s16 vendor;
    __s16 product;
};

// ioctl interface
#define HIDIOCGRDESCSIZE	_IOR('H', 0x01, int)
#define HIDIOCGRDESC		_IOR('H', 0x02, struct hidraw_report_descriptor)
#define HIDIOCGRAWINFO		_IOR('H', 0x03, struct hidraw_devinfo)
#define HIDIOCGRAWNAME(len)     _IOC(_IOC_READ, 'H', 0x04, len)
#define HIDIOCGRAWPHYS(len)     _IOC(_IOC_READ, 'H', 0x05, len)
// The first byte of SFEATURE and GFEATURE is the report number
#define HIDIOCSFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
#define HIDIOCGFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)

#define HIDRAW_FIRST_MINOR 0
#define HIDRAW_MAX_DEVICES 64
// number of reports to buffer
#define HIDRAW_BUFFER_SIZE 64

#define OVR_DEVICE_NAMES	"ovr"

namespace NervGear { namespace Android {

static const char * deviceModeNames[DEVICE_MODE_MAX] =
{
	"UNKNOWN", "READ", "READ_WRITE", "WRITE"
};


//-------------------------------------------------------------------------------------
// **** Android::DeviceManager
//-----------------------------------------------------------------------------
HIDDeviceManager::HIDDeviceManager(DeviceManager* manager) : DevManager(manager)
{
	TimeToPollForDevicesSeconds = 0.0;
}

//-----------------------------------------------------------------------------
HIDDeviceManager::~HIDDeviceManager()
{
}

//-----------------------------------------------------------------------------
bool HIDDeviceManager::initializeManager()
{
    return true;
}

//-----------------------------------------------------------------------------
bool HIDDeviceManager::Initialize()
{
    DevManager->pThread->addTicksNotifier(this);
	scanForDevices(true);
	return true;
}

//-----------------------------------------------------------------------------
void HIDDeviceManager::Shutdown()
{
    LogText("NervGear::Android::HIDDeviceManager - shutting down.\n");
}

//-------------------------------------------------------------------------------
bool HIDDeviceManager::AddNotificationDevice(HIDDevice* device)
{
    NotificationDevices.append(device);
    return true;
}

//-------------------------------------------------------------------------------
bool HIDDeviceManager::RemoveNotificationDevice(HIDDevice* device)
{
    for (uint i = 0; i < NotificationDevices.size(); i++)
    {
        if (NotificationDevices[i] == device)
        {
            NotificationDevices.removeAt(i);
            return true;
        }
    }
    return false;
}

//-------------------------------------------------------------------------------
bool HIDDeviceManager::initVendorProduct(int deviceHandle, HIDDeviceDesc* desc) const
{
    // Get Raw Info
    hidraw_devinfo info;
    memset(&info, 0, sizeof(info));

    int r = ioctl(deviceHandle, HIDIOCGRAWINFO, &info);

    if (r < 0)
    {
        return false;
    }

    desc->VendorId = info.vendor;
    desc->ProductId = info.product;

    return true;
}

//-----------------------------------------------------------------------------
bool HIDDeviceManager::getStringProperty(const VString &devNodePath, const char* propertyName, NervGear::VString* pResult) const
{
	// LDC - In order to get device data on Android we walk the sysfs folder hierarchy. Normally this would be handled by
	// libudev but that doesn't seem available through the ndk. For the device '/dev/ovr0' we find the physical
	// path of the symbolic link '/sys/class/ovr/ovr0'. Then we walk up the folder hierarchy checking to see if
	// the property 'file' is present in that folder e.g. '/sys/devices/platform/xhci-hcd/usb1/1-1/manufacturer'.
	// When we find it we read and return the reported string. We have to walk the path backwards to avoid returning
	// properties of any usb hubs that are in the usb graph.

	// Create the command string using the last character of the device path.
    VString cmdStr;
    cmdStr.sprintf("cd -P /sys/class/ovr/ovr%c; pwd", devNodePath.at(devNodePath.length() - 1));

	// Execute the command and get stdout.
    FILE* cmdFile = popen(cmdStr.toCString(), "r");

	if (cmdFile)
	{
		char stdoutBuf[1024];
		char *pSysPath = fgets(stdoutBuf, sizeof(stdoutBuf), cmdFile);
		pclose(cmdFile);

		if (pSysPath == NULL)
		{
			return false;
		}

		// Now walk the path back until we find a folder with a file that has the same name as the propertyName.
		while(true)
		{
			DIR* dir = opendir(pSysPath);
			if (dir)
			{
				dirent* entry = readdir(dir);
				while (entry)
				{
			    	if (strcmp(entry->d_name, propertyName) == 0)
			    	{
			    		// We've found the file.
				    	closedir(dir);

                        VString propertyPath;
                        propertyPath.sprintf("%s/%s", pSysPath, propertyName);

                        FILE* propertyFile = fopen(propertyPath.toCString(), "r");

						char propertyContents[2048];
				    	char* result = fgets(propertyContents, sizeof(propertyContents), propertyFile);
						fclose(propertyFile);

						if (result == NULL)
						{
							return false;
						}

						// Remove line feed character.
						propertyContents[strlen(propertyContents)-1] = '\0';

						*pResult = propertyContents;

				    	return true;
			    	}

					entry = readdir(dir);
				}

				closedir(dir);
			}

			// Determine the parent folder path.
			while (pSysPath[strlen(pSysPath)-1] != '/')
			{
				pSysPath[strlen(pSysPath)-1] = '\0';

				if (strlen(pSysPath) == 0)
				{
					return false;
				}
			}

			// Remove the '/' from the end.
			pSysPath[strlen(pSysPath)-1] = '\0';
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
bool HIDDeviceManager::Enumerate(HIDEnumerateVisitor* enumVisitor)
{

    // Scan the /dev directory looking for devices
    DIR* dir = opendir("/dev");
    if (dir)
    {
        dirent* entry = readdir(dir);
        while (entry)
        {
            if (strstr(entry->d_name, OVR_DEVICE_NAMES))
            {   // Open the device to check if this is an Oculus device.

                VString devicePath;
                devicePath.sprintf("/dev/%s", entry->d_name);


                // Try to open for both read and write initially if we can.
                int device = open(devicePath.toCString(), O_RDWR);
                if (device < 0)
                {
                    device = open(devicePath.toCString(), O_RDONLY);
                }

                if (device >= 0)
                {
                    HIDDeviceDesc devDesc;

                    if (initVendorProduct(device, &devDesc) &&
                        enumVisitor->MatchVendorProduct(devDesc.VendorId, devDesc.ProductId))
                    {
                        getFullDesc(device, devicePath, &devDesc);

                        // Look for the device to check if it is already opened.
                        Ptr<DeviceCreateDesc> existingDevice = DevManager->FindHIDDevice(devDesc);
                        // if device exists and it is opened then most likely the device open()
                        // will fail; therefore, we just set Enumerated to 'true' and continue.
                        if (existingDevice && existingDevice->pDevice)
                        {
                            existingDevice->Enumerated = true;
                        }
                        else
                        {
                        	// Construct minimal device that the visitor callback can get feature reports from.
                            Android::HIDDevice hidDevice(this, device);
                            enumVisitor->Visit(hidDevice, devDesc);
                        }
                    }

                    close(device);
                }
                else
                {
                    LogText("Failed to open device %s with error %d\n", devicePath.toCString(), errno);
                }
            }
            entry = readdir(dir);
        }

        closedir(dir);
    }

    return true;
}

//-----------------------------------------------------------------------------
bool HIDDeviceManager::getPath(int deviceHandle, const VString& devNodePath, VString* pPath) const
{

	HIDDeviceDesc desc;

    if (!initVendorProduct(deviceHandle, &desc))
    {
        return false;
    }

    getStringProperty(devNodePath, "serial", &(desc.SerialNumber));

    // Compose the path.
    pPath->clear();
    pPath->sprintf("vid=%04hx:pid=%04hx:ser=%s",
							desc.VendorId,
							desc.ProductId,
							desc.SerialNumber.toCString());

    return true;
}

//-----------------------------------------------------------------------------
bool HIDDeviceManager::getFullDesc(int deviceHandle, const VString& devNodePath, HIDDeviceDesc* desc) const
{

    if (!initVendorProduct(deviceHandle, desc))
    {
        return false;
    }

    VString versionStr;
    getStringProperty(devNodePath, "bcdDevice", &versionStr);
    SInt32 versionNum;
    sscanf(versionStr.toCString(), "%x", &versionNum);
    desc->VersionNumber = versionNum;

    getStringProperty(devNodePath, "manufacturer", &(desc->Manufacturer));
    getStringProperty(devNodePath, "product", &(desc->Product));
    getStringProperty(devNodePath, "serial", &(desc->SerialNumber));

    // Compose the path.
    getPath(deviceHandle, devNodePath, &(desc->Path));

    return true;
}

//-----------------------------------------------------------------------------
bool HIDDeviceManager::GetHIDDeviceDesc(const VString& devNodePath, HIDDeviceDesc* pdevDesc) const
{

    int deviceHandle = open(devNodePath.toCString(), O_RDONLY);

    if (deviceHandle < 0)
    {
        return false;
    }

    bool result = getFullDesc(deviceHandle, devNodePath, pdevDesc);

    close(deviceHandle);
    return result;
}

//-----------------------------------------------------------------------------
NervGear::HIDDevice* HIDDeviceManager::Open(const VString& path)
{

    Ptr<Android::HIDDevice> device = *new Android::HIDDevice(this);

    if (!device->HIDInitialize(path))
    {
    	return NULL;
    }

    device->AddRef();

    return device;
}

//=============================================================================
//                           Android::HIDDevice
//=============================================================================

HIDDevice::HIDDevice(HIDDeviceManager* manager) :
	InMinimalMode(false),
	HIDManager(manager),
	Device( -1 ),
	DeviceMode( DEVICE_MODE_UNKNOWN )
{
}

//-----------------------------------------------------------------------------
// This is a minimal constructor used during enumeration for us to pass
// a HIDDevice to the visit function (so that it can query feature reports).
HIDDevice::HIDDevice(HIDDeviceManager* manager, int deviceHandle) :
	InMinimalMode(true),
	HIDManager(manager),
	Device(deviceHandle),
	DeviceMode( DEVICE_MODE_UNKNOWN )
{
}

//-----------------------------------------------------------------------------
HIDDevice::~HIDDevice()
{
    if (!InMinimalMode)
    {
        HIDShutdown();
    }
}

//-----------------------------------------------------------------------------
bool HIDDevice::HIDInitialize(const VString& path)
{

	DevDesc.Path = path;

	if (!openDevice())
    {
        LogText("NervGear::Android::HIDDevice - Failed to open HIDDevice: %s", path.toCString());
        return false;
    }

    if (!initDeviceInfo())
    {
        LogText("NervGear::Android::HIDDevice - Failed to get device info for HIDDevice: %s", path.toCString());
        closeDevice();
        return false;
    }


    HIDManager->DevManager->pThread->addTicksNotifier(this);
    HIDManager->AddNotificationDevice(this);

    LogText("NervGear::Android::HIDDevice - Opened:'%s'  Manufacturer:'%s'  Product:'%s'  Serial#:'%s'  Version:'%04x'\n",
    		DevDesc.Path.toCString(),
            DevDesc.Manufacturer.toCString(),
            DevDesc.Product.toCString(),
            DevDesc.SerialNumber.toCString(),
            DevDesc.VersionNumber);

    return true;
}

//-----------------------------------------------------------------------------
bool HIDDevice::initDeviceInfo()
{
    // Device must have been successfully opened.
	OVR_ASSERT(Device >= 0);

#if 0
    int desc_size = 0;
    hidraw_report_descriptor rpt_desc;
    memset(&rpt_desc, 0, sizeof(rpt_desc));

    // get report descriptor size
    int r = ioctl(Device, HIDIOCGRDESCSIZE, &desc_size);
    if (r < 0)
    {
        OVR_ASSERT_LOG(false, ("Failed to get report descriptor size."));
        return false;
    }

    // Get the report descriptor
    rpt_desc.size = desc_size;
    r = ioctl(Device, HIDIOCGRDESC, &rpt_desc);
    if (r < 0)
    {
        OVR_ASSERT_LOG(false, ("Failed to get report descriptor."));
        return false;
    }

    // TODO: Parse the report descriptor and read out the report lengths etc.
#endif

    // Hard code report lengths for now.
    InputReportBufferLength = 62;
    OutputReportBufferLength = 0;
    FeatureReportBufferLength = 69;

    if (ReadBufferSize < InputReportBufferLength)
    {
        OVR_ASSERT_LOG(false, ("Input report buffer length is bigger than read buffer."));
        return false;
    }


	// Get device desc.
    if (!HIDManager->getFullDesc(Device, DevNodePath, &DevDesc))
    {
        OVR_ASSERT_LOG(false, ("Failed to get device desc while initializing device."));
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
bool HIDDevice::openDevice()
{

	OVR_ASSERT(Device == -1);

	OVR_DEBUG_LOG(("HIDDevice::openDevice %s", DevDesc.Path.toCString()));

	// Have to iterate through devices to find the one with this path.
	// Scan the /dev directory looking for devices
	DIR* dir = opendir("/dev");
	if (dir)
	{
		dirent* entry = readdir(dir);
		while (entry)
		{
			if (strstr(entry->d_name, OVR_DEVICE_NAMES))
			{
				// Open the device to check if this is an Oculus device.
                VString devicePath;
                devicePath.sprintf("/dev/%s", entry->d_name);

				// Try to open for both read and write if we can.
                int device = open(devicePath.toCString(), O_RDWR);
				DeviceMode = DEVICE_MODE_READ_WRITE;
				if (device < 0)
				{
                    device = open(devicePath.toCString(), O_RDONLY);
					DeviceMode = DEVICE_MODE_READ;
				}

				if (device >= 0)
				{
					VString path;
                    if (!HIDManager->getPath(device, devicePath.toCString(), &path) ||
						path != DevDesc.Path)
					{
						close(device);
						continue;
					}

					Device = device;
                    DevNodePath = devicePath;
					LogText("NervGear::Android::HIDDevice - device mode %s", deviceModeNames[DeviceMode] );
					break;
				}
			}
			entry = readdir(dir);
		}
		closedir(dir);
	}

    if (Device < 0)
    {
        OVR_DEBUG_LOG(("Failed to open device %s, error = 0x%X.", DevDesc.Path.toCString(), errno));
        Device = -1;
        return false;
    }

    // Add the device to the polling list.
    if (!HIDManager->DevManager->pThread->addSelectFd(this, Device))
    {
        OVR_ASSERT_LOG(false, ("Failed to initialize polling for HIDDevice."));

        close(Device);
        Device = -1;
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
void HIDDevice::HIDShutdown()
{

    HIDManager->DevManager->pThread->removeTicksNotifier(this);
    HIDManager->RemoveNotificationDevice(this);

    if (Device >= 0) // Device may already have been closed if unplugged.
    {
        closeDevice();
    }

    LogText("NervGear::Android::HIDDevice - HIDShutdown '%s'\n", DevDesc.Path.toCString());
}

//-----------------------------------------------------------------------------
void HIDDevice::closeDevice()
{
    OVR_ASSERT(Device >= 0);

    HIDManager->DevManager->pThread->removeSelectFd(this, Device);

    close(Device);  // close the file handle
    Device = -1;
    DeviceMode = DEVICE_MODE_UNKNOWN;
    LogText("NervGear::Android::HIDDevice - HID Device Closed '%s'\n", DevDesc.Path.toCString());
}

//-----------------------------------------------------------------------------
void HIDDevice::closeDeviceOnIOError()
{
    LogText("NervGear::Android::HIDDevice - Lost connection to '%s'\n", DevDesc.Path.toCString());
    closeDevice();
}

//-----------------------------------------------------------------------------
bool HIDDevice::SetFeatureReport(UByte* data, UInt32 length)
{
    if (Device < 0)
        return false;

    UByte reportID = data[0];

    if (reportID == 0)
    {
        // Not using reports so remove from data packet.
        data++;
        length--;
    }

    int r = ioctl(Device, HIDIOCSFEATURE(length), data);
	return (r >= 0);
}

//-----------------------------------------------------------------------------
bool HIDDevice::GetFeatureReport(UByte* data, UInt32 length)
{
    if (Device < 0)
        return false;

	int r = ioctl(Device, HIDIOCGFEATURE(length), data);
    return (r >= 0);
}

//-----------------------------------------------------------------------------
double HIDDevice::onTicks(double tickSeconds)
{
    if (Handler)
    {
        return Handler->OnTicks(tickSeconds);
    }

    return DeviceManagerThread::Notifier::onTicks(tickSeconds);
}

//-----------------------------------------------------------------------------
void HIDDevice::onEvent(int i, int fd)
{
    // We have data to read from the device
    int bytes = read(fd, ReadBuffer, ReadBufferSize);

	if (bytes < 0)
	{
		LogText( "NervGear::Android::HIDDevice - ReadError: fd %d, ReadBufferSize %d, BytesRead %d, errno %d\n",
			fd, ReadBufferSize, bytes, errno );

		if ( errno == EAGAIN )
		{
			LogText( "NervGear::Android::HIDDevice - EAGAIN, device is %s.", deviceModeNames[DeviceMode] );
		}

		// Close the device on read error.
		closeDeviceOnIOError();

		// Generate a device removed event.
		bool error;
		OnDeviceNotification(Message_DeviceRemoved, &DevDesc, &error);

		HIDManager->removeDevicePath( this );

		return;
	}


	// TODO: I need to handle partial messages and package reconstruction
	if (Handler)
	{
		Handler->OnInputReport(ReadBuffer, bytes);
	}
}

//-----------------------------------------------------------------------------
bool HIDDevice::OnDeviceAddedNotification(	const VString& devNodePath,
                                     	 	HIDDeviceDesc* devDesc,
                                     	 	bool* error)
{

	VString devicePath = devDesc->Path;

    // Is this the correct device?
    if (DevDesc.Path.icompare(devicePath) != 0)
    {
        return false;
    }

    if (Device == -1)
    {
    	// A closed device has been re-added. Try to reopen.

    	DevNodePath = devNodePath;

        if (!openDevice())
        {
            LogError("NervGear::Android::HIDDevice - Failed to reopen a device '%s' that was re-added.\n", devicePath.toCString());
			*error = true;
            return true;
        }

        LogText("NervGear::Android::HIDDevice - Reopened device '%s'\n", devicePath.toCString());
    }

    *error = false;
    return true;
}

//-----------------------------------------------------------------------------
bool HIDDevice::OnDeviceNotification(MessageType messageType,
                                     HIDDeviceDesc* devDesc,
                                     bool* error)
{

	VString devicePath = devDesc->Path;

    // Is this the correct device?
    if (DevDesc.Path.icompare(devicePath) != 0)
    {
        return false;
    }

    if (messageType == Message_DeviceRemoved)
    {
        if (Device >= 0)
        {
            closeDevice();
        }

        if (Handler)
        {
            Handler->OnDeviceMessage(HIDHandler::HIDDeviceMessage_DeviceRemoved);
        }
    }
    else
    {
        OVR_ASSERT(0);
    }

    *error = false;
    return true;
}

//-----------------------------------------------------------------------------
HIDDeviceManager* HIDDeviceManager::CreateInternal(Android::DeviceManager* devManager)
{

    if (!System::IsInitialized())
    {
        // Use custom message, since Log is not yet installed.
        OVR_DEBUG_STATEMENT(Log::GetDefaultLog()->
                            LogMessage(Log_Debug, "HIDDeviceManager::Create failed - NervGear::System not initialized"); );
        return 0;
    }

    Ptr<Android::HIDDeviceManager> manager = *new Android::HIDDeviceManager(devManager);

    if (manager)
    {
        if (manager->Initialize())
        {
            manager->AddRef();
        }
        else
        {
            manager.Clear();
        }
    }

    return manager.GetPtr();
}

//-----------------------------------------------------------------------------
void HIDDeviceManager::getCurrentDevices(VArray<VString>* deviceList)
{
	deviceList->clear();

	DIR* dir = opendir("/dev");
	if (dir)
	{
		dirent* entry = readdir(dir);
		while (entry)
		{
			if (strstr(entry->d_name, OVR_DEVICE_NAMES))
			{
                VString dev_path;
                dev_path.sprintf("/dev/%s", entry->d_name);

                deviceList->append(dev_path);
			}

			entry = readdir(dir);
		}
	}

	closedir(dir);
}

//-----------------------------------------------------------------------------
void HIDDeviceManager::scanForDevices(bool firstScan)
{

	// Create current device list.
	VArray<VString> currentDeviceList;
	getCurrentDevices(&currentDeviceList);

	if (firstScan)
	{
		ScannedDevicePaths = currentDeviceList;
		return;
	}

	// Check for new devices.
    for (VArray<VString>::iterator itCurrent = currentDeviceList.begin();
    			itCurrent != currentDeviceList.end(); ++itCurrent)
	{
    	VString devNodePath = *itCurrent;
    	bool found = false;

    	// Was it in the previous scan?
    	for (VArray<VString>::iterator itScanned = ScannedDevicePaths.begin();
        		itScanned != ScannedDevicePaths.end(); ++itScanned)
    	{
        	if (devNodePath.icompare(*itScanned) == 0)
        	{
        		found = true;
        		break;
        	}
    	}

    	if (found)
    	{
    		continue;
    	}

    	// Get device desc.
        HIDDeviceDesc devDesc;
		if (!GetHIDDeviceDesc(devNodePath, &devDesc))
        {
        	continue;
        }


    	bool error = false;
        bool deviceFound = false;
    	for (uint i = 0; i < NotificationDevices.size(); i++)
        {
    		if (NotificationDevices[i] &&
    			NotificationDevices[i]->OnDeviceAddedNotification(devNodePath, &devDesc, &error))
    		{
    			// The device handled the notification so we're done.
                deviceFound = true;
    			break;
    		}
        }

		if (deviceFound)
		{
			continue;
		}


		// A new device was connected. Go through all device factories and
		// try to detect the device using HIDDeviceDesc.
		Lock::Locker deviceLock(DevManager->GetLock());
		for (DeviceFactory* factory:DevManager->Factories) {
			if (factory->DetectHIDDevice(DevManager, devDesc))
			{
				break;
			}
		}
	}

    ScannedDevicePaths = currentDeviceList;
}

//-----------------------------------------------------------------------------
void HIDDeviceManager::removeDevicePath(HIDDevice* device)
{
	for ( int i = 0; i < ScannedDevicePaths.length(); i++ )
	{
		if ( ScannedDevicePaths[i] == device->DevNodePath )
		{
			ScannedDevicePaths.removeAt( i );
			return;
		}
	}
}

//-----------------------------------------------------------------------------
double HIDDeviceManager::onTicks(double tickSeconds)
{
	if (tickSeconds >= TimeToPollForDevicesSeconds)
	{
		TimeToPollForDevicesSeconds = tickSeconds + 1.0;
		scanForDevices();
	}

    return TimeToPollForDevicesSeconds - tickSeconds;
}

} // namespace Android

//-------------------------------------------------------------------------------------
// ***** Creation

// Creates a new HIDDeviceManager and initializes OVR.
HIDDeviceManager* HIDDeviceManager::Create()
{
    OVR_ASSERT_LOG(false, ("Standalone mode not implemented yet."));

    if (!System::IsInitialized())
    {
        // Use custom message, since Log is not yet installed.
        OVR_DEBUG_STATEMENT(Log::GetDefaultLog()->
            LogMessage(Log_Debug, "HIDDeviceManager::Create failed - NervGear::System not initialized"); );
        return 0;
    }

    Ptr<Android::HIDDeviceManager> manager = *new Android::HIDDeviceManager(NULL);

    if (manager)
    {
        if (manager->Initialize())
        {
            manager->AddRef();
        }
        else
        {
            manager.Clear();
        }
    }

    return manager.GetPtr();
}

} // namespace NervGear

