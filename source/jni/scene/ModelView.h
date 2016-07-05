#pragma once

#include "vglobal.h"

#include "App.h"		// VrFrame
#include "DebugLines.h"

#pragma once

NV_NAMESPACE_BEGIN

//-----------------------------------------------------------------------------------
// OvrSceneView
//
class OvrSceneView
{
public:
	OvrSceneView();

	// Allow movement inside the scene based on the joypad.
	// Sets the timeWarpParms for smooth joypad turning while dropping frames.
	// Models that have DontRenderForClientUid == supressModelsWithClientId will be skipped
	// to prevent the client's own head model from drawing in their view.
	void		Frame(const VViewSettings viewParms_, const VFrame vrFrame,
            VR4Matrixf & timeWarpParmsExternalVelocity, const long long supressModelsWithClientId = -1 );

	// Issues GL calls and returns the MVP for the eye, as needed by AppInterface DrawEyeVIew
    VR4Matrixf	DrawEyeView( const int eye, const float fovDegrees ) const;

	// WARNING: this does not take into account the head model, it is just footpos + eyeheight
    V3Vectf	CenterEyePos() const;

	// This includes the head/neck model or position tracking.
//    V3Vectf	ShiftedCenterEyePos() const;

//    V3Vectf	Forward() const;
    VR4Matrixf 	CenterViewMatrix() const;
    VR4Matrixf 	ViewMatrixForEye( const int eye ) const;	// includes InterpupillaryDistance
    VR4Matrixf 	MvpForEye( const int eye, const float fovDegrees ) const;
    VR4Matrixf 	ProjectionMatrixForEye( const int eye, const float fovDegrees ) const;

    static V3Vectf HeadModelOffset(float EyeRoll, float EyePitch, float EyeYaw, float HeadModelDepth, float HeadModelHeight);

	void		UpdateViewMatrix(const VFrame vrFrame );
	VGlShader				ProgVertexColor;
	VGlShader				ProgSingleTexture;
	VGlShader				ProgLightMapped;
	VGlShader				ProgReflectionMapped;
	VGlShader				ProgSkinnedVertexColor;
	VGlShader				ProgSkinnedSingleTexture;
	VGlShader				ProgSkinnedLightMapped;
	VGlShader				ProgSkinnedReflectionMapped;
	bool					LoadedPrograms;

	// Updated each Frame()
	VViewSettings				ViewParms;

	// 3.0 m/s by default.  Different apps may want different move speeds
    float					MoveSpeed;

	// For small scenes with 16 bit depth buffers, it is useful to
	// keep the ratio as small as possible.
	float					Znear;
	float					Zfar;

	// Position tracking test
    V3Vectf				ImuToEyeCenter;

	// Angle offsets in radians
	float					YawOffset;		// added on top of the sensor reading
//	float					PitchOffset;	// only applied if the tracking sensor isn't active

	// Applied one frame later to avoid bounce-back from async time warp yaw velocity prediction.
	float					YawVelocity;

	// This is only for experiments right now.
	bool					AllowPositionTracking;

	// Allow smooth transition from head model to position tracking experiments
    V3Vectf				LastHeadModelOffset;
    V3Vectf				LatchedHeadModelOffset;

    // Calculated in Frame()
    VR4Matrixf 				ViewMatrix;
    float					EyeYaw;         // Rotation around Y, CCW positive when looking at RHS (X,Z) plane.
    float					EyePitch;       // Pitch. If sensor is plugged in, only read from sensor.
    float					EyeRoll;        // Roll, only accessible from Sensor.

    // Includes the head/neck model or position tracking
    V3Vectf				ShiftedEyePos;

    // Modified by joypad movement and collision detection
    V3Vectf				FootPos;
};

NV_NAMESPACE_END


