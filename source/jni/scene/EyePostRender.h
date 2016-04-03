#pragma once

#include "vglobal.h"

#include "../api/VGlGeometry.h"
#include "../api/VGlShader.h"

NV_NAMESPACE_BEGIN

class EyePostRender
{
public:
	void		Init();
    void		Shutdown();
	void		DrawEyeCalibrationLines( const float bufferFovDegrees, const int eye );
	void 		DrawEyeVignette();
    void		FillEdgeColor( int fbWidth, int fbHeight, float r, float g, float b, float a );
	void		FillEdge( int fbWidth, int fbHeight );
	VGlShader	UntexturedMvpProgram;
	VGlShader	UntexturedScreenSpaceProgram;
	VGlGeometry	CalibrationLines;
	VGlGeometry	VignetteSquare;
};

NV_NAMESPACE_END
