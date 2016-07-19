#pragma once

#include "VGlShader.h"

NV_USING_NAMESPACE

namespace OculusCinema {

class CinemaApp;

class ShaderManager
{
public:
							ShaderManager( CinemaApp &cinema );

	void					OneTimeInit(const VString &launchIntent );
	void					OneTimeShutdown();

	CinemaApp &				Cinema;

	// Render the external image texture to a conventional texture to allow
	// mipmap generation.
	VGlShader				CopyMovieProgram;
	VGlShader				MovieExternalUiProgram;
};

} // namespace OculusCinema
