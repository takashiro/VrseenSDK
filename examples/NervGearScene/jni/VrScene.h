#pragma once

#include "VMainActivity.h"

#include "App.h"
#include "ModelView.h"

NV_USING_NAMESPACE

class VrScene : public NervGear::VMainActivity
{
public:
                        VrScene(JNIEnv *jni, jclass activityClass, jobject activityObject);
						~VrScene();

	virtual void 		configureVrMode( ovrModeParms & modeParms );
	virtual void		init(const VString &fromPackage, const VString &launchIntentJSON, const VString &launchIntentURI );
	virtual void		shutdown();
	virtual Matrix4f	drawEyeView( const int eye, const float fovDegrees );
	virtual Matrix4f	onNewFrame( VrFrame vrFrame );
    virtual	void		onNewIntent(const VString &fromPackageName, const VString &command, const VString &uri);

    void				LoadScene(const VString &path );
    void				ReloadScene();

	// When launched by an intent, we may be viewing a partial
	// scene for debugging, so always clear the screen to grey
	// before drawing, instead of letting partial renders show through.
	bool				forceScreenClear;

	bool				ModelLoaded;

	VString				SceneFile;	// for reload
	OvrSceneView		Scene;

	ModelInScene		TestObject;		// bouncing object

	VArray<VString> 		SearchPaths;
};
