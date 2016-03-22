/************************************************************************************

Filename    :   VrScene.cpp
Content     :   Trivial game style scene viewer VR sample
Created     :   September 8, 2013
Authors     :   John Carmack

Copyright   :   Copyright 2012 Oculus VR, LCC. All Rights reserved.

*************************************************************************************/

#include "VrScene.h"

#include <VStandardPath.h>
#include <VLog.h>

static const char * versionString = "VrScene v0.1.0";

extern "C"
{

void Java_com_vrseen_nervgear_scene_MainActivity_nativeSetAppInterface( JNIEnv *jni, jclass clazz, jobject activity,
	jstring fromPackageName, jstring commandString, jstring uriString )
{
	// This is called by the java UI thread.
	LOG( "nativeSetAppInterface" );
    (new VrScene(jni, clazz, activity))->onCreate( jni, clazz, activity, fromPackageName, commandString, uriString );
}

} // extern "C"

//=============================================================================
//                             VrScene
//=============================================================================

VrScene::VrScene(JNIEnv *jni, jclass activityClass, jobject activityObject)
    : VMainActivity(jni, activityClass, activityObject)
    , forceScreenClear( false )
    , ModelLoaded( false )
{
}

VrScene::~VrScene() {
	LOG( "~VrScene()");
}

void VrScene::ConfigureVrMode( ovrModeParms & modeParms )
{
	modeParms.CpuLevel = 2;
	modeParms.GpuLevel = 2;

	// Always use 2x MSAA for now
	vApp->vrParms().multisamples = 2;
}

void VrScene::init(const VString &fromPackage, const VString &launchIntentJSON, const VString &launchIntentURI)
{
	LOG( "VrScene::OneTimeInit" );

    vApp->storagePaths().PushBackSearchPathIfValid(VStandardPath::SecondaryExternalStorage, VStandardPath::RootFolder, "RetailMedia/", SearchPaths);
    vApp->storagePaths().PushBackSearchPathIfValid(VStandardPath::SecondaryExternalStorage, VStandardPath::RootFolder, "", SearchPaths);
    vApp->storagePaths().PushBackSearchPathIfValid(VStandardPath::PrimaryExternalStorage, VStandardPath::RootFolder, "RetailMedia/", SearchPaths);
    vApp->storagePaths().PushBackSearchPathIfValid(VStandardPath::PrimaryExternalStorage, VStandardPath::RootFolder, "", SearchPaths);

	// Check if we already loaded the model through an intent
	if ( !ModelLoaded )
	{
		LoadScene( launchIntentURI );
	}
}

void VrScene::shutdown()
{
	LOG( "VrScene::OneTimeShutdown" );

	// Free GL resources
}

void VrScene::onNewIntent( const char * fromPackageName, const char * command, const char * uri )
{
	LOG( "NewIntent - fromPackageName : %s, command : %s, uri : %s", fromPackageName, command, uri );

	// Scene will be loaded in "OneTimeInit" function.
	// LoadScene( intent );
}

// uncomment this to make the intent load a test model instead of change the scene.
// we may just want to make the intent consist of <command> <parameters>.
//#define INTENT_TEST_MODEL

