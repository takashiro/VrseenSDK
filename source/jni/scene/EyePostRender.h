#pragma once

#include "vglobal.h"

#include "GlGeometry.h"
#include "../api/VGlShader.h"

NV_NAMESPACE_BEGIN

// This class is for rendering things on top of a complete eye buffer that can
// be common between native code and Unity.

class EyePostRender
{
public:
	void		Init();
	void		Shutdown();

	void		DrawEyeCalibrationLines( const float bufferFovDegrees, const int eye );
	void 		DrawEyeVignette();

	// Draw a single pixel border around the framebuffer using clear rects
	void		FillEdgeColor( int fbWidth, int fbHeight, float r, float g, float b, float a );

	// FillEdgeColor ( 0 0 0 1 )
	void		FillEdge( int fbWidth, int fbHeight );

	VGlShader	UntexturedMvpProgram;
	VGlShader	UntexturedScreenSpaceProgram;

	GlGeometry	CalibrationLines;
	GlGeometry	VignetteSquare;
};

NV_NAMESPACE_END
