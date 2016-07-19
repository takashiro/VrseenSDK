#pragma once

#include "VGlShader.h"

NV_USING_NAMESPACE

class PanoCinema;

class ShaderManager
{
public:
							ShaderManager( PanoCinema &cinema );

	void					OneTimeInit(const VString &launchIntent );
	void					OneTimeShutdown();

	PanoCinema &				Cinema;

	// Render the external image texture to a conventional texture to allow
	// mipmap generation.
	VGlShader				CopyMovieProgram;
	VGlShader				MovieExternalUiProgram;
};
