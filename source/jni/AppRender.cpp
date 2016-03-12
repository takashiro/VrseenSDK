/************************************************************************************

Filename    :   AppRender.cpp
Content     :
Created     :
Authors     :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/
#include <math.h>
#include <jni.h>

#include <sys/time.h>

#include "Android/GlUtils.h"
#include "api/VrApi.h"
#include "api/VrApi_Helpers.h"

#include "GlSetup.h"
#include "GlTexture.h"
#include "VrCommon.h"
#include "App.h"

#include "AppLocal.h"
#include "BitmapFont.h"
#include "TypesafeNumber.h"
#include "GazeCursor.h"
#include "gui/VRMenuMgr.h"
#include "gui/GuiSys.h"
#include "DebugLines.h"


//#define BASIC_FOLLOW 1

NV_NAMESPACE_BEGIN

// Debug tool to draw outlines of a 3D bounds
void AppLocal::DrawBounds( const Vector3f &mins, const Vector3f &maxs, const Matrix4f &mvp, const Vector3f &color )
{
	Matrix4f	scaled = mvp * Matrix4f::Translation( mins ) * Matrix4f::Scaling( maxs - mins );
	const VGlShader & prog = untexturedMvpProgram;
	glUseProgram(prog.program);
	glLineWidth( 1.0f );
	glUniform4f(prog.uniformColor, color.x, color.y, color.z, 1);
	glUniformMatrix4fv(prog.uniformModelViewProMatrix, 1, GL_FALSE /* not transposed */,
			scaled.Transposed().M[0] );
	glBindVertexArrayOES_( unitCubeLines.vertexArrayObject );
	glDrawElements(GL_LINES, unitCubeLines.indexCount, GL_UNSIGNED_SHORT, NULL);
	glBindVertexArrayOES_( 0 );
}

void AppLocal::DrawDialog( const Matrix4f & mvp )
{
	// draw the pop-up dialog
	const float now = ovr_GetTimeInSeconds();
	if ( now >= dialogStopSeconds )
	{
		return;
	}
	const Matrix4f dialogMvp = mvp * dialogMatrix;

	const float fadeSeconds = 0.5f;
	const float f = now - ( dialogStopSeconds - fadeSeconds );
	const float clampF = f < 0.0f ? 0.0f : f;
	const float alpha = 1.0f - clampF;

	DrawPanel( dialogTexture->textureId, dialogMvp, alpha );
}

void AppLocal::DrawPanel( const GLuint externalTextureId, const Matrix4f & dialogMvp, const float alpha )
{
	const VGlShader & prog = externalTextureProgram2;
	glUseProgram( prog.program );
	glUniform4f(prog.uniformColor, 1, 1, 1, alpha );

	glUniformMatrix4fv(prog.uniformTexMatrix, 1, GL_FALSE, Matrix4f::Identity().Transposed().M[0]);
	glUniformMatrix4fv(prog.uniformModelViewProMatrix, 1, GL_FALSE, dialogMvp.Transposed().M[0] );

	// It is important that panels write to destination alpha, or they
	// might get covered by an overlay plane/cube in TimeWarp.
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, externalTextureId);
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	panelGeometry.Draw();
	glDisable( GL_BLEND );
	glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0 );	// don't leave it bound
}

