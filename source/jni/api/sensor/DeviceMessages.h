#pragma once

#include "vglobal.h"
#include "DeviceConstants.h"
#include "DeviceHandle.h"

#include "VMath.h"
#include "Array.h"
#include "VColor.h"

NV_NAMESPACE_BEGIN

class DeviceBase;
class DeviceHandle;


#define OVR_MESSAGETYPE(devName, msgIndex)   ((Device_##devName << 8) | msgIndex)

// MessageType identifies the structure of the Message class; based on the message,
// casting can be used to obtain the exact value.
enum MessageType
{
    // Used for unassigned message types.
    Message_None            = 0,

    // Device Manager Messages
    Message_DeviceAdded             = OVR_MESSAGETYPE(Manager, 0),  // A new device is detected by manager.
    Message_DeviceRemoved           = OVR_MESSAGETYPE(Manager, 1),  // Existing device has been plugged/unplugged.
    // Sensor Messages
    Message_BodyFrame               = OVR_MESSAGETYPE(Sensor, 0),   // Emitted by sensor at regular intervals.
    // Latency Tester Messages
    Message_LatencyTestSamples          = OVR_MESSAGETYPE(LatencyTester, 0),
    Message_LatencyTestColorDetected    = OVR_MESSAGETYPE(LatencyTester, 1),
    Message_LatencyTestStarted          = OVR_MESSAGETYPE(LatencyTester, 2),
    Message_LatencyTestButton           = OVR_MESSAGETYPE(LatencyTester, 3),

};

//-------------------------------------------------------------------------------------
// Base class for all messages.
class Message
{
public:
    Message(MessageType type = Message_None,
            DeviceBase* pdev = 0) : Type(type), pDevice(pdev)
    { }

    MessageType Type;    // What kind of message this is.
    DeviceBase* pDevice; // Device that emitted the message.
};


// Sensor BodyFrame notification.
// Sensor uses Right-Handed coordinate system to return results, with the following
// axis definitions:
//  - Y Up positive
//  - X Right Positive
//  - Z Back Positive
// Rotations a counter-clockwise (CCW) while looking in the negative direction
// of the axis. This means they are interpreted as follows:
//  - Roll is rotation around Z, counter-clockwise (tilting left) in XY plane.
//  - Yaw is rotation around Y, positive for turning left.
//  - Pitch is rotation around X, positive for pitching up.

class MessageBodyFrame : public Message
{
public:
    MessageBodyFrame(DeviceBase* dev)
        : Message(Message_BodyFrame, dev), Temperature(0.0f), TimeDelta(0.0f)
    {
    }

    // JDC: added this so I can have an array of them
    MessageBodyFrame()
    	: Message(Message_BodyFrame,NULL), Temperature(0.0f), TimeDelta(0.0f)
    {
    }

    Vector3f Acceleration;  // Acceleration in m/s^2.
    Vector3f RotationRate;  // Angular velocity in rad/s.
    Vector3f MagneticField; // Magnetic field strength in Gauss.
    Vector3f MagneticBias;  // Magnetic field calibration bias in Gauss.
    float    Temperature;   // Temperature reading on sensor surface, in degrees Celsius.
    float    TimeDelta;     // Time passed since last Body Frame, in seconds.

    // The absolute time from the host computers perspective that the message should be
    // interpreted as.  This is derived from the rolling 16 bit timestamp on the incoming
    // messages and a continuously correcting delta value maintained by SensorDeviceImpl.
    //
    // Integration should use TimeDelta, but prediction into the future should derive
    // the delta time from PredictToSeconds - AbsoluteTimeSeconds.
    //
    // This value will always be <= the return from a call to Timer::GetSeconds().
    //
    // This value will not usually be an integral number of milliseconds, and it will
    // drift by fractions of a millisecond as the time delta between the sensor and
    // host is continuously adjusted.
    double   AbsoluteTimeSeconds;
};

// Sent when we receive a device status changes (e.g.:
// Message_DeviceAdded, Message_DeviceRemoved).
class MessageDeviceStatus : public Message
{
public:
	MessageDeviceStatus(MessageType type, DeviceBase* dev, const DeviceHandle &hdev)
		: Message(type, dev), Handle(hdev) { }

	DeviceHandle Handle;
};

//-------------------------------------------------------------------------------------
// ***** Latency Tester

// Sent when we receive Latency Tester samples.
class MessageLatencyTestSamples : public Message
{
public:
    MessageLatencyTestSamples(DeviceBase* dev)
        : Message(Message_LatencyTestSamples, dev)
    {
    }

    Array<VColor>     Samples;
};

// Sent when a Latency Tester 'color detected' event occurs.
class MessageLatencyTestColorDetected : public Message
{
public:
    MessageLatencyTestColorDetected(DeviceBase* dev)
        : Message(Message_LatencyTestColorDetected, dev)
    {
    }

    UInt16      Elapsed;
    VColor       DetectedValue;
    VColor       TargetValue;
};

// Sent when a Latency Tester 'change color' event occurs.
class MessageLatencyTestStarted : public Message
{
public:
    MessageLatencyTestStarted(DeviceBase* dev)
        : Message(Message_LatencyTestStarted, dev)
    {
    }

    VColor    TargetValue;
};

// Sent when a Latency Tester 'button' event occurs.
class MessageLatencyTestButton : public Message
{
public:
    MessageLatencyTestButton(DeviceBase* dev)
        : Message(Message_LatencyTestButton, dev)
    {
    }

};

NV_NAMESPACE_END