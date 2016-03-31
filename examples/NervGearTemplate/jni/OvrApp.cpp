#include <jni.h>
#include <VStandardPath.h>

#include "OvrApp.h"

extern "C" {

void Java_oculus_MainActivity_nativeSetAppInterface( JNIEnv * jni, jclass clazz, jobject activity,
		jstring fromPackageName, jstring commandString, jstring uriString )
{
	vInfo("nativeSetAppInterface");
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
		vInfo("OvrApp::OneTimeInit SearchPaths failed to find" << scenePath);
	}
}

void OvrApp::shutdown()
{
	// Free GL resources
        
}

VR4Matrixf OvrApp::drawEyeView( const int eye, const float fovDegrees )
{
    const VR4Matrixf view = Scene.DrawEyeView( eye, fovDegrees );

	return view;
}

VR4Matrixf OvrApp::onNewFrame(const VrFrame vrFrame)
{
	// Player movement
    Scene.Frame( vApp->vrViewParms(), vrFrame, vApp->kernel()->m_externalVelocity);

	vApp->drawEyeViewsPostDistorted( Scene.CenterViewMatrix() );

	return Scene.CenterViewMatrix();
}
