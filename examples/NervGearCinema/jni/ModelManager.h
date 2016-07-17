#pragma once

#include "ModelFile.h"

#include <VArray.h>
#include <VEvent.h>
#include <VString.h>
#include <VTexture.h>

NV_USING_NAMESPACE

namespace OculusCinema {

class CinemaApp;

class SceneDef
{
public:
						SceneDef() :
                            SceneModel( NULL ),
							UseScreenGeometry( false ),
							LobbyScreen( false ),
							UseFreeScreen( false ),
							UseSeats( false ),
							UseDynamicProgram( false ),
							Loaded( false ) { }

	ModelFile *			SceneModel;
	VString				Filename;
    VTexture IconTexture;
	bool				UseScreenGeometry;	// set to true to draw using the screen geoemetry (for curved screens)
	bool				LobbyScreen;
	bool				UseFreeScreen;
	bool 				UseSeats;
	bool 				UseDynamicProgram;
	bool				Loaded;
};

class ModelManager
{
public:
						ModelManager( CinemaApp &cinema );
						~ModelManager();

	void				OneTimeInit(const VString &launchIntent );
	void				OneTimeShutdown();

	bool 				Command(const NervGear::VEvent & );

    uint				GetTheaterCount() const { return Theaters.size(); }
	const SceneDef & 	GetTheater( uint index ) const;

public:
	CinemaApp &			Cinema;

	VArray<SceneDef *>	Theaters;
	SceneDef *			BoxOffice;
	SceneDef *			VoidScene;

	VString				LaunchIntent;

	ModelFile *			DefaultSceneModel;

private:
	void 				LoadModels();
    void 				ScanDirectoryForScenes(const VString &directory, bool useDynamicProgram, bool useScreenGeometry, VArray<SceneDef *> &scenes ) const;
	SceneDef *			LoadScene(const VString &filename, bool useDynamicProgram, bool useScreenGeometry, bool loadFromApplicationPackage ) const;
};

} // namespace OculusCinema
