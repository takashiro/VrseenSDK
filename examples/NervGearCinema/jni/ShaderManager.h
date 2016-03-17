#pragma once

#include "GlProgram.h"
#include "ModelFile.h"

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
	GlProgram				CopyMovieProgram;
	GlProgram				MovieExternalUiProgram;
	GlProgram				UniformColorProgram;

	GlProgram				ProgVertexColor;
	GlProgram				ProgSingleTexture;
	GlProgram				ProgLightMapped;
	GlProgram				ProgReflectionMapped;
	GlProgram				ProgSkinnedVertexColor;
	GlProgram				ProgSkinnedSingleTexture;
	GlProgram				ProgSkinnedLightMapped;
	GlProgram				ProgSkinnedReflectionMapped;

	GlProgram				ScenePrograms[ SCENE_PROGRAM_MAX ];

	ModelGlPrograms 		DynamicPrograms;
	ModelGlPrograms 		DefaultPrograms;
};

} // namespace OculusCinema
