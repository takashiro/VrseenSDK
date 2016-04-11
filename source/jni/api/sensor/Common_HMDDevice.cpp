#include "Android_HMDDevice.h"
#include "Android_DeviceManager.h"
#include "Profile.h"

NV_NAMESPACE_BEGIN

namespace Android {

DeviceBase* HMDDeviceCreateDesc::NewDeviceInstance()
{
    return new HMDDevice(this);
}

bool HMDDeviceCreateDesc::Is7Inch() const
{
    return DeviceId.contains("OVR0001") || (Contents & Contents_7Inch);
}

Profile* HMDDeviceCreateDesc::GetProfileAddRef() const
{
    // Create device may override profile name, so get it from there is possible.
    ProfileManager* profileManager = GetManagerImpl()->GetProfileManager();
    ProfileType     profileType    = GetProfileType();
    const char *    profileName    = pDevice ?
                        ((HMDDevice*)pDevice)->profileName() :
                        profileManager->GetDefaultProfileName(profileType);

    return profileName ?
        profileManager->LoadProfile(profileType, profileName) :
        profileManager->GetDeviceDefaultProfile(profileType);
}



bool HMDDeviceCreateDesc::GetDeviceInfo(DeviceInfo* info) const
{
    if ((info->InfoClassType != Device_HMD) &&
        (info->InfoClassType != Device_None))
        return false;

	// LDC - Use zero data for now.
	info->Version = 0;
    info->ProductName.clear();
    info->Manufacturer.clear();

	if (info->InfoClassType == Device_HMD)
    {
		HMDInfo* hmdInfo = static_cast<HMDInfo*>(info);
        *hmdInfo = HMDInfo();
	}

    return true;
}


//-------------------------------------------------------------------------------------
// ***** HMDDevice

HMDDevice::HMDDevice(HMDDeviceCreateDesc* createDesc)
    : NervGear::DeviceImpl<NervGear::HMDDevice>(createDesc, 0)
{
}
HMDDevice::~HMDDevice()
{
}

bool HMDDevice::initialize(DeviceBase* parent)
{
    pParent = parent;

    // Initialize user profile to default for device.
    ProfileManager* profileManager = GetManager()->GetProfileManager();
    m_profileName = profileManager->GetDefaultProfileName(getDesc()->GetProfileType());

    return true;
}
void HMDDevice::shutdown()
{
    m_profileName.clear();
    m_pCachedProfile.Clear();
    pParent.Clear();
}

Profile* HMDDevice::profile() const
{
    if (!m_pCachedProfile)
        m_pCachedProfile = *getDesc()->GetProfileAddRef();
    return m_pCachedProfile.GetPtr();
}

const char* HMDDevice::profileName() const
{
    return m_profileName.toCString();
}

bool HMDDevice::setProfileName(const char* name)
{
    m_pCachedProfile.Clear();
    if (!name)
    {
        m_profileName.clear();
        return 0;
    }
    if (GetManager()->GetProfileManager()->HasProfile(getDesc()->GetProfileType(), name))
    {
        m_profileName = name;
        return true;
    }
    return false;
}

}

NV_NAMESPACE_END
