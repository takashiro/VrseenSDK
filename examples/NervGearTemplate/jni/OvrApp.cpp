#include <jni.h>
#include <VStandardPath.h>

#include "OvrApp.h"

extern "C" {

void Java_oculus_MainActivity_nativeSetAppInterface( JNIEnv * jni, jclass clazz, jobject activity,
		jstring fromPackageName, jstring commandString, jstring uriString )
{
	LOG( "nativeSetAppInterface" );
    (new OvrApp(jni, activity))->SetActivity( jni, clazz, activity, fromPackageName, commandString, uriString );
}

} // extern "C"

OvrApp::OvrApp(JNIEnv *jni, jobject activityObject)
    : VMainActivity(jni, activityObject)
{
}

OvrApp::~OvrApp()
{
}

void OvrApp::OneTimeInit(const VString &fromPackage, const VString &launchIntentJSON, const VString &launchIntentURI)
{
	// This is called by the VR thread, not the java UI thread.
	MaterialParms materialParms;
	materialParms.UseSrgbTextureFormats = false;
        
	const char * scenePath = "Oculus/tuscany.ovrscene";
	VString	        SceneFile;
	VArray<VString>   SearchPaths;

    const VStandardPath &paths = app->storagePaths();
    paths.PushBackSearchPathIfValid(VStandardPath::SecondaryExternalStorage, VStandardPath::RootFolder, "RetailMedia/", SearchPaths);
    paths.PushBackSearchPathIfValid(VStandardPath::SecondaryExternalStorage, VStandardPath::RootFolder, "", SearchPaths);
    paths.PushBackSearchPathIfValid(VStandardPath::PrimaryExternalStorage, VStandardPath::RootFolder, "RetailMedia/", SearchPaths);
    paths.PushBackSearchPathIfValid(VStandardPath::PrimaryExternalStorage, VStandardPath::RootFolder, "", SearchPaths);

	if ( GetFullPath( SearchPaths, scenePath, SceneFile ) )
	{
		Scene.LoadWorldModel( SceneFile , materialParms );
	}
	else
	{
		LOG( "OvrApp::OneTimeInit SearchPaths failed to find %s", scenePath );
	}
}

void OvrApp::OneTimeShutdown()
{
	// Free GL resources
        
}

void OvrApp::Command( const char * msg )
{
}

Matrix4f OvrApp::DrawEyeView( const int eye, const float fovDegrees )
{
	const Matrix4f view = Scene.DrawEyeView( eye, fovDegrees );

	return view;
}

Matrix4f OvrApp::Frame(const VrFrame vrFrame)
{
	// Player movement
    Scene.Frame( app->vrViewParms(), vrFrame, app->swapParms().ExternalVelocity );

	app->drawEyeViewsPostDistorted( Scene.CenterViewMatrix() );

	return Scene.CenterViewMatrix();
}
