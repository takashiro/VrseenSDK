#pragma once

#include "VMainActivity.h"

#include "App.h"
#include "ModelView.h"

NV_USING_NAMESPACE

class OvrApp : public NervGear::VMainActivity
{
public:
                        OvrApp(JNIEnv *jni, jclass activityClass, jobject activityObject);
    virtual				~OvrApp();

	virtual void		init(const VString &fromPackage, const VString &launchIntentJSON, const VString &launchIntentURI );
	virtual void		shutdown();
    virtual VR4Matrixf 	drawEyeView( const int eye, const float fovDegrees );
    virtual VR4Matrixf 	onNewFrame( VrFrame vrFrame );

	OvrSceneView		Scene;
};
