/************************************************************************************

Filename    :   ModelView.cpp
Content     :   Basic viewing and movement in a scene.
Created     :   December 19, 2013
Authors     :   John Carmack

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "ModelView.h"
#include "VAlgorithm.h"
#include "api/VrApi.h"
#include "api/VrApi_Helpers.h"

#include "Input.h"		// VrFrame, etc
#include "BitmapFont.h"
#include "DebugLines.h"

#include "VLog.h"

NV_NAMESPACE_BEGIN

void ModelInScene::SetModelFile( const ModelFile * mf ) 
{ 
	Definition = mf;
	State.modelDef = mf ? &mf->Def : NULL;
	State.Joints.resize( mf->GetJointCount() );
};

void ModelInScene::AnimateJoints( const float timeInSeconds )
{
	if ( State.Flags.Pause )
	{
		return;
	}

	for ( int i = 0; i < Definition->GetJointCount(); i++ )
	{
		const ModelJoint * joint = Definition->GetJoint( i );
		if ( joint->animation == MODEL_JOINT_ANIMATION_NONE )
		{
			continue;
		}

		float time = ( timeInSeconds + joint->timeOffset ) * joint->timeScale;

		switch( joint->animation )
		{
			case MODEL_JOINT_ANIMATION_SWAY:
			{
                time = sinf( time * VConstants<float>::Pi );
				// NOTE: fall through
			}
			case MODEL_JOINT_ANIMATION_ROTATE:
			{
                const V3Vectf angles = joint->parameters * ( VConstants<float>::VDTR * time );
                const VR4Matrixf matrix = joint->transform *
                                        VR4Matrixf::RotationY( angles.y ) *
                                        VR4Matrixf::RotationX( angles.x ) *
                                        VR4Matrixf::RotationZ( angles.z ) *
										joint->transform.Inverted();
				State.Joints[i] = matrix.Transposed();
				break;
			}
			case MODEL_JOINT_ANIMATION_BOB:
			{
                const float frac = sinf( time * VConstants<float>::Pi );
                const V3Vectf offset = joint->parameters * frac;
                const VR4Matrixf matrix = joint->transform *
                                        VR4Matrixf::Translation( offset ) *
										joint->transform.Inverted();
				State.Joints[i] = matrix.Transposed();
				break;
			}
			case MODEL_JOINT_ANIMATION_NONE:
				break;
		}
	}
}

//-------------------------------------------------------------------------------------
// The RHS coordinate system is defines as follows (as seen in perspective view):
//  Y - Up
//  Z - Back
//  X - Right
const V3Vectf	UpVector( 0.0f, 1.0f, 0.0f );
const V3Vectf	ForwardVector( 0.0f, 0.0f, -1.0f );
const V3Vectf	RightVector( 1.0f, 0.0f, 0.0f );

OvrSceneView::OvrSceneView() :
	FreeWorldModelOnChange( false ),
	SceneId( 0 ),
	LoadedPrograms( false ),
	MoveSpeed( 3.0f ),
	Znear( 1.0f ),
	Zfar( 1000.0f ),
	ImuToEyeCenter( 0.06f, 0.0f, 0.03f ),
	YawOffset( 0.0f ),
	PitchOffset( 0.0f ),
	YawVelocity( 0.0f ),
	AllowPositionTracking( false ),
	LastHeadModelOffset( 0.0f ),
	EyeYaw( 0.0f ),
	EyePitch( 0.0f ),
	EyeRoll( 0.0f ),
	FootPos( 0.0f )
{
}

int OvrSceneView::AddModel( ModelInScene * model )
{
	const int modelsSize = Models.length();

	// scan for a NULL entry
	for ( int i = 0; i < modelsSize; ++i )
	{
		if ( Models[i] == NULL )
		{
			Models[i] = model;
			return i;
		}
	}

	Models.append( model );

	return Models.length() - 1;
}

void OvrSceneView::RemoveModelIndex( int index )
{
	Models[index] = NULL;
}

ModelGlPrograms OvrSceneView::GetDefaultGLPrograms()
{
	ModelGlPrograms programs;

	if ( !LoadedPrograms )
	{
		ProgVertexColor				.initShader( VertexColorVertexShaderSrc, VertexColorFragmentShaderSrc );
		ProgSingleTexture			.initShader( SingleTextureVertexShaderSrc, SingleTextureFragmentShaderSrc );
		ProgLightMapped				.initShader( LightMappedVertexShaderSrc, LightMappedFragmentShaderSrc );
		ProgReflectionMapped		.initShader( ReflectionMappedVertexShaderSrc, ReflectionMappedFragmentShaderSrc );
		ProgSkinnedVertexColor		.initShader( VertexColorSkinned1VertexShaderSrc, VertexColorFragmentShaderSrc );
		ProgSkinnedSingleTexture	.initShader( SingleTextureSkinned1VertexShaderSrc, SingleTextureFragmentShaderSrc );
		ProgSkinnedLightMapped		.initShader( LightMappedSkinned1VertexShaderSrc, LightMappedFragmentShaderSrc );
		ProgSkinnedReflectionMapped	.initShader( ReflectionMappedSkinned1VertexShaderSrc, ReflectionMappedFragmentShaderSrc );
		LoadedPrograms = true;
	}

	programs.ProgVertexColor				= & ProgVertexColor;
	programs.ProgSingleTexture				= & ProgSingleTexture;
	programs.ProgLightMapped				= & ProgLightMapped;
	programs.ProgReflectionMapped			= & ProgReflectionMapped;
	programs.ProgSkinnedVertexColor			= & ProgSkinnedVertexColor;
	programs.ProgSkinnedSingleTexture		= & ProgSkinnedSingleTexture;
	programs.ProgSkinnedLightMapped			= & ProgSkinnedLightMapped;
	programs.ProgSkinnedReflectionMapped	= & ProgSkinnedReflectionMapped;

	return programs;
}

void OvrSceneView::LoadWorldModel(const VString &sceneFileName, const MaterialParms & materialParms )
{
    vInfo("OvrSceneView::LoadScene(" << sceneFileName << ")");

	if ( GlPrograms.ProgSingleTexture == NULL )
	{
		GlPrograms = GetDefaultGLPrograms();
	}

	// Load the scene we are going to draw
    ModelFile * model = LoadModelFile( sceneFileName.toCString(), GlPrograms, materialParms );

	SetWorldModel( *model );

	FreeWorldModelOnChange = true;
}

void OvrSceneView::SetWorldModel( ModelFile & world )
{
    vInfo("OvrSceneView::SetWorldModel(" << world.FileName << ")");

	if ( FreeWorldModelOnChange && Models.length() > 0 )
	{
		delete WorldModel.Definition;
		FreeWorldModelOnChange = false;
	}
	Models.clear();

	WorldModel.SetModelFile( &world );
	AddModel( &WorldModel );

	// Projection matrix
	Znear = 0.01f;
	Zfar = 2000.0f;

	// Set the initial player position
    FootPos = V3Vectf( 0.0f, 0.0f, 0.0f );
	YawOffset = 0;

    LastHeadModelOffset = V3Vectf( 0.0f, 0.0f, 0.0f );
}

SurfaceDef * OvrSceneView::FindNamedSurface( const char * name ) const
{
	return ( WorldModel.Definition == NULL ) ? NULL : WorldModel.Definition->FindNamedSurface( name );
}

const ModelTexture * OvrSceneView::FindNamedTexture( const char * name ) const
{
	return ( WorldModel.Definition == NULL ) ? NULL : WorldModel.Definition->FindNamedTexture( name );
}

const ModelTag * OvrSceneView::FindNamedTag(const VString &name) const
{
    return ( WorldModel.Definition == NULL ) ? NULL : WorldModel.Definition->FindNamedTag(name);
}

VBoxf OvrSceneView::GetBounds() const
{
	return ( WorldModel.Definition == NULL ) ?
            VBoxf( V3Vectf( 0, 0, 0 ), V3Vectf( 0, 0, 0 ) ) :
			WorldModel.Definition->GetBounds();
}

VR4Matrixf OvrSceneView::CenterViewMatrix() const
{
	return ViewMatrix;
}

VR4Matrixf OvrSceneView::ViewMatrixForEye( const int eye ) const
{
	const float eyeOffset = ( eye ? -1 : 1 ) * 0.5f * ViewParms.InterpupillaryDistance;
    return VR4Matrixf::Translation( eyeOffset, 0.0f, 0.0f ) * ViewMatrix;
}

VR4Matrixf OvrSceneView::ProjectionMatrixForEye( const int eye, const float fovDegrees ) const
{
	// We may want to make per-eye projection matrices if we move away from
	// nearly-centered lenses.
    return VR4Matrixf::PerspectiveRH( VDegreeToRad( fovDegrees ), 1.0f, Znear, Zfar );
}

VR4Matrixf OvrSceneView::MvpForEye( const int eye, const float fovDegrees ) const
{
	return ProjectionMatrixForEye( eye, fovDegrees ) * ViewMatrixForEye( eye );
}

VR4Matrixf OvrSceneView::DrawEyeView( const int eye, const float fovDegrees ) const
{
	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );
	glFrontFace( GL_CCW );

    const VR4Matrixf projectionMatrix = ProjectionMatrixForEye( eye, fovDegrees );
    const VR4Matrixf viewMatrix = ViewMatrixForEye( eye );

	const DrawSurfaceList & surfs = BuildDrawSurfaceList( RenderModels, viewMatrix, projectionMatrix );
	(void)RenderSurfaceList( surfs );

	// TODO: sort the emit surfaces with the model based surfaces
	if ( EmitList.length() > 0 )
	{
		DrawSurfaceList	emits;
		emits.drawSurfaces = &EmitList[0];
		emits.numDrawSurfaces = EmitList.length();
		emits.projectionMatrix = projectionMatrix;
		emits.viewMatrix = viewMatrix;

        const VR4Matrixf vpMatrix = ( projectionMatrix * viewMatrix ).Transposed();

		for ( int i = 0 ; i < EmitList.length() ; i++ )
		{
			DrawMatrices & matrices = *(DrawMatrices *)EmitList[i].matrices;
			matrices.Mvp = matrices.Model * vpMatrix;
		}

		(void)RenderSurfaceList( emits );
	}

	return ( projectionMatrix * viewMatrix );
}

V3Vectf OvrSceneView::Forward() const
{
    return V3Vectf( -ViewMatrix.M[2][0], -ViewMatrix.M[2][1], -ViewMatrix.M[2][2] );
}

V3Vectf OvrSceneView::CenterEyePos() const
{
    return V3Vectf( FootPos.x, FootPos.y + ViewParms.EyeHeight, FootPos.z );
}

V3Vectf OvrSceneView::ShiftedCenterEyePos() const
{
	return ShiftedEyePos;
}

V3Vectf OvrSceneView::HeadModelOffset( float EyeRoll, float EyePitch, float EyeYaw, float HeadModelDepth, float HeadModelHeight )
{
	// head-on-a-stick model
    const VR4Matrixf rollPitchYaw = VR4Matrixf::RotationY( EyeYaw )
            * VR4Matrixf::RotationX( EyePitch )
            * VR4Matrixf::RotationZ( EyeRoll );
    V3Vectf eyeCenterInHeadFrame( 0.0f, HeadModelHeight, -HeadModelDepth );
    V3Vectf lastHeadModelOffset = rollPitchYaw.Transform( eyeCenterInHeadFrame );

	lastHeadModelOffset.y -= eyeCenterInHeadFrame.y; // Bring the head back down to original height

	return lastHeadModelOffset;
}

void OvrSceneView::UpdateViewMatrix(const VrFrame vrFrame )
{
	// Experiments with position tracking
	const bool	useHeadModel = !AllowPositionTracking ||
			( ( vrFrame.Input.buttonState & ( BUTTON_A | BUTTON_X ) ) == 0 );

	// Delta time in seconds since last frame.
	const float dt = vrFrame.DeltaSeconds;
	const float yawSpeed = 1.5f;

    V3Vectf GamepadMove;

	// Allow up / down movement if there is no floor collision model
	if ( vrFrame.Input.buttonState & BUTTON_RIGHT_TRIGGER )
	{
		FootPos.y -= vrFrame.Input.sticks[0][1] * dt * MoveSpeed;
	}
	else
	{
		GamepadMove.z = vrFrame.Input.sticks[0][1];
	}
	GamepadMove.x = vrFrame.Input.sticks[0][0];

	// Turn based on the look stick
	// Because this can be predicted ahead by async TimeWarp, we apply
	// the yaw from the previous frame's controls, trading a frame of
	// latency on stick controls to avoid a bounce-back.
	YawOffset -= YawVelocity * dt;

	if ( !( vrFrame.OvrStatus & ovrStatus_OrientationTracked ) )
	{
		PitchOffset -= yawSpeed * vrFrame.Input.sticks[1][1] * dt;
		YawVelocity = yawSpeed * vrFrame.Input.sticks[1][0];
	}
	else
	{
		YawVelocity = 0.0f;
	}

	// We extract Yaw, Pitch, Roll instead of directly using the orientation
	// to allow "additional" yaw manipulation with mouse/controller.
    const VQuatf quat = vrFrame.PoseState.Pose.Orientation;

    quat.GetEulerAngles<VAxis_Y, VAxis_X, VAxis_Z>( &EyeYaw, &EyePitch, &EyeRoll );

	EyeYaw += YawOffset;

	// If the sensor isn't plugged in, allow right stick up/down
	// to adjust pitch, which can be useful for debugging.  Never
	// do this when head tracking
	if ( !( vrFrame.OvrStatus & ovrStatus_OrientationTracked ) )
	{
		EyePitch += PitchOffset;
	}

	// Perform player movement.
	if ( GamepadMove.LengthSq() > 0.0f )
	{
        const VR4Matrixf yawRotate = VR4Matrixf::RotationY( EyeYaw );
        const V3Vectf orientationVector = yawRotate.Transform( GamepadMove );

		// Don't let move get too crazy fast
		const float moveDistance = std::min<float>( MoveSpeed * (float)dt, 1.0f );
		if ( WorldModel.Definition )
		{
			FootPos = SlideMove( FootPos, ViewParms.EyeHeight, orientationVector, moveDistance,
						WorldModel.Definition->Collisions, WorldModel.Definition->GroundCollisions );
		}
		else
		{	// no scene loaded, walk without any collisions
			CollisionModel collisionModel;
			CollisionModel groundCollisionModel;
			FootPos = SlideMove( FootPos, ViewParms.EyeHeight, orientationVector, moveDistance,
						collisionModel, groundCollisionModel );
		}
	}

	// Rotate and position View Camera, using YawPitchRoll in BodyFrame coordinates.
    VR4Matrixf rollPitchYaw = VR4Matrixf::RotationY( EyeYaw )
            * VR4Matrixf::RotationX( EyePitch )
            * VR4Matrixf::RotationZ( EyeRoll );
    const V3Vectf up = rollPitchYaw.Transform( UpVector );
    const V3Vectf forward = rollPitchYaw.Transform( ForwardVector );
    const V3Vectf right = rollPitchYaw.Transform( RightVector );

	// Have sensorFusion zero the integration when not using it, so the
	// first frame is correct.
	if ( vrFrame.Input.buttonPressed & (BUTTON_A | BUTTON_X) )
	{
		LatchedHeadModelOffset = LastHeadModelOffset;
	}

	// Calculate the shiftedEyePos
	ShiftedEyePos = CenterEyePos();

    V3Vectf headModelOffset = HeadModelOffset( EyeRoll, EyePitch, EyeYaw,
			ViewParms.HeadModelDepth, ViewParms.HeadModelHeight );
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
        ShiftedEyePos += VR4Matrixf::RotationY( YawOffset ).Transform( vrFrame.PoseState.Pose.Position );

		ShiftedEyePos -= forward * ImuToEyeCenter.z;
		ShiftedEyePos -= right * ImuToEyeCenter.x;

		ShiftedEyePos += LatchedHeadModelOffset;
	}

    ViewMatrix = VR4Matrixf::LookAtRH( ShiftedEyePos, ShiftedEyePos + forward, up );
}

void OvrSceneView::UpdateSceneModels( const VrFrame vrFrame, const long long supressModelsWithClientId  )
{
	// Build the packed array of ModelState to pass to the renderer for both eyes
	RenderModels.resize( 0 );

	for ( int i = 0; i < Models.length(); ++i )
	{
		if ( Models[i] != NULL && Models[i]->DontRenderForClientUid != supressModelsWithClientId )
		{
			Models[i]->AnimateJoints( vrFrame.PoseState.TimeInSeconds );
			RenderModels.append( Models[i]->State );
		}
	}
}

void OvrSceneView::Frame( const VrViewParms viewParms_, const VrFrame vrFrame,
        ovrMatrix4f & timeWarpParmsExternalVelocity, const long long supressModelsWithClientId )
{
	ViewParms = viewParms_;
	UpdateViewMatrix( vrFrame );
	UpdateSceneModels( vrFrame, supressModelsWithClientId );

	// External systems can add surfaces to this list before drawing.
	EmitList.clear();

	// Set the external velocity matrix so TimeWarp can smoothly rotate the
	// view even if we are dropping frames.
    const ovrMatrix4f localViewMatrix = ViewMatrix;
	timeWarpParmsExternalVelocity = CalculateExternalVelocity( &localViewMatrix, YawVelocity );
}

NV_NAMESPACE_END
