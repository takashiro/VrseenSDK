#pragma once

#include "VGlShader.h"

NV_USING_NAMESPACE

namespace OculusCinema {

class CinemaApp;

enum sceneProgram_t
{
	SCENE_PROGRAM_BLACK,
	SCENE_PROGRAM_STATIC_ONLY,
	SCENE_PROGRAM_STATIC_DYNAMIC,
	SCENE_PROGRAM_DYNAMIC_ONLY,
	SCENE_PROGRAM_ADDITIVE,
	SCENE_PROGRAM_MAX
};

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
	VGlShader				UniformColorProgram;

	VGlShader				ProgVertexColor;
	VGlShader				ProgSingleTexture;
	VGlShader				ProgLightMapped;
	VGlShader				ProgReflectionMapped;
	VGlShader				ProgSkinnedVertexColor;
	VGlShader				ProgSkinnedSingleTexture;
	VGlShader				ProgSkinnedLightMapped;
	VGlShader				ProgSkinnedReflectionMapped;

	VGlShader				ScenePrograms[ SCENE_PROGRAM_MAX ];
};

} // namespace OculusCinema
