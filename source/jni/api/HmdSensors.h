#pragma once

#include "vglobal.h"
#include "VMutex.h"

#include "sensor/SensorFusion.h"
#include "sensor/LatencyTest.h"
#include "VAtomicInt.h"

// HMD capability bits reported by device.
typedef enum
{
    ovrHmdCap_Orientation       = 0x0010,   //  Support orientation tracking (IMU).
    ovrHmdCap_YawCorrection     = 0x0020,   //  Supports yaw correction through magnetometer or other means.
    ovrHmdCap_Position          = 0x0040,   //  Supports positional tracking.
} ovrHmdCapBits;

NV_USING_NAMESPACE

class HMDState : public MessageHandler
{
public:
							HMDState();
							~HMDState();

    bool					initDevice();

    bool					startSensor( unsigned supportedCaps, unsigned requiredCaps );
    void					stopSensor();
    void					resetSensor();
    SensorInfo			sensorInfo();

    float					yaw();
    void					setYaw( float yaw );
    void					recenterYaw();

    SensorState		predictedSensorState( double absTime );

    bool					processLatencyTest( unsigned char rgbColorOut[3] );
    const char *			latencyTestResult() { return m_latencyUtil.GetResultsString(); }

    DeviceManager *	deviceManager() { return m_deviceManager.GetPtr(); }

    void					onMessage( const Message & msg );

private:
    Ptr<DeviceManager>		m_deviceManager;
    Ptr<HMDDevice>			m_device;

    bool								m_sensorStarted;
    unsigned							m_sensorCaps;

    VAtomicInt					m_sensorChangedCount;
    VMutex							m_sensorChangedMutex;
    Ptr<SensorDevice>			m_sensor;
    SensorFusion					m_sFusion;

    VAtomicInt					m_latencyTesterChangedCount;
    VMutex							m_latencyTesterChangedMutex;
    Ptr<LatencyTestDevice>	m_latencyTester;
    LatencyTest					m_latencyUtil;

    SensorState					m_lastSensorState;
};


