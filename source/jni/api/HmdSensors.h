/************************************************************************************

Filename    :   HmdSensors.h
Content     :   State associated with a single HMD
Created     :   January 24, 2014
Authors     :   Michael Antonov

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#ifndef OVR_HMDState_h
#define OVR_HMDState_h

#include "sensor/SensorFusion.h"
#include "sensor/LatencyTest.h"

// HMD capability bits reported by device.
typedef enum
{
    ovrHmdCap_Orientation       = 0x0010,   //  Support orientation tracking (IMU).
    ovrHmdCap_YawCorrection     = 0x0020,   //  Supports yaw correction through magnetometer or other means.
    ovrHmdCap_Position          = 0x0040,   //  Supports positional tracking.
} ovrHmdCapBits;

class HMDState : public NervGear::MessageHandler, public NervGear::NewOverrideBase
{
public:
							HMDState();
							~HMDState();

    bool					initDevice();

    bool					startSensor( unsigned supportedCaps, unsigned requiredCaps );
    void					stopSensor();
    void					resetSensor();
    NervGear::SensorInfo			sensorInfo();

    float					yaw();
    void					setYaw( float yaw );
    void					recenterYaw();

    NervGear::SensorState		predictedSensorState( double absTime );

    bool					processLatencyTest( unsigned char rgbColorOut[3] );
    const char *			latencyTestResult() { return m_latencyUtil.GetResultsString(); }

    NervGear::DeviceManager *	deviceManager() { return m_deviceManager.GetPtr(); }

    void					onMessage( const NervGear::Message & msg );

private:
    NervGear::Ptr<NervGear::DeviceManager>		m_deviceManager;
    NervGear::Ptr<NervGear::HMDDevice>			m_device;

    bool								m_sensorStarted;
    unsigned							m_sensorCaps;

    NervGear::AtomicInt<int>					m_sensorChangedCount;
    NervGear::Mutex							m_sensorChangedMutex;
    NervGear::Ptr<NervGear::SensorDevice>			m_sensor;
    NervGear::SensorFusion					m_sFusion;

    NervGear::AtomicInt<int>					m_latencyTesterChangedCount;
    NervGear::Mutex							m_latencyTesterChangedMutex;
    NervGear::Ptr<NervGear::LatencyTestDevice>	m_latencyTester;
    NervGear::LatencyTest					m_latencyUtil;

    NervGear::SensorState					m_lastSensorState;
};

#endif	// !OVR_HmdSensors_h
