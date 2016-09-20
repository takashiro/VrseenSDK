#pragma once

#include "VEventLoop.h"
#include "VString.h"

#include "VFrame.h"
#include "KeyState.h"
#include "VMatrix.h"

#include <jni.h>

NV_NAMESPACE_BEGIN

class VKernel;

enum MovieFormat
{
    VT_UNKNOWN,
    VT_2D,
    VT_LEFT_RIGHT_3D,			// Left & right are scaled horizontally by 50%.
    VT_LEFT_RIGHT_3D_FULL,		// Left & right are unscaled.
    VT_TOP_BOTTOM_3D,			// Top & bottom are scaled vertically by 50%.
    VT_TOP_BOTTOM_3D_FULL,		// Top & bottom are unscaled.
};

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

    virtual VMatrix4f onNewFrame( VFrame vrFrame );
    virtual VMatrix4f drawEyeView( const int eye, const float fovDegrees );
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

    VMatrix4f  getTexMatrix(int eye,MovieFormat format);
    virtual VMatrix4f  getModelViewProMatrix(int eye) const;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VMainActivity)
};

NV_NAMESPACE_END