void AppLocal::DrawEyeViewsPostDistorted( Matrix4f const & centerViewMatrix, const int numPresents )
{
	// update vr lib systems after the app frame, but before rendering anything
	GetGuiSys().frame( this, vrFrame, GetVRMenuMgr(), GetDefaultFont(), GetMenuFontSurface(), centerViewMatrix );
	GetGazeCursor().Frame( centerViewMatrix, vrFrame.DeltaSeconds );

	GetMenuFontSurface().Finish( centerViewMatrix );
	GetWorldFontSurface().Finish( centerViewMatrix );
	GetVRMenuMgr().finish( centerViewMatrix );

	// Increase the fov by about 10 degrees if we are not holding 60 fps so
	// there is less black pull-in at the edges.
	//
	// Doing this dynamically based just on time causes visible flickering at the
	// periphery when the fov is increased, so only do it if minimumVsyncs is set.
	const float fovDegrees = hmdInfo.SuggestedEyeFov[0] +
			( ( ( swapParms.MinimumVsyncs > 1 ) || ovr_GetPowerLevelStateThrottled() ) ? 10.0f : 0.0f ) +
			( ( !showVignette ) ? 5.0f : 0.0f );

	// DisplayMonoMode uses a single eye rendering for speed improvement
	// and / or high refresh rate double-scan hardware modes.
	const int numEyes = renderMonoMode ? 1 : 2;

	// Flush out and report any errors
	GL_CheckErrors("FrameStart");

	if ( drawCalibrationLines && calibrationLinesDrawn )
	{
		// doing a time warp test, don't generate new images
		LOG( "drawCalibrationLines && calibrationLinesDrawn" );
	}
	else
	{
		// possibly change the buffer parameters
		eyeTargets->BeginFrame( vrParms );

		for ( int eye = 0; eye < numEyes; eye++ )
		{
			eyeTargets->BeginRenderingEye( eye );

			// Call back to the app for drawing.
			const Matrix4f mvp = appInterface->DrawEyeView( eye, fovDegrees );

			GetVRMenuMgr().renderSubmitted( mvp.Transposed(), centerViewMatrix );
			GetMenuFontSurface().Render3D( GetDefaultFont(), mvp.Transposed() );
			GetWorldFontSurface().Render3D( GetDefaultFont(), mvp.Transposed() );

			glDisable( GL_DEPTH_TEST );
			glDisable( GL_CULL_FACE );

			// Optionally draw thick calibration lines into the texture,
			// which will be overlayed by the thinner origin cross when
			// distorted to the window.
			if ( drawCalibrationLines )
			{
				eyeDecorations.DrawEyeCalibrationLines( fovDegrees, eye );
				calibrationLinesDrawn = true;
			}
			else
			{
				calibrationLinesDrawn = false;
			}

			DrawDialog( mvp );

			GetGazeCursor().Render( eye, mvp );

			GetDebugLines().Render( mvp.Transposed() );

			if ( showVignette )
			{
				// Draw a thin vignette at the edges of the view so clamping will give black
				// This will not be reflected correctly in overlay planes.
				// EyeDecorations.DrawEyeVignette();

				eyeDecorations.FillEdge( vrParms.resolution, vrParms.resolution );
			}

			eyeTargets->EndRenderingEye( eye );
		}
	}

	// This eye set is complete, use it now.
	if ( numPresents > 0 )
	{
		const CompletedEyes eyes = eyeTargets->GetCompletedEyes();

		for ( int eye = 0; eye < MAX_WARP_EYES; eye++ )
		{
			swapParms.Images[eye][0].TexCoordsFromTanAngles = TanAngleMatrixFromFov( fovDegrees );
			swapParms.Images[eye][0].TexId = eyes.Textures[renderMonoMode ? 0 : eye ];
			swapParms.Images[eye][0].Pose = sensorForNextWarp.Predicted;
		}

		ovr_WarpSwap( OvrMobile, &swapParms );
	}
}

// Draw a screen to an eye buffer the same way it would be drawn as a
// time warp overlay.
void AppLocal::DrawScreenDirect( const GLuint texid, const ovrMatrix4f & mvp )
{
	const Matrix4f mvpMatrix( mvp );
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, texid );

	glUseProgram( overlayScreenDirectProgram.program );

	glUniformMatrix4fv( overlayScreenDirectProgram.uniformModelViewProMatrix, 1, GL_FALSE, mvpMatrix.Transposed().M[0] );

	glBindVertexArrayOES_( unitSquare.vertexArrayObject );
	glDrawElements( GL_TRIANGLES, unitSquare.indexCount, GL_UNSIGNED_SHORT, NULL );

	glBindTexture( GL_TEXTURE_2D, 0 );	// don't leave it bound
}

// draw a zero to destination alpha
void AppLocal::DrawScreenMask( const ovrMatrix4f & mvp, const float fadeFracX, const float fadeFracY )
{
	Matrix4f mvpMatrix( mvp );

	glUseProgram( overlayScreenFadeMaskProgram.program );

	glUniformMatrix4fv( overlayScreenFadeMaskProgram.uniformModelViewProMatrix, 1, GL_FALSE, mvpMatrix.Transposed().M[0] );

	if ( fadedScreenMaskSquare.vertexArrayObject == 0 )
	{
		fadedScreenMaskSquare = BuildFadedScreenMask( fadeFracX, fadeFracY );
	}

	glColorMask( 0.0f, 0.0f, 0.0f, 1.0f );
	fadedScreenMaskSquare.Draw();
	glColorMask( 1.0f, 1.0f, 1.0f, 1.0f );
}

NV_NAMESPACE_END
