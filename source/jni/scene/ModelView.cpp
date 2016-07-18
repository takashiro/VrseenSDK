#include "ModelView.h"
#include "VAlgorithm.h"
#include "api/VKernel.h"

#include "VFrame.h"		// VrFrame, etc
#include "BitmapFont.h"
#include "VLog.h"

NV_NAMESPACE_BEGIN

//-------------------------------------------------------------------------------------
// The RHS coordinate system is defines as follows (as seen in perspective view):
//  Y - Up
//  Z - Back
//  X - Right
const VVect3f	UpVector( 0.0f, 1.0f, 0.0f );
const VVect3f	ForwardVector( 0.0f, 0.0f, -1.0f );
const VVect3f	RightVector( 1.0f, 0.0f, 0.0f );

OvrSceneView::OvrSceneView() :
//	FreeWorldModelOnChange( false ),
//	SceneId( 0 ),
	LoadedPrograms( false ),
	MoveSpeed( 3.0f ),
	Znear( 1.0f ),
	Zfar( 1000.0f ),
	ImuToEyeCenter( 0.06f, 0.0f, 0.03f ),
	YawOffset( 0.0f ),
	YawVelocity( 0.0f ),
	AllowPositionTracking( false ),
	LastHeadModelOffset( 0.0f ),
	EyeYaw( 0.0f ),
	EyePitch( 0.0f ),
	EyeRoll( 0.0f ),
	FootPos( 0.0f )
{
}

VMatrix4f OvrSceneView::CenterViewMatrix() const
{
	return ViewMatrix;
}

VMatrix4f OvrSceneView::ViewMatrixForEye( const int eye ) const
{
	const float eyeOffset = ( eye ? -1 : 1 ) * 0.5f * ViewParms.interpupillaryDistance;
    return VMatrix4f::Translation( eyeOffset, 0.0f, 0.0f ) * ViewMatrix;
}

VMatrix4f OvrSceneView::ProjectionMatrixForEye( const int eye, const float fovDegrees ) const
{
	// We may want to make per-eye projection matrices if we move away from
	// nearly-centered lenses.
    return VMatrix4f::PerspectiveRH( VDegreeToRad( fovDegrees ), 1.0f, Znear, Zfar );
}

VMatrix4f OvrSceneView::MvpForEye( const int eye, const float fovDegrees ) const
{
	return ProjectionMatrixForEye( eye, fovDegrees ) * ViewMatrixForEye( eye );
}

VMatrix4f OvrSceneView::DrawEyeView( const int eye, const float fovDegrees ) const
{
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );
	glFrontFace( GL_CCW );

    const VMatrix4f projectionMatrix = ProjectionMatrixForEye( eye, fovDegrees );
    const VMatrix4f viewMatrix = ViewMatrixForEye( eye );

//	const DrawSurfaceList & surfs = BuildDrawSurfaceList( RenderModels, viewMatrix, projectionMatrix );
//	(void)RenderSurfaceList( surfs );

	// TODO: sort the emit surfaces with the model based surfaces
//	if ( EmitList.length() > 0 )
//	{
//		DrawSurfaceList	emits;
//		emits.drawSurfaces = &EmitList[0];
//		emits.numDrawSurfaces = EmitList.length();
//		emits.projectionMatrix = projectionMatrix;
//		emits.viewMatrix = viewMatrix;

//        const VR4Matrixf vpMatrix = ( projectionMatrix * viewMatrix ).Transposed();

//		for ( int i = 0 ; i < EmitList.length() ; i++ )
//		{
//			DrawMatrices & matrices = *(DrawMatrices *)EmitList[i].matrices;
//			matrices.Mvp = matrices.Model * vpMatrix;
//		}

//		(void)RenderSurfaceList( emits );
//	}

	return ( projectionMatrix * viewMatrix );
}


VVect3f OvrSceneView::CenterEyePos() const
{
    return VVect3f( FootPos.x, FootPos.y + ViewParms.eyeHeight, FootPos.z );
}


VVect3f OvrSceneView::HeadModelOffset( float EyeRoll, float EyePitch, float EyeYaw, float HeadModelDepth, float HeadModelHeight )
{
	// head-on-a-stick model
    const VMatrix4f rollPitchYaw = VMatrix4f::RotationY( EyeYaw )
            * VMatrix4f::RotationX( EyePitch )
            * VMatrix4f::RotationZ( EyeRoll );
    VVect3f eyeCenterInHeadFrame( 0.0f, HeadModelHeight, -HeadModelDepth );
    VVect3f lastHeadModelOffset = rollPitchYaw.transform( eyeCenterInHeadFrame );

	lastHeadModelOffset.y -= eyeCenterInHeadFrame.y; // Bring the head back down to original height

	return lastHeadModelOffset;
}

