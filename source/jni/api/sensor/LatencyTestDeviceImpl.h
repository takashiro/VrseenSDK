#pragma once

#include "HIDDeviceImpl.h"

NV_NAMESPACE_BEGIN

struct LatencyTestSamplesMessage;
struct LatencyTestButtonMessage;
struct LatencyTestStartedMessage;
struct LatencyTestColorDetectedMessage;

//-------------------------------------------------------------------------------------
// LatencyTestDeviceFactory enumerates Oculus Latency Tester devices.
class LatencyTestDeviceFactory : public DeviceFactory
{
public:
	static LatencyTestDeviceFactory &GetInstance();

    // Enumerates devices, creating and destroying relevant objects in manager.
    virtual void EnumerateDevices(EnumerateVisitor& visitor);

    virtual bool MatchVendorProduct(UInt16 vendorId, UInt16 productId) const;
    virtual bool DetectHIDDevice(DeviceManager* pdevMgr, const HIDDeviceDesc& desc);

protected:
    DeviceManager* getManager() const { return (DeviceManager*) pManager; }
};


// Describes a single a Oculus Latency Tester device and supports creating its instance.
class LatencyTestDeviceCreateDesc : public HIDDeviceCreateDesc
{
public:
    LatencyTestDeviceCreateDesc(DeviceFactory* factory, const HIDDeviceDesc& hidDesc)
        : HIDDeviceCreateDesc(factory, Device_LatencyTester, hidDesc) { }

    virtual DeviceCreateDesc* Clone() const
    {
        return new LatencyTestDeviceCreateDesc(*this);
    }

    virtual DeviceBase* NewDeviceInstance();

    virtual MatchResult MatchDevice(const DeviceCreateDesc& other,
                                    DeviceCreateDesc**) const
    {
        if ((other.Type == Device_LatencyTester) && (pFactory == other.pFactory))
        {
            const LatencyTestDeviceCreateDesc& s2 = (const LatencyTestDeviceCreateDesc&) other;
            if (MatchHIDDevice(s2.HIDDesc))
                return Match_Found;
        }
        return Match_None;
    }

    virtual bool MatchHIDDevice(const HIDDeviceDesc& hidDesc) const
    {
        // should paths comparison be case insensitive?
        return ((HIDDesc.Path.icompare(hidDesc.Path) == 0) &&
                (HIDDesc.SerialNumber == hidDesc.SerialNumber));
    }
    virtual bool        GetDeviceInfo(DeviceInfo* info) const;
};


//-------------------------------------------------------------------------------------
// ***** LatencyTestDeviceImpl

// Oculus Latency Tester interface.

class LatencyTestDeviceImpl : public HIDDeviceImpl<LatencyTestDevice>
{
public:
     LatencyTestDeviceImpl(LatencyTestDeviceCreateDesc* createDesc);
    ~LatencyTestDeviceImpl();

    // DeviceCommon interface.
    virtual bool  initialize(DeviceBase* parent);
    virtual void shutdown();

    // DeviceManagerThread::Notifier interface.
    virtual void OnInputReport(UByte* pData, UInt32 length);

    // LatencyTesterDevice interface
    virtual bool SetConfiguration(const LatencyTestConfiguration& configuration, bool waitFlag = false);
    virtual bool GetConfiguration(LatencyTestConfiguration* configuration);

    virtual bool SetCalibrate(const VColor& calibrationColor, bool waitFlag = false);

    virtual bool SetStartTest(const VColor& targetColor, bool waitFlag = false);
    virtual bool SetDisplay(const LatencyTestDisplay& display, bool waitFlag = false);

protected:
    bool    openDevice(const char** errorFormatString);
    void    closeDevice();
    void    closeDeviceOnIOError();

    bool    initializeRead();
    bool    processReadResult();

    bool    setConfiguration(const LatencyTestConfiguration& configuration);
    bool    getConfiguration(LatencyTestConfiguration* configuration);
    bool    setCalibrate(const VColor& calibrationColor);
    bool    setStartTest(const VColor& targetColor);
    bool    setDisplay(const LatencyTestDisplay& display);

    // Called for decoded messages
    void onLatencyTestSamplesMessage(LatencyTestSamplesMessage* message);
    void onLatencyTestButtonMessage(LatencyTestButtonMessage* message);
    void onLatencyTestStartedMessage(LatencyTestStartedMessage* message);
    void onLatencyTestColorDetectedMessage(LatencyTestColorDetectedMessage* message);

};

NV_NAMESPACE_END

