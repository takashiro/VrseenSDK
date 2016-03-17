/************************************************************************************

Filename    :   ModelManager.h
Content     :
Created     :	7/3/2014
Authors     :   Jim Dos�

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#if !defined( ModelManager_h )
#define ModelManager_h

#include "ModelFile.h"
#include "VString.h"
#include "VArray.h"

NV_USING_NAMESPACE

namespace OculusCinema {

class CinemaApp;

class SceneDef
{
public:
						SceneDef() :
							SceneModel( NULL ),
							Filename(),
							IconTexture( 0 ),
							UseScreenGeometry( false ),
							LobbyScreen( false ),
							UseFreeScreen( false ),
							UseSeats( false ),
							UseDynamicProgram( false ),
							Loaded( false ) { }

	ModelFile *			SceneModel;
	VString				Filename;
	GLuint				IconTexture;
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

	void				OneTimeInit( const char * launchIntent );
	void				OneTimeShutdown();

	bool 				Command( const char * msg );

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
	SceneDef *			LoadScene( const char *filename, bool useDynamicProgram, bool useScreenGeometry, bool loadFromApplicationPackage ) const;
};

} // namespace OculusCinema

#endif // ModelManager_h