void OvrSceneView::UpdateViewMatrix(const VFrame vrFrame )
{
	// Experiments with position tracking
	const bool	useHeadModel = !AllowPositionTracking ||
			( ( vrFrame.input.buttonState & ( BUTTON_A | BUTTON_X ) ) == 0 );

	// Delta time in seconds since last frame.
	const float dt = vrFrame.deltaSeconds;
    //const float yawSpeed = 1.5f;

    VVect3f GamepadMove;

	// Allow up / down movement if there is no floor collision model
	if ( vrFrame.input.buttonState & BUTTON_RIGHT_TRIGGER )
	{
		FootPos.y -= vrFrame.input.sticks[0][1] * dt * MoveSpeed;
	}
	else
	{
		GamepadMove.z = vrFrame.input.sticks[0][1];
	}
	GamepadMove.x = vrFrame.input.sticks[0][0];

	// Turn based on the look stick
	// Because this can be predicted ahead by async TimeWarp, we apply
	// the yaw from the previous frame's controls, trading a frame of
	// latency on stick controls to avoid a bounce-back.
	YawOffset -= YawVelocity * dt;

    /*if ( !( vrFrame.OvrStatus & Status_OrientationTracked ) )
    {
        PitchOffset -= yawSpeed * vrFrame.Input.sticks[1][1] * dt;
        YawVelocity = yawSpeed * vrFrame.Input.sticks[1][0];
    }
    else
    {
        YawVelocity = 0.0f;
    }*/
    //@to-do: Sensor should output if orientation is tracked
    YawVelocity = 0.0f;

	// We extract Yaw, Pitch, Roll instead of directly using the orientation
	// to allow "additional" yaw manipulation with mouse/controller.
    const VQuatf quat = vrFrame.pose;

    quat.GetEulerAngles<VAxis_Y, VAxis_X, VAxis_Z>( &EyeYaw, &EyePitch, &EyeRoll );

	EyeYaw += YawOffset;

	// If the sensor isn't plugged in, allow right stick up/down
	// to adjust pitch, which can be useful for debugging.  Never
	// do this when head tracking
    /*if ( !( vrFrame.OvrStatus & Status_OrientationTracked ) )
	{
		EyePitch += PitchOffset;
    }*/

	// Perform player movement.
//    vInfo("this is gamepadMove");
//	if ( GamepadMove.LengthSq() > 0.0f )
//	{
//        vInfo("gamepadMove is not empty");
//        const VR4Matrixf yawRotate = VR4Matrixf::RotationY( EyeYaw );
//        const V3Vectf orientationVector = yawRotate.Transform( GamepadMove );

//		// Don't let move get too crazy fast
//		const float moveDistance = std::min<float>( MoveSpeed * (float)dt, 1.0f );
//		if ( WorldModel.Definition )
//		{
//			FootPos = SlideMove( FootPos, ViewParms.eyeHeight, orientationVector, moveDistance,
//						WorldModel.Definition->Collisions, WorldModel.Definition->GroundCollisions );
//		}
//		else
//		{	// no scene loaded, walk without any collisions
//			CollisionModel collisionModel;
//			CollisionModel groundCollisionModel;
//			FootPos = SlideMove( FootPos, ViewParms.eyeHeight, orientationVector, moveDistance,
//						collisionModel, groundCollisionModel );
//		}
//	}

	// Rotate and position View Camera, using YawPitchRoll in BodyFrame coordinates.
    VMatrix4f rollPitchYaw = VMatrix4f::RotationY( EyeYaw )
            * VMatrix4f::RotationX( EyePitch )
            * VMatrix4f::RotationZ( EyeRoll );
    const VVect3f up = rollPitchYaw.transform( UpVector );
    const VVect3f forward = rollPitchYaw.transform( ForwardVector );
    const VVect3f right = rollPitchYaw.transform( RightVector );

	// Have sensorFusion zero the integration when not using it, so the
	// first frame is correct.
	if ( vrFrame.input.buttonPressed & (BUTTON_A | BUTTON_X) )
	{
		LatchedHeadModelOffset = LastHeadModelOffset;
	}

	// Calculate the shiftedEyePos
	ShiftedEyePos = CenterEyePos();

    VVect3f headModelOffset = HeadModelOffset( EyeRoll, EyePitch, EyeYaw,
			ViewParms.headModelDepth, ViewParms.headModelHeight );
	if ( useHeadModel )
	{
		ShiftedEyePos += headModelOffset;
	}

	headModelOffset += forward * ImuToEyeCenter.z;
	headModelOffset += right * ImuToEyeCenter.x;

	LastHeadModelOffset = headModelOffset;

	if ( !useHeadModel )
	{
		// Use position tracking from the sensor system, which is in absolute
		// coordinates without the YawOffset
        //ShiftedEyePos += VR4Matrixf::RotationY( YawOffset ).Transform( vrFrame.PoseState.Position );

		ShiftedEyePos -= forward * ImuToEyeCenter.z;
		ShiftedEyePos -= right * ImuToEyeCenter.x;

		ShiftedEyePos += LatchedHeadModelOffset;
	}

    ViewMatrix = VMatrix4f::LookAtRH( ShiftedEyePos, ShiftedEyePos + forward, up );
}

void OvrSceneView::Frame( const VViewSettings viewParms_, const VFrame vrFrame,
        VMatrix4f & timeWarpParmsExternalVelocity, const long long supressModelsWithClientId )
{
	ViewParms = viewParms_;
	UpdateViewMatrix( vrFrame );
//	UpdateSceneModels( vrFrame, supressModelsWithClientId );

	// External systems can add surfaces to this list before drawing.
//	EmitList.clear();

	// Set the external velocity matrix so TimeWarp can smoothly rotate the
	// view even if we are dropping frames.
    const VMatrix4f localViewMatrix = ViewMatrix;
    timeWarpParmsExternalVelocity = localViewMatrix.calculateExternalVelocity(YawVelocity);
}

NV_NAMESPACE_END
