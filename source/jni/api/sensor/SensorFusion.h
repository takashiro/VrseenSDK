#pragma once

#include "Device.h"
#include "SensorFilter.h"
#include "VLockless.h"
#include "VMutex.h"
//#include <time.h>

// VrApi forward declarations.
typedef struct ovrPoseStatef_ ovrPoseStatef;
typedef struct ovrSensorState_ ovrSensorState;

NV_NAMESPACE_BEGIN

//-------------------------------------------------------------------------------------
// ***** Sensor State

// These values are reported as compatible with VrApi.


// PoseState describes the complete pose, or a rigid body configuration, at a
// point in time, including first and second derivatives. It is used to specify
// instantaneous location and movement of the headset.
// SensorState is returned as a part of the sensor state.
template<class T>
class PoseState
{
public:

    PoseState() : TimeInSeconds(0.0) { }
    // float <-> double conversion constructor.
    explicit PoseState(const PoseState<typename VConstants<T>::VdifFloat> &src)
        : Transform(src.Transform),
          AngularVelocity(src.AngularVelocity), LinearVelocity(src.LinearVelocity),
          AngularAcceleration(src.AngularAcceleration), LinearAcceleration(src.LinearAcceleration),
          TimeInSeconds(src.TimeInSeconds)
    { }



    VPos<T>     Transform;
    V3Vect<T>  AngularVelocity;
    V3Vect<T>  LinearVelocity;
    V3Vect<T>  AngularAcceleration;
    V3Vect<T>  LinearAcceleration;
    // Absolute time of this state sample; always a double measured in seconds.
    double      TimeInSeconds;
};

typedef PoseState<float>  PoseStatef;

// Bit flags describing the current status of sensor tracking.
enum StatusBits
{
    Status_OrientationTracked    = 0x0001,   // Orientation is currently tracked (connected and in use).
    Status_PositionTracked       = 0x0002,   // Position is currently tracked (false if out of range).
    Status_PositionConnected     = 0x0020,   // Position tracking HW is conceded.
    Status_HmdConnected          = 0x0080    // HMD Display is available & connected.
};


// Full state of of the sensor reported by GetSensorState() at a given absolute time.
class SensorState
{
public:
    SensorState() : Temperature(0), Status(0) { }

    // C-interop support
    SensorState(const ovrSensorState& s);
    operator const ovrSensorState& () const;

    // Pose state at the time that SensorState was requested.
    PoseStatef   Predicted;
    // Actual recorded pose configuration based on sensor sample at a
    // moment closest to the requested time.
    PoseStatef   Recorded;

    // Sensor temperature reading, in degrees Celcius, as sample time.
    float        Temperature;
    // Sensor status described by ovrStatusBits.
    unsigned     Status;
};




//-------------------------------------------------------------------------------------
// ***** SensorFusion

// SensorFusion class accumulates Sensor notification messages to keep track of
// orientation, which involves integrating the gyro and doing correction with gravity.
// Magnetometer based yaw drift correction is also supported; it is usually enabled
// automatically based on loaded magnetometer configuration.
// Orientation is reported as a quaternion, from which users can obtain either the
// rotation matrix or Euler angles.
//
// The class can operate in two ways:
//  - By user manually passing MessageBodyFrame messages to the OnMessage() function.
//  - By attaching SensorFusion to a SensorDevice, in which case it will
//    automatically handle notifications from that device.


class SensorFusion
{
    enum
    {
        MagMaxReferences = 1000,
		MagLatencyBufferSizeMax = 512,
		MagLatencyCompensationMilliseconds = 95,
    };

public:
    SensorFusion(SensorDevice* sensor = 0);
    ~SensorFusion();

	bool ApplyDrift;

    // *** Setup

    // Attaches this SensorFusion to a sensor device, from which it will receive
    // notification messages. If a sensor is attached, manual message notification
    // is not necessary. Calling this function also resets SensorFusion state.
    bool        AttachToSensor(SensorDevice* sensor);

    // Returns true if this Sensor fusion object is attached to a sensor.
    bool        IsAttachedToSensor() const  { return pHandler->IsHandlerInstalled(); }

    // Resets everything.
    void        Reset();


    // *** State Query - These can be called any time from any thread.

    // The orientation will be predicted to the given time from the base of the most
    // recently processed messages.  In general, absoluteTimeSeconds should always be
    // ahead of the most recent message time, but it is possible for a new messages to
    // arrive right before processing, giving a small negative delta in rare cases.
    SensorState GetPredictionForTime( double absoluteTimeSeconds ) const;

	// Get the current yaw.
	float		GetYaw();

	// Set a particular yaw.
	void        SetYaw( float newYaw );

	// Recenters the current orientation on yaw axis
	void        RecenterYaw();


    // *** Configuration

    void        EnableMotionTracking(bool enable = true)    { MotionTrackingEnabled = enable; }
    bool        IsMotionTrackingEnabled() const             { return MotionTrackingEnabled;   }



