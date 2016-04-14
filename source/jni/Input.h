#pragma once

#include "VInput.h"
#include "VRotationSensor.h"

NV_NAMESPACE_BEGIN

// Passed to applications each frame.
struct VrFrame
{
	VrFrame() : OvrStatus( 0 ), DeltaSeconds( 0.0f ), FrameNumber( 0 ) {}

	// Updated every frame, TimeWarp will transform from this
	// value to whatever the current orientation is when displaying
	// the eyeFrameBuffers after they are submitted next GetEyeBuffers().
	//
	// Applications will perform additional centering and rotation
	// by transforming this value before use.
	//
	// To make accurate journal playback possible, applications should
	// use poseState.TimeInSeconds instead of geting system time directly.
    VRotationSensor::State pose;

	// ovrStatus_OrientationTracked, etc
	unsigned		OvrStatus;

	// The amount of time that has passed since the last frame,
	// usable for movement scaling.  Applications should not use any
	// other means of getting timing information.
	// This will be clamped to no more than 0.1 seconds to prevent
	// excessive movement after pauses for loading or initialization.
	float			DeltaSeconds;

	// incremented every time App::Frame() is called
	long long		FrameNumber;

	// Various different joypad button combinations are mapped to
	// standard positions.
    VInput			Input;
};

NV_NAMESPACE_END

