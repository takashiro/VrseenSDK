#pragma once

#include "VEvent.h"
#include "VString.h"

#include "Input.h"
#include "KeyState.h"

#include <jni.h>

NV_NAMESPACE_BEGIN

class App;

class VMainActivity
{
public:
    VMainActivity(JNIEnv *jni, jclass activityClass, jobject activityObject);
    virtual ~VMainActivity();

    void finishActivity();

    VString getPackageCodePath() const;
    VString getPackageName() const;

    void onCreate( JNIEnv * jni, jclass clazz, jobject activity,
            jstring javaFromPackageNameString, jstring javaCommandString,
            jstring javaUriString );

    virtual void init(const VString &fromPackage, const VString &launchIntentJSON, const VString &launchIntentURI) = 0;
    virtual void shutdown();

    virtual Matrix4f onNewFrame( VrFrame vrFrame );
    virtual Matrix4f drawEyeView( const int eye, const float fovDegrees );
    virtual void configureVrMode( ovrModeParms & modeParms );

    virtual void onNewIntent(const VString &fromPackageName, const VString &command, const VString &uri);
    virtual void onWindowCreated();
    virtual void onWindowDestroyed();
    virtual void onPause();
    virtual void onResume();
    virtual bool onKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType );
    virtual bool onVrWarningDismissed( const bool accepted );

    virtual bool showLoadingIcon() const;
    virtual bool wantSrgbFramebuffer() const;
    virtual bool wantProtectedFramebuffer() const;

    virtual void command(const VEvent &msg);

    jclass javaClass() const;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VMainActivity)
};

NV_NAMESPACE_END
