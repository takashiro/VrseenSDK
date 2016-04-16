#include <dirent.h>
#include "ModelManager.h"
#include "CinemaApp.h"
#include "core/VTimer.h"
#include <VPath.h>
#include <fstream>
#include <VApkFile.h>
#include "VImageManager.h"
#include "VOpenGLTexture.h"

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

void ModelManager::OneTimeInit(const VString &launchIntent)
{
	vInfo("ModelManager::OneTimeInit");
    const double start = VTimer::Seconds();
	LaunchIntent = launchIntent;

	DefaultSceneModel = new ModelFile( "default" );

	LoadModels();

    vInfo("ModelManager::OneTimeInit:" << Theaters.length() << "theaters loaded," << (VTimer::Seconds() - start) << "seconds");
}

void ModelManager::OneTimeShutdown()
{
	vInfo("ModelManager::OneTimeShutdown");

	// Free GL resources

    for( uint i = 0; i < Theaters.size(); i++ )
	{
		delete Theaters[ i ];
	}
}

void ModelManager::LoadModels()
{
	vInfo("ModelManager::LoadModels");
    const double start = VTimer::Seconds();

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

    vInfo("ModelManager::LoadModels:" << Theaters.length() << "theaters loaded," << (VTimer::Seconds() - start) << "seconds");
}

void ModelManager::ScanDirectoryForScenes(const VString &directory, bool useDynamicProgram, bool useScreenGeometry, VArray<SceneDef *> &scenes ) const
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
            vInfo("Scene" << sceneFilename << "not found in application package.  Checking sdcard.");
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

    vInfo("Adding scene:" << filename << "," << sceneFilename);

	SceneDef *def = new SceneDef();
	def->Filename = sceneFilename;
	def->UseSeats = true;
	def->UseDynamicProgram = useDynamicProgram;

	MaterialParms materialParms;

	// This may be called during init, before the FramebufferIsSrgb is set,
	// so use WantsSrgbFramebuffer instead.
    materialParms.UseSrgbTextureFormats = vApp->appInterface()->wantSrgbFramebuffer();

	// Improve the texture quality with anisotropic filtering.
	materialParms.EnableDiffuseAniso = true;

	// The emissive texture is used as a separate lighting texture and should not be LOD clamped.
	materialParms.EnableEmissiveLodClamp = false;

	ModelGlPrograms glPrograms = ( useDynamicProgram ) ? Cinema.shaderMgr.DynamicPrograms : Cinema.shaderMgr.DefaultPrograms;

    VPath iconFilename = filename;
    iconFilename.setExtension("png");

	//int textureWidth = 0, textureHeight = 0;


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


        VImageManager *imagemanager = new VImageManager();
        VImage *iconimage = imagemanager->loadImage(iconFilename);
        delete imagemanager;
        def->IconTexture = VOpenGLTexture(iconimage, iconFilename,TextureFlags(_NO_DEFAULT)).getTextureName();
    } else {
        def->SceneModel = LoadModelFile(fileName.data(), glPrograms, materialParms );

        VImageManager *imagemanager = new VImageManager();
        VImage *iconimage = imagemanager->loadImage(iconFilename);
        delete imagemanager;
        def->IconTexture = VOpenGLTexture(iconimage, iconFilename,TextureFlags(_NO_DEFAULT)).getTextureName();
    }

	if ( def->IconTexture != 0 )
	{
        vInfo("Loaded external icon for theater:" << iconFilename);
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
			vInfo("No icon in scene.  Loading default.");

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
bool ModelManager::Command(const VEvent &)
{
	return false;
}

} // namespace OculusCinema
