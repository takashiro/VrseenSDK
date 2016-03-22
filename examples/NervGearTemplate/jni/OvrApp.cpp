#include <jni.h>
#include <VStandardPath.h>

#include "OvrApp.h"

extern "C" {

void Java_oculus_MainActivity_nativeSetAppInterface( JNIEnv * jni, jclass clazz, jobject activity,
		jstring fromPackageName, jstring commandString, jstring uriString )
{
	LOG( "nativeSetAppInterface" );
    (new OvrApp(jni, clazz, activity))->onCreate(fromPackageName, commandString, uriString );
}

} // extern "C"

OvrApp::OvrApp(JNIEnv *jni, jclass activityClass, jobject activityObject)
    : VMainActivity(jni, activityClass, activityObject)
{
}

OvrApp::~OvrApp()
{
}

void OvrApp::init(const VString &fromPackage, const VString &launchIntentJSON, const VString &launchIntentURI)
{
	// This is called by the VR thread, not the java UI thread.
	MaterialParms materialParms;
	materialParms.UseSrgbTextureFormats = false;
        
	const char * scenePath = "Oculus/tuscany.ovrscene";
	VString	        SceneFile;
	VArray<VString>   SearchPaths;

    const VStandardPath &paths = vApp->storagePaths();
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

void OvrApp::shutdown()
{
	// Free GL resources
        
}

Matrix4f OvrApp::drawEyeView( const int eye, const float fovDegrees )
{
	const Matrix4f view = Scene.DrawEyeView( eye, fovDegrees );

	return view;
}

Matrix4f OvrApp::onNewFrame(const VrFrame vrFrame)
{
	// Player movement
    Scene.Frame( vApp->vrViewParms(), vrFrame, vApp->swapParms().ExternalVelocity );

	vApp->drawEyeViewsPostDistorted( Scene.CenterViewMatrix() );

	return Scene.CenterViewMatrix();
}