void VrScene::LoadScene( const VString &path )
{
    vInfo("VrScene::LoadScene" << path);

#if defined( INTENT_TEST_MODEL )
    const VString scenePath = u"Oculus/tuscany.ovrscene";
#else
    const VString scenePath = !path.isEmpty() ? path : VString(u"Oculus/tuscany.ovrscene");
#endif
	if ( !GetFullPath( SearchPaths, scenePath, SceneFile ) )
	{
        vInfo("VrScene::NewIntent SearchPaths failed to find" << scenePath);
	}

	MaterialParms materialParms;
	materialParms.UseSrgbTextureFormats = ( vApp->vrParms().colorFormat == COLOR_8888_sRGB );
	LOG( "VrScene::LoadScene loading %s", SceneFile.toCString() );
    Scene.LoadWorldModel( SceneFile, materialParms );
	ModelLoaded = true; 
	LOG( "VrScene::LoadScene model is loaded" );
	Scene.YawOffset = -M_PI / 2;

#if defined( INTENT_TEST_MODEL )
	// load a test model
	const char * testModelPath = intent;
	if ( testModelPath != NULL && testModelPath[0] != '\0' )
	{
		// Create the render programs we are going to use
		VGlShader ProgSingleTexture;
		ProgSingleTexture.initShader(SingleTextureVertexShaderSrc,
				SingleTextureFragmentShaderSrc );

		ModelGlPrograms programs( &ProgSingleTexture );

		TestObject.SetModelFile( LoadModelFile( testModelPath, programs, materialParms ) );
		Scene.AddModel( &TestObject );
	}
#endif

	// When launched by an intent, we may be viewing a partial
	// scene for debugging, so always clear the screen to grey
	// before drawing, instead of letting partial renders show through.
	forceScreenClear = ( path[0] != '\0' );
}

void VrScene::ReloadScene()
{
	// Reload the scene, presumably to switch texture formats
    const V3Vectf pos = Scene.FootPos;
	const float	yaw = Scene.YawOffset;

	MaterialParms materialParms;
	materialParms.UseSrgbTextureFormats = ( vApp->vrParms().colorFormat == COLOR_8888_sRGB );
    Scene.LoadWorldModel( SceneFile, materialParms );

	Scene.YawOffset = yaw;
	Scene.FootPos = pos;
}

void VrScene::Command( const char * msg )
{
}

VR4Matrixf VrScene::drawEyeView( const int eye, const float fovDegrees )
{
	if ( forceScreenClear )
	{
		glClearColor( 0.25f, 0.25f, 0.25f, 1.0f );
		glClear( GL_COLOR_BUFFER_BIT );
	}

    const VR4Matrixf view = Scene.DrawEyeView( eye, fovDegrees );

	return view;
}

VR4Matrixf VrScene::onNewFrame( const VrFrame vrFrame )
{
	// Get the current vrParms for the buffer resolution.
	const EyeParms vrParms = vApp->eyeParms();

	// Player movement
	Scene.Frame( vApp->vrViewParms(), vrFrame, vApp->swapParms().ExternalVelocity );

	// Make the test object hop up and down
	{
		const float y = 1 + sin( 2 * vrFrame.PoseState.TimeInSeconds );
		TestObject.State.modelMatrix.M[0][3] = 2;
		TestObject.State.modelMatrix.M[1][3] = y;
	}

	// these should probably use OnKeyEvent() now so that the menu can just consume the events
	// if it's open, rather than having an explicit check here.
	if ( !vApp->isGuiOpen() )
	{
		//-------------------------------------------
		// Check for button actions
		//-------------------------------------------
		if ( !( vrFrame.Input.buttonState & BUTTON_RIGHT_TRIGGER ) )
		{
			if ( vrFrame.Input.buttonPressed & BUTTON_SELECT )
			{
				vApp->createToast( "%s", versionString );
			}

			// Switch buffer parameters for testing
			if ( vrFrame.Input.buttonPressed & BUTTON_X )
			{
				EyeParms newParms = vrParms;
				switch ( newParms.multisamples )
				{
					case 2: newParms.multisamples = 4; break;
					case 4: newParms.multisamples = 1; break;
					default: newParms.multisamples = 2; break;
				}
				vApp->setEyeParms( newParms );
				vApp->createToast( "multisamples: %i", newParms.multisamples );
			}
		}
	}

	//-------------------------------------------
	// Render the two eye views, each to a separate texture, and TimeWarp
	// to the screen.
	//-------------------------------------------
	vApp->drawEyeViewsPostDistorted( Scene.CenterViewMatrix() );

	return Scene.CenterViewMatrix();
}
