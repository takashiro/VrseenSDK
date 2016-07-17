#include "ModelManager.h"
#include "CinemaApp.h"

#include <dirent.h>
#include <fstream>

#include <VTimer.h>
#include <VPath.h>
#include <VZipFile.h>
#include <VDir.h>
#include <VFile.h>
#include <VResource.h>
#include <VTexture.h>

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
        Theaters.append( LoadScene(LaunchIntent, true, true, false ) );
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
        VoidScene->IconTexture.load(VResource("assets/VoidTheater.png"));

        VoidScene->IconTexture.buildMipmaps();
        VoidScene->IconTexture.trilinear();
        VoidScene->IconTexture.clamp();

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
    VDir dir(directory);
    VArray<VString> entryList = dir.entryList();
    for (const VString &fileName : entryList) {
        VString ext = VPath(fileName).extension().toLower();
        if (ext == "ovrscene") {
            VString fullpath = directory + u'/' + fileName;
            SceneDef *def = LoadScene(fullpath, useDynamicProgram, useScreenGeometry, false);
            scenes.append(def);
        }
    }
}

SceneDef * ModelManager::LoadScene(const VString &sceneFilename, bool useDynamicProgram, bool useScreenGeometry, bool loadFromApplicationPackage ) const
{
    VString fileName;

    if (loadFromApplicationPackage) {
        const VZipFile &apk = vApp->apkFile();
        if (!apk.contains(sceneFilename)) {
            vInfo("Scene" << sceneFilename << "not found in application package.  Checking sdcard.");
            loadFromApplicationPackage = false;
        }
	}

	if ( loadFromApplicationPackage )
	{
        fileName = sceneFilename;
	}
    else if ( ( sceneFilename != NULL ) && (sceneFilename[0] == '/' ) ) 	// intent will have full path for scene file, so check for /
	{
        fileName = sceneFilename;
	}
    else if (VFile::Exists(Cinema.externalRetailDir(sceneFilename)))
	{
        fileName = Cinema.externalRetailDir(sceneFilename);
	}
    else if (VFile::Exists(Cinema.retailDir(sceneFilename)))
	{
        fileName = Cinema.retailDir( sceneFilename );
	}
	else
	{
        fileName = Cinema.sdcardDir( sceneFilename );
	}

    vInfo("Adding scene:" << fileName << "," << sceneFilename);

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

    VPath iconFileName = fileName;
    iconFileName.setExtension("png");

    if (loadFromApplicationPackage) {
        const VZipFile &apk = vApp->apkFile();
        void *buffer = nullptr;
        uint length = 0;
        apk.read(fileName, buffer, length);
        def->SceneModel = LoadModelFileFromMemory(fileName.toUtf8().data(), buffer, length, glPrograms, materialParms);
        free(buffer);
        buffer = nullptr;
        length = 0;

        VFile icon(iconFileName, VFile::ReadOnly);
        if(icon.exists() && icon.isReadable()) {
            def->IconTexture.load(icon);
        }
    } else {
        def->SceneModel = LoadModelFile(fileName.toUtf8().data(), glPrograms, materialParms );

        VFile icon(iconFileName, VFile::ReadOnly);
        if (icon.exists() && icon.isReadable()) {
            def->IconTexture.load(icon);
        }
    }

    if (def->IconTexture.id() != 0) {
        vInfo("Loaded external icon for theater:" << iconFileName);
    } else {
		const ModelTexture * iconTexture = def->SceneModel->FindNamedTexture( "icon" );
		if ( iconTexture != NULL )
		{
			def->IconTexture = iconTexture->texid;
		}
		else
		{
			vInfo("No icon in scene.  Loading default.");
            def->IconTexture.load(VResource("assets/noimage.png"));
		}
	}

    def->IconTexture.buildMipmaps();
    def->IconTexture.trilinear();
    def->IconTexture.clamp();

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
