#pragma once

#include "VEventLoop.h"
#include "VString.h"

#include "Input.h"
#include "KeyState.h"
#include "VMatrix.h"

#include <jni.h>

NV_NAMESPACE_BEGIN

class VKernel;

class VMainActivity
{
public:
    VMainActivity(JNIEnv *jni, jclass activityClass, jobject activityObject);
    virtual ~VMainActivity();

    void finishActivity();

    VString getPackageCodePath() const;
    VString getPackageName() const;

    void onCreate(jstring javaFromPackageNameString, jstring javaCommandString, jstring javaUriString);

    virtual void init(const VString &fromPackage, const VString &launchIntentJSON, const VString &launchIntentURI) = 0;
    virtual void shutdown();

    virtual VR4Matrixf onNewFrame( VrFrame vrFrame );
    virtual VR4Matrixf drawEyeView( const int eye, const float fovDegrees );
    virtual void configureVrMode(VKernel* kernel);

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

    JNIEnv *javaEnv() const;
    jclass javaClass() const;
    jobject javaObject() const;

    void Init(JavaVM *javaVM);
    VEventLoop &eventLoop();

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VMainActivity)
};

NV_NAMESPACE_END
