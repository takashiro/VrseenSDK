#pragma once

#include "VMainActivity.h"

#include "App.h"
#include "ModelView.h"

NV_USING_NAMESPACE

class OvrApp : public NervGear::VMainActivity
{
public:
                        OvrApp(JNIEnv *jni, jobject activityObject);
    virtual				~OvrApp();

	virtual void		OneTimeInit(const VString &fromPackage, const VString &launchIntentJSON, const VString &launchIntentURI );
	virtual void		OneTimeShutdown();
	virtual Matrix4f 	DrawEyeView( const int eye, const float fovDegrees );
	virtual Matrix4f 	Frame( VrFrame vrFrame );
	virtual void		Command( const char * msg );

	OvrSceneView		Scene;
};
