#pragma once

#include "vglobal.h"
#include "VString.h"

#include "Input.h"
#include "KeyState.h"

#include <jni.h>

NV_NAMESPACE_BEGIN

class App;

class VMainActivity
{
public:
    VMainActivity(JNIEnv *jni, jobject activityObject);
    virtual ~VMainActivity();

    void finishActivity();

    VString getPackageCodePath() const;
    VString getPackageName() const;

    void SetActivity( JNIEnv * jni, jclass clazz, jobject activity,
            jstring javaFromPackageNameString, jstring javaCommandString,
            jstring javaUriString );

    virtual void OneTimeInit(const VString &fromPackage, const VString &launchIntentJSON, const VString &launchIntentURI) = 0;
    virtual void OneTimeShutdown();
    virtual Matrix4f Frame( VrFrame vrFrame );
    virtual Matrix4f DrawEyeView( const int eye, const float fovDegrees );
    virtual void NewIntent( const char * fromPackageName, const char * command, const char * uri );
    virtual void ConfigureVrMode( ovrModeParms & modeParms );
    virtual void WindowCreated();
    virtual void WindowDestroyed();
    virtual void Paused();
    virtual void Resumed();
    virtual void Command( const char * msg );
    virtual bool onKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType );
    virtual bool OnVrWarningDismissed( const bool accepted );
    virtual bool ShouldShowLoadingIcon() const;
    virtual bool wantSrgbFramebuffer() const;
    virtual bool GetWantProtectedFramebuffer() const;

    jclass javaClass() const;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VMainActivity)
};

NV_NAMESPACE_END
