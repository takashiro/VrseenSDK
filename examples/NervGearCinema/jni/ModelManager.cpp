/************************************************************************************

Filename    :   ModelManager.cpp
Content     :
Created     :	7/3/2014
Authors     :   Jim Dos�

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include <dirent.h>
#include "String_Utils.h"
#include "ModelManager.h"
#include "CinemaApp.h"
#include "PackageFiles.h"

#include <VPath.h>
#include <VApkFile.h>

namespace OculusCinema {

static const char * TheatersDirectory = "Oculus/Cinema/Theaters";

//=======================================================================================

ModelManager::ModelManager( CinemaApp &cinema ) :
	Cinema( cinema ),
	Theaters(),
	BoxOffice( NULL ),
	VoidScene( NULL ),
	LaunchIntent(),
	DefaultSceneModel( NULL )

{
}

ModelManager::~ModelManager()
{
}

void ModelManager::OneTimeInit( const char * launchIntent )
{
	LOG( "ModelManager::OneTimeInit" );
	const double start = ovr_GetTimeInSeconds();
	LaunchIntent = launchIntent;

	DefaultSceneModel = new ModelFile( "default" );

	LoadModels();

    LOG( "ModelManager::OneTimeInit: %i theaters loaded, %3.1f seconds", Theaters.sizeInt(), ovr_GetTimeInSeconds() - start );
}

void ModelManager::OneTimeShutdown()
{
	LOG( "ModelManager::OneTimeShutdown" );

	// Free GL resources

    for( uint i = 0; i < Theaters.size(); i++ )
	{
		delete Theaters[ i ];
	}
}

void ModelManager::LoadModels()
{
	LOG( "ModelManager::LoadModels" );
	const double start = ovr_GetTimeInSeconds();

	BoxOffice = LoadScene( "assets/scenes/BoxOffice.ovrscene", false, true, true );
	BoxOffice->UseSeats = false;
	BoxOffice->LobbyScreen = true;

    if ( LaunchIntent.length() > 0 )
	{
        Theaters.append( LoadScene( LaunchIntent.toCString(), true, true, false ) );
	}
	else
	{
		// we want our theaters to show up first
        Theaters.append( LoadScene( "assets/scenes/home_theater.ovrscene", true, false, true ) );

		// create void scene
		VoidScene = new SceneDef();
		VoidScene->SceneModel = new ModelFile( "Void" );
		VoidScene->UseSeats = false;
		VoidScene->UseDynamicProgram = false;
		VoidScene->UseScreenGeometry = false;
		VoidScene->UseFreeScreen = true;

		int width = 0, height = 0;
		VoidScene->IconTexture = LoadTextureFromApplicationPackage( "assets/VoidTheater.png",
				TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), width, height );

		BuildTextureMipmaps( VoidScene->IconTexture );
		MakeTextureTrilinear( VoidScene->IconTexture );
		MakeTextureClamped( VoidScene->IconTexture );

        Theaters.append( VoidScene );

		// load all scenes on startup, so there isn't a delay when switching theaters
		ScanDirectoryForScenes( Cinema.externalRetailDir( TheatersDirectory ), true, false, Theaters );
		ScanDirectoryForScenes( Cinema.retailDir( TheatersDirectory ), true, false, Theaters );
		ScanDirectoryForScenes( Cinema.sdcardDir( TheatersDirectory ), true, false, Theaters );
	}

    LOG( "ModelManager::LoadModels: %i theaters loaded, %3.1f seconds", Theaters.sizeInt(), ovr_GetTimeInSeconds() - start );
}

void ModelManager::ScanDirectoryForScenes(const VString &directory, bool useDynamicProgram, bool useScreenGeometry, Array<SceneDef *> &scenes ) const
{
    DIR * dir = opendir( directory.toCString() );
	if ( dir != NULL )
	{
		struct dirent * entry;
		while( ( entry = readdir( dir ) ) != NULL ) {
			VString filename = entry->d_name;
            VString ext = VPath(filename).extension().toLower();
            if ( ( ext == "ovrscene" ) )
			{
				VString fullpath = directory;
                fullpath.append( "/" );
                fullpath.append( filename );
                SceneDef *def = LoadScene( fullpath.toCString(), useDynamicProgram, useScreenGeometry, false );
                scenes.append( def );
			}
		}

		closedir( dir );
	}
}

SceneDef * ModelManager::LoadScene( const char *sceneFilename, bool useDynamicProgram, bool useScreenGeometry, bool loadFromApplicationPackage ) const
{
	VString filename;

    if (loadFromApplicationPackage) {
        const VApkFile &apk = VApkFile::CurrentApkFile();
        if (!apk.contains(sceneFilename)) {
            LOG( "Scene %s not found in application package.  Checking sdcard.", sceneFilename);
            loadFromApplicationPackage = false;
        }
	}

	if ( loadFromApplicationPackage )
	{
		filename = sceneFilename;
	}
	else if ( ( sceneFilename != NULL ) && ( *sceneFilename == '/' ) ) 	// intent will have full path for scene file, so check for /
	{
		filename = sceneFilename;
	}
	else if ( Cinema.fileExists( Cinema.externalRetailDir( sceneFilename ) ) )
	{
		filename = Cinema.externalRetailDir( sceneFilename );
	}
	else if ( Cinema.fileExists( Cinema.retailDir( sceneFilename ) ) )
	{
		filename = Cinema.retailDir( sceneFilename );
	}
	else
	{
		filename = Cinema.sdcardDir( sceneFilename );
	}

    LOG( "Adding scene: %s, %s", filename.toCString(), sceneFilename );

	SceneDef *def = new SceneDef();
	def->Filename = sceneFilename;
	def->UseSeats = true;
	def->UseDynamicProgram = useDynamicProgram;

	MaterialParms materialParms;

	// This may be called during init, before the FramebufferIsSrgb is set,
	// so use WantsSrgbFramebuffer instead.
    materialParms.UseSrgbTextureFormats = Cinema.app->GetAppInterface()->wantSrgbFramebuffer();

	// Improve the texture quality with anisotropic filtering.
	materialParms.EnableDiffuseAniso = true;

	// The emissive texture is used as a separate lighting texture and should not be LOD clamped.
	materialParms.EnableEmissiveLodClamp = false;

	ModelGlPrograms glPrograms = ( useDynamicProgram ) ? Cinema.shaderMgr.DynamicPrograms : Cinema.shaderMgr.DefaultPrograms;

    VPath iconFilename = filename;
    iconFilename.setExtension("png");

	int textureWidth = 0, textureHeight = 0;


    VByteArray fileName = filename.toUtf8();
    VByteArray iconFileName = iconFilename.toUtf8();
	if ( loadFromApplicationPackage )
    {
        const VApkFile &apk = VApkFile::CurrentApkFile();
        void *buffer = nullptr;
        uint length = 0;
        apk.read(filename, buffer, length);
        def->SceneModel = LoadModelFileFromMemory(fileName.data(), buffer, length, glPrograms, materialParms);
        free(buffer);
        buffer = nullptr;
        length = 0;
        apk.read(iconFilename, buffer, length);
        def->IconTexture = LoadTextureFromBuffer(iconFileName.data(), MemBuffer(buffer, bufferLength), TextureFlags_t(TEXTUREFLAG_NO_DEFAULT), textureWidth, textureHeight);
    } else {
        def->SceneModel = LoadModelFile(fileName.data(), glPrograms, materialParms );
        def->IconTexture = LoadTextureFromBuffer(iconFileName.data(), MemBufferFile(iconFileName.data()), TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), textureWidth, textureHeight );
	}

	if ( def->IconTexture != 0 )
	{
        LOG( "Loaded external icon for theater: %s", iconFilename.toCString() );
	}
	else
	{
		const ModelTexture * iconTexture = def->SceneModel->FindNamedTexture( "icon" );
		if ( iconTexture != NULL )
		{
			def->IconTexture = iconTexture->texid;
		}
		else
		{
			LOG( "No icon in scene.  Loading default." );

			int	width = 0, height = 0;
			def->IconTexture = LoadTextureFromApplicationPackage( "assets/noimage.png",
				TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), width, height );
		}
	}

	BuildTextureMipmaps( def->IconTexture );
	MakeTextureTrilinear( def->IconTexture );
	MakeTextureClamped( def->IconTexture );

	def->UseScreenGeometry = useScreenGeometry;
	def->UseFreeScreen = false;

	return def;
}

const SceneDef & ModelManager::GetTheater( uint index ) const
{
    if ( index < Theaters.size() )
	{
		return *Theaters[ index ];
	}

	// default to the Void Scene
	return *VoidScene;
}

/*
 * Command
 *
 * Actions that need to be performed on the render thread.
 */
bool ModelManager::Command( const char * msg )
{
	return false;
}

} // namespace OculusCinema