    // *** Accelerometer/Gravity Correction Control

    // Enables/disables gravity correction (on by default).
    void        SetGravityEnabled(bool enableGravity)       { EnableGravity = enableGravity; }
    bool        IsGravityEnabled() const                    { return EnableGravity;}


    // *** Magnetometer and Yaw Drift Correction Control

    // Enables/disables magnetometer based yaw drift correction. Must also have mag calibration
    // data for this correction to work.
	void        SetYawCorrectionEnabled(bool enable)    { EnableYawCorrection = enable; }
    // Determines if yaw correction is enabled.
    bool        IsYawCorrectionEnabled() const          { return EnableYawCorrection;}
#if defined( ANDROID )
    bool 		HasMagCalibration() const				{ return true; }
#elif !defined( FBX_TOOL )
    // this is Android only implementation for now
    bool 		HasMagCalibration() const				{ OVR_COMPILER_ASSERT(false); return false; }
#endif

    // Sets the focus filter direction to the current HMD direction
    void		SetFocusDirection();
    // Sets the focus filter to a direction in the body frame. Once set, a complementary filter
    // will very slowly drag the world to keep the direction of the HMD within the FOV of the focus
    void		SetFocusDirection(V3Vectf direction);
    // Sets the FOV (in radians) of the focus. When the yaw difference between the HMD's current pose
    // and the focus is smaller than the FOV, the complementary filter does not act.
    void		SetFocusFOV(float rads);
    // Turns off the focus filter (equivalent to setting the focus to 0
    void		ClearFocus();

    // *** Message Handler Logic

    // Notifies SensorFusion object about a new BodyFrame message from a sensor.
    // Should be called by user if not attaching to a sensor.
    void        OnMessage(const MessageBodyFrame& msg)
    {
        OVR_ASSERT(!IsAttachedToSensor());
        handleMessage(msg);
    }


private:

    // Internal handler for messages; bypasses error checking.
    void        handleMessage(const MessageBodyFrame& msg);

    // Apply headset yaw correction from magnetometer
	// for models without camera or when camera isn't available
    void        applyMagYawCorrection(const V3Vectf& magUncalibrated, const V3Vectf& magBias, const V3Vectf& gyro, float deltaT);
    // Apply headset tilt correction from the accelerometer
    void        applyTiltCorrection(float deltaT);
	// Apply camera focus correction
	void		applyFocusCorrection(float deltaT);

    class BodyFrameHandler : public MessageHandler
    {
        SensorFusion* pFusion;
    public:
        BodyFrameHandler(SensorFusion* fusion) : pFusion(fusion) { }
        ~BodyFrameHandler();

        virtual void onMessage(const Message& msg);
        virtual bool SupportsMessageType(MessageType type) const;
    };

    // This is the state needed by GetPredictionForTime()
    class StateForPrediction
    {
    public:
        // time the current state is correct for
    	PoseStatef        State;
    	float             Temperature;

    	StateForPrediction() : Temperature(0) { };
    };

    struct MagReferencePoint
    {
        V3Vectf          MagUncalibratedInImuFrame;
        VQuatf             WorldFromImu;
		int				  Score;

		MagReferencePoint() { }
        MagReferencePoint(const V3Vectf& magUncalibInImuFrame, const VQuatf& worldFromImu, int score)
            : MagUncalibratedInImuFrame(magUncalibInImuFrame), WorldFromImu(worldFromImu), Score(score) { }
    };


    bool getBufferedOrientation(VQuatf* orientation, const V3Vectf& gyro, float gyroThreshold, float deltaT);

    BodyFrameHandler*		pHandler;

    // This can be read without any locks, so a high priority rendering thread doesn't
    // have to worry about being blocked by a sensor thread that got preempted.
    VLockless<StateForPrediction>	UpdatedState;

    // The phase of the head as estimated by sensor fusion
	PoseStatef              State;
    unsigned int            Stage;

    SensorFilterBodyFrame   FAccelHeadset;
    SensorFilterf           FAngV;

    bool                    MotionTrackingEnabled;
    bool                    EnableGravity;
    bool                    EnableYawCorrection;

    VArray<MagReferencePoint> MagRefs;
    int                     MagRefIdx;
    float                   MagCorrectionIntegralTerm;

	// Apply compensation since magnetometer latency is higher than HMT sensors. We also don't perform mag yaw correction if the
	// angular velocity is too high.
	struct MagCompEntry
	{
        VQuatf	Orientation;
		float	GyroMagnitude;
	};
	MagCompEntry			MagLatencyCompBuffer[MagLatencyBufferSizeMax];
	int						MagLatencyCompBufferIndex;
	int						MagLatencyCompFillCount;
	float					YawCorrectionTimer;

    V3Vectf				FocusDirection;
	float					FocusFOV;

    VMutex					RecenterMutex;
    VLockless<VPosf>	RecenterTransform;	// this is an additional transform that is applied to "recenter" the orientation in yaw.
};


NV_NAMESPACE_END
