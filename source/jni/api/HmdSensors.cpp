/************************************************************************************

Filename    :   HmdSensors.cpp
Content     :   State associated with a single HMD
Created     :   January 24, 2014
Authors     :   Michael Antonov

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#include "HmdSensors.h"

#include <unistd.h>					// gettid()
#include "Android/LogUtils.h"

HMDState::HMDState() :
		m_sensorStarted( 0 ),
		m_sensorCaps( 0 ),
		m_sensorChangedCount( 0 ),
		m_latencyTesterChangedCount( 0 )
{
}

HMDState::~HMDState()
{
	stopSensor();
	RemoveHandlerFromDevices();
}

bool HMDState::initDevice()
{
	m_deviceManager = *NervGear::DeviceManager::Create();
	m_deviceManager->SetMessageHandler( this );

	// Just use the first HMDDevice found.
	m_device = *m_deviceManager->EnumerateDevices<NervGear::HMDDevice>().createDevice();
	return ( m_device != NULL );
}

bool HMDState::startSensor( unsigned supportedCaps, unsigned requiredCaps )
{
	if ( m_sensorStarted )
	{
		stopSensor();
	}

	supportedCaps |= requiredCaps;

	if ( requiredCaps & ovrHmdCap_Position )
	{
		LOG( "HMDState::StartSensor: ovrHmdCap_Position not supported." );
		return false;
	}

	// Assume the first sensor found is associated with this HMDDevice.
	m_sensor = *m_deviceManager->EnumerateDevices<NervGear::SensorDevice>().createDevice();
	if ( m_sensor != NULL )
	{
		m_sensor->SetCoordinateFrame( NervGear::SensorDevice::Coord_HMD );
		m_sensor->SetReportRate( 500 );
		m_sFusion.AttachToSensor( m_sensor );
		m_sFusion.SetYawCorrectionEnabled( ( supportedCaps & ovrHmdCap_YawCorrection ) != 0 );

		if ( requiredCaps & ovrHmdCap_YawCorrection )
		{
			if ( !m_sFusion.HasMagCalibration() )
			{
				LOG( "HMDState::StartSensor: ovrHmdCap_YawCorrection not available." );
				m_sFusion.AttachToSensor( 0 );
				m_sFusion.Reset();
				m_sensor.Clear();
				return false;
			}
		}
		LOG( "HMDState::StartSensor: created sensor." );
	}
	else
	{
		if ( requiredCaps & ovrHmdCap_Orientation )
		{
			LOG( "HMDState::StartSensor: ovrHmdCap_Orientation not available." );
			return false;
		}
		LOG( "HMDState::StartSensor: wait for sensor." );
	}

	m_sensorStarted = true;
	m_sensorCaps = supportedCaps;

	return true;
}

void HMDState::stopSensor()
{
	if ( m_sensorStarted )
	{
		m_sFusion.AttachToSensor( 0 );
		m_sFusion.Reset();
		m_sensor.Clear();
		m_sensorCaps = 0;
		m_sensorStarted = false;
		m_lastSensorState = NervGear::SensorState();
		LOG( "HMDState::StopSensor: stopped sensor.\n" );
	}
}

void HMDState::resetSensor()
{
	m_sFusion.Reset();
	m_lastSensorState = NervGear::SensorState();
}

NervGear::SensorInfo	HMDState::sensorInfo()
{
	NervGear::SensorInfo si;

	if ( m_sensor != NULL )
	{
		m_sensor->getDeviceInfo( &si );
	}
	else
	{
		memset( &si, 0, sizeof( si ) );
	}
	return si;
}

float HMDState::yaw()
{
	return m_sFusion.GetYaw();
}

void HMDState::setYaw( float yaw )
{
	m_sFusion.SetYaw( yaw );
}

void HMDState::recenterYaw()
{
	m_sFusion.RecenterYaw();
}

// Any number of threads can fetch the predicted sensor state.
NervGear::SensorState HMDState::predictedSensorState( double absTime )
{
	// Only update when a device was added or removed because enumerating the devices is expensive.
	if ( m_sensorChangedCount > 0 )
	{
		// Use a mutex to keep multiple threads from trying to create a sensor simultaneously.
		m_sensorChangedMutex.lock();
		for ( int count = m_sensorChangedCount; count > 0; count = m_sensorChangedCount )
		{
			m_sensorChangedCount -= count;

			// Assume the first sensor found is associated with this HMDDevice.
			m_sensor = *m_deviceManager->EnumerateDevices<NervGear::SensorDevice>().createDevice();
			if ( m_sensor != NULL )
			{
				m_sensor->SetCoordinateFrame( NervGear::SensorDevice::Coord_HMD );
				m_sensor->SetReportRate( 500 );
				m_sFusion.AttachToSensor( m_sensor );
				m_sFusion.SetYawCorrectionEnabled( ( m_sensorCaps & ovrHmdCap_YawCorrection ) != 0 );

				// restore the yaw from the last sensor state when we re-connect to a sensor
				float yaw, pitch, roll;
				m_lastSensorState.Predicted.Transform.Orientation.GetEulerAngles< NervGear::Axis_Y, NervGear::Axis_X, NervGear::Axis_Z >( &yaw, &pitch, &roll );
				m_sFusion.SetYaw( yaw );

				LOG( "HMDState::PredictedSensorState: created sensor (tid=%d)", gettid() );
			}
			else
			{
				m_sFusion.AttachToSensor( 0 );
				LOG( "HMDState::PredictedSensorState: wait for sensor (tid=%d)", gettid() );
			}
		}
		m_sensorChangedMutex.unlock();
	}

	if ( m_sensor != NULL )
	{
		// GetPredictionForTime() is thread safe.
		m_lastSensorState = m_sFusion.GetPredictionForTime( absTime );
	}
	else
	{
		// Always set valid times so frames will get a delta-time and
		// joypad operation works when the sensor isn't connected.
		m_lastSensorState.Recorded.TimeInSeconds = absTime;
		m_lastSensorState.Predicted.TimeInSeconds = absTime;
	}

	return m_lastSensorState;
}

// Only one thread can use the latency tester.
bool HMDState::processLatencyTest( unsigned char rgbColorOut[3] )
{
	// Only update when a device was added or removed because enumerating the devices is expensive.
	if ( m_latencyTesterChangedCount > 0 )
	{
		// Use a mutex to keep multiple threads from trying to create a latency tester simultaneously.
		m_latencyTesterChangedMutex.lock();
		for ( int count = m_latencyTesterChangedCount; count > 0; count = m_latencyTesterChangedCount )
		{
			m_latencyTesterChangedCount -= count;

			// Assume the first latency tester found is associated with this HMDDevice.
			m_latencyTester = *m_deviceManager->EnumerateDevices<NervGear::LatencyTestDevice>().createDevice();
			if ( m_latencyTester != NULL )
			{
				m_latencyUtil.SetDevice( m_latencyTester );
				LOG( "HMDState::ProcessLatencyTest: created latency tester (tid=%d)", gettid() );
			}
			else
			{
				m_latencyUtil.SetDevice( 0 );
				LOG( "HMDState::ProcessLatencyTest: wait for latency tester (tid=%d)", gettid() );
			}
		}
		m_latencyTesterChangedMutex.unlock();
	}

	bool result = false;

	if ( m_latencyTester != NULL )
	{
		NervGear::VColor colorToDisplay;

		// NOTE: ProcessInputs() is not thread safe.
		m_latencyUtil.ProcessInputs();
		result = m_latencyUtil.DisplayScreenColor( colorToDisplay );
		rgbColorOut[0] = colorToDisplay.red;
		rgbColorOut[1] = colorToDisplay.green;
		rgbColorOut[2] = colorToDisplay.blue;
	}

	return result;
}

// OnMessage() is called from the device manager thread.
void HMDState::onMessage( const NervGear::Message & msg )
{
	if ( msg.pDevice != m_deviceManager )
	{
		return;
	}

	const NervGear::MessageDeviceStatus & statusMsg = static_cast<const NervGear::MessageDeviceStatus &>(msg);

	if ( statusMsg.Handle.type() == NervGear::Device_Sensor )
	{
		m_sensorChangedCount++;
		if ( msg.Type == NervGear::Message_DeviceAdded )
		{
			LOG( "HMDState::OnMessage: added Device_Sensor (tid=%d, cnt=%d)", gettid(), m_sensorChangedCount );
		}
		else if ( msg.Type == NervGear::Message_DeviceRemoved )
		{
			LOG( "HMDState::OnMessage: removed Device_Sensor (tid=%d, cnt=%d)", gettid(), m_sensorChangedCount );
		}
	}
	else if ( statusMsg.Handle.type() == NervGear::Device_LatencyTester )
	{
		m_latencyTesterChangedCount++;
		if ( msg.Type == NervGear::Message_DeviceAdded )
		{
			LOG( "HMDState::OnMessage: added Device_LatencyTester (tid=%d, cnt=%d)", gettid(), m_latencyTesterChangedCount );
		}
		else if ( msg.Type == NervGear::Message_DeviceRemoved )
		{
			LOG( "HMDState::OnMessage: removed Device_LatencyTester (tid=%d, cnt=%d)", gettid(), m_latencyTesterChangedCount );
		}
	}
}
