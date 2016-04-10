/************************************************************************************

Filename    :   EyePostRender.cpp
Content     :   Render on top of an eye render, portable between native and Unity.
Created     :   May 23, 2014
Authors     :   John Carmack

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "EyePostRender.h"
#include "VBasicmath.h"
#include "VAlgorithm.h"
#include "VArray.h"
#include "api/VEglDriver.h"

NV_NAMESPACE_BEGIN

void EyePostRender::Init()
{
	vInfo("EyePostRender::Init()");

	// grid of lines for drawing to eye buffer
    CalibrationLines.createCalibrationGrid( 24, false );

	// thin border around the outside
    VignetteSquare.createStylePattern( 128.0f / 1024.0f, 128.0f / 1024.0f );

    UntexturedMvpProgram.initShader(VGlShader::getUntextureMvpVertexShaderSource(),VGlShader::getUntexturedFragmentShaderSource());

    UntexturedScreenSpaceProgram.initShader( VGlShader::getUniformColorVertexShaderSource(), VGlShader::getUntexturedFragmentShaderSource() );
}

void EyePostRender::Shutdown()
{
	vInfo("EyePostRender::Shutdown()");
	CalibrationLines.destroy();
	VignetteSquare.destroy();
	UntexturedMvpProgram.destroy();
	UntexturedScreenSpaceProgram.destroy();
}

void EyePostRender::DrawEyeCalibrationLines( const float bufferFovDegrees, const int eye )
{
    VEglDriver glOperation;
	// Optionally draw thick calibration lines into the texture,
	// which will be overlayed by the thinner pre-distorted lines
	// later -- they should match very closely!
    const VR4Matrixf projectionMatrix =
    //VR4Matrixf::Identity();
     VR4Matrixf::PerspectiveRH( VDegreeToRad( bufferFovDegrees ), 1.0f, 0.01f, 2000.0f );

	const VGlShader & prog = UntexturedMvpProgram;
	glUseProgram( prog.program );
	glLineWidth( 3.0f );
	glUniform4f( prog.uniformColor, 0, 1-eye, eye, 1 );
    glUniformMatrix4fv( prog.uniformModelViewProMatrix, 1, GL_FALSE /* not transposed */,
			projectionMatrix.Transposed().M[0] );

    glOperation.glBindVertexArrayOES( CalibrationLines.vertexArrayObject );

	glDrawElements( GL_LINES, CalibrationLines.indexCount, GL_UNSIGNED_SHORT,
		NULL);
    glOperation.glBindVertexArrayOES( 0 );
}

void EyePostRender::DrawEyeVignette()
{
    VEglDriver glOperation;
	// Draw a thin vignette at the edges of the view so clamping will give black
	glUseProgram( UntexturedScreenSpaceProgram.program);
	glUniform4f( UntexturedScreenSpaceProgram.uniformColor, 1, 1, 1, 1 );
	glEnable( GL_BLEND );
	glBlendFunc( GL_ZERO, GL_SRC_COLOR );
	VignetteSquare.drawElements();
	glDisable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glOperation.glBindVertexArrayOES( 0 );
}

void EyePostRender::FillEdge( int fbWidth, int fbHeight )
{
	FillEdgeColor( fbWidth, fbHeight, 0.0f, 0.0f, 0.0f, 1.0f );
}

void EyePostRender::FillEdgeColor( int fbWidth, int fbHeight, float r, float g, float b, float a )
{

	glClearColor( r, g, b, a );
	glEnable( GL_SCISSOR_TEST );

	glScissor( 0, 0, fbWidth, 1 );
	glClear( GL_COLOR_BUFFER_BIT );

	glScissor( 0, fbHeight-1, fbWidth, 1 );
	glClear( GL_COLOR_BUFFER_BIT );

	glScissor( 0, 0, 1, fbHeight );
	glClear( GL_COLOR_BUFFER_BIT );

	glScissor( fbWidth-1, 0, 1, fbHeight );
	glClear( GL_COLOR_BUFFER_BIT );

	glScissor( 0, 0, fbWidth, fbHeight );
	glDisable( GL_SCISSOR_TEST );
}

NV_NAMESPACE_END
