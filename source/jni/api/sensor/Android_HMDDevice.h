#pragma once

#include "vglobal.h"

#include "Android_DeviceManager.h"
#include "Profile.h"

NV_NAMESPACE_BEGIN

namespace Android {

class HMDDevice;

//-------------------------------------------------------------------------------------

// HMDDeviceFactory enumerates attached Oculus HMD devices.
//
// This is currently done by matching monitor device strings.

class HMDDeviceFactory : public DeviceFactory
{
public:
	static HMDDeviceFactory &GetInstance();

    // Enumerates devices, creating and destroying relevant objects in manager.
    virtual void EnumerateDevices(EnumerateVisitor& visitor);

protected:
    DeviceManager* getManager() const { return (DeviceManager*) pManager; }
};


class HMDDeviceCreateDesc : public DeviceCreateDesc
{
    friend class HMDDevice;

protected:
    enum
    {
        Contents_Screen     = 1,
        Contents_Distortion = 2,
        Contents_7Inch      = 4,
    };
    VString              DeviceId;
    VString              DisplayDeviceName;
    struct
    {
        int             X, Y;
    }                   Desktop;
    unsigned int        Contents;

    VSizei               ResolutionInPixels;
    VSizef               ScreenSizeInMeters;

    // TODO: add VCenterFromTopInMeters, LensSeparationInMeters and LensDiameterInMeters
    DistortionEqnType   DistortionEqn;
    float               DistortionK[4];

    long                DisplayId;

public:
    HMDDeviceCreateDesc(DeviceFactory* factory, const VString& displayDeviceName, long dispId);
    HMDDeviceCreateDesc(const HMDDeviceCreateDesc& other);

    DeviceCreateDesc* Clone() const override
    {
        return new HMDDeviceCreateDesc(*this);
    }

    DeviceBase* NewDeviceInstance() override;

    MatchResult MatchDevice(const DeviceCreateDesc& other, DeviceCreateDesc**) const override;

    // Matches device by path.
    bool MatchDeviceByPath(const VString& path) override;

    bool UpdateMatchedCandidate(const DeviceCreateDesc&, bool* newDeviceFlag = NULL) override;

    bool GetDeviceInfo(DeviceInfo* info) const override;

    // Requests the currently used default profile. This profile affects the
    // settings reported by HMDInfo.
    Profile* GetProfileAddRef() const;

    ProfileType GetProfileType() const
    {
        return (ResolutionInPixels.Width >= 1920) ? Profile_RiftDKHD : Profile_RiftDK1;
    }


    void  SetScreenParameters(int x, int y, unsigned hres, unsigned vres, float hsize, float vsize)
    {
        Desktop.X = x;
        Desktop.Y = y;
        ResolutionInPixels = VSizei(hres, vres);
        ScreenSizeInMeters = VSizef(hsize, vsize);
        Contents |= Contents_Screen;
    }
    void SetDistortion(const float* dks)
    {
        for (int i = 0; i < 4; i++)
            DistortionK[i] = dks[i];
        // TODO: add DistortionEqn
        Contents |= Contents_Distortion;
    }

    void Set7Inch() { Contents |= Contents_7Inch; }

    bool Is7Inch() const;
};


//-------------------------------------------------------------------------------------

// HMDDevice represents an Oculus HMD device unit. An instance of this class
// is typically created from the DeviceManager.
//  After HMD device is created, we its sensor data can be obtained by
//  first creating a Sensor object and then wrappig it in SensorFusion.

class HMDDevice : public DeviceImpl<NervGear::HMDDevice>
{
public:
    HMDDevice(HMDDeviceCreateDesc* createDesc);
    ~HMDDevice();

    bool  initialize(DeviceBase* parent) override;
    void shutdown() override;

    // Requests the currently used default profile. This profile affects the
    // settings reported by HMDInfo.
    Profile*    profile() const override;
    const char* profileName() const override;
    bool        setProfileName(const char* name) override;

protected:
    HMDDeviceCreateDesc* getDesc() const { return (HMDDeviceCreateDesc*)pCreateDesc.GetPtr(); }

    // User name for the profile used with this device.
    VString               m_profileName;
    mutable Ptr<Profile> m_pCachedProfile;
};


}

NV_NAMESPACE_END
