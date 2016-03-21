#include "VMainActivity.h"

#include <android/native_window_jni.h>

#include "App.h"
#include "SurfaceTexture.h"

#include "android/JniUtils.h"
#include "VLog.h"

NV_NAMESPACE_BEGIN

struct VMainActivity::Private
{
    JNIEnv *jni;
    jobject activityObject;
    jclass activityClass;

    jmethodID getMethodID(const char *name, const char *signature) const
    {
        jmethodID mid = jni->GetMethodID(activityClass, name, signature);
        if (!mid) {
            vFatal("couldn't get" << name);
        }
        return mid;
    }
};

VMainActivity::VMainActivity(JNIEnv *jni, jclass activityClass, jobject activityObject)
    : d(new Private)
{
    d->jni = jni;
    d->activityObject = jni->NewGlobalRef(activityObject);
    d->activityClass = (jclass) jni->NewGlobalRef(activityClass);
}

VMainActivity::~VMainActivity()
{
    d->jni->DeleteGlobalRef(d->activityClass);
    d->jni->DeleteGlobalRef(d->activityObject);
    delete d;
}

void VMainActivity::finishActivity()
{
    jmethodID method = d->getMethodID("finishActivity", "()V");
    d->jni->CallVoidMethod(d->activityObject, method);
}

VString VMainActivity::getPackageCodePath() const
{
    jmethodID getPackageCodePathId = d->jni->GetMethodID(d->activityClass, "getPackageCodePath", "()Ljava/lang/String;");
    if (getPackageCodePathId == 0) {
        vInfo("Failed to find getPackageCodePath on class" << (ulonglong) d->activityClass);
        return VString();
    }

    VString packageCodePath = JniUtils::Convert(d->jni, (jstring) d->jni->CallObjectMethod(d->activityObject, getPackageCodePathId));
    if (!d->jni->ExceptionOccurred()) {
        vInfo("getPackageCodePath() = " << packageCodePath);
        return packageCodePath;
    } else {
        d->jni->ExceptionClear();
        vInfo("Cleared JNI exception");
    }

    return VString();
}

VString VMainActivity::getPackageName() const
{
    jmethodID getPackageNameId = d->jni->GetMethodID(d->activityClass, "getPackageName", "()Ljava/lang/String;");
    if (getPackageNameId != 0) {
        VString packageName = JniUtils::Convert(d->jni, (jstring) d->jni->CallObjectMethod(d->activityObject, getPackageNameId));
        if (!d->jni->ExceptionOccurred()) {
            vInfo("GetCurrentPackageName() =" << packageName);
            return packageName;
        } else {
            d->jni->ExceptionClear();
            vInfo("Cleared JNI exception");
        }
    }
    return VString();
}


void VMainActivity::onCreate(jstring javaFromPackageNameString, jstring javaCommandString, jstring javaUriString)
{
    JNIEnv *jni = javaEnv();
    jobject activity = javaObject();
    VString fromPackage = JniUtils::Convert(jni, javaFromPackageNameString);
    VString json = JniUtils::Convert(jni, javaCommandString);
    VString uri = JniUtils::Convert(jni, javaUriString);
    vInfo("VMainActivity::SetActivity:" << fromPackage << json << uri);

    if (vApp == nullptr)
    {	// First time initialization
        // This will set the VrAppInterface app pointer directly,
        // so it is set when OneTimeInit is called.
        vInfo("new AppLocal()");
        new App(jni, activity, this);

        // Start the VrThread and wait for it to have initialized.
        vApp->startVrThread();
        vApp->syncVrThread();
    }
    else
    {	// Just update the activity object.
        vInfo("Update AppLocal");
        if (vApp->javaObject() != nullptr)
        {
            jni->DeleteGlobalRef(vApp->javaObject());
        }
        vApp->javaObject() = jni->NewGlobalRef(activity);
        vApp->VrModeParms.ActivityObject = vApp->javaObject();
    }

    // Send the intent and wait for it to complete.
    VJson args(VJson::Array);
    args << fromPackage << uri << json;
    vApp->eventLoop().post("intent", args);
    vApp->syncVrThread();
}

void VMainActivity::shutdown()
{
}

void VMainActivity::onWindowCreated()
{
    vInfo("VMainActivity::WindowCreated - default handler called");
}

void VMainActivity::onWindowDestroyed()
{
    vInfo("VMainActivity::WindowDestroyed - default handler called");
}

void VMainActivity::onPause()
{
    vInfo("VMainActivity::Paused - default handler called");
}

void VMainActivity::onResume()
{
    vInfo("VMainActivity::Resumed - default handler called");
}

void VMainActivity::command(const VEvent &msg)
{
    vInfo("VMainActivity::Command - default handler called, msg =" << msg.name);
}

JNIEnv *VMainActivity::javaEnv() const
{
    return d->jni;
}

void VMainActivity::onNewIntent(const VString &fromPackageName, const VString &command, const VString &uri)
{
    vInfo("VMainActivity::NewIntent - default handler called -" << fromPackageName << command << uri);
}

Matrix4f VMainActivity::onNewFrame(VrFrame vrFrame)
{
    vInfo("VMainActivity::Frame - default handler called");
    return Matrix4f();
}

void VMainActivity::configureVrMode(ovrModeParms & modeParms)
{
    vInfo("VMainActivity::ConfigureVrMode - default handler called");
}

Matrix4f VMainActivity::drawEyeView(const int eye, const float fovDegrees)
{
    vInfo("VMainActivity::DrawEyeView - default handler called");
    return Matrix4f();
}

bool VMainActivity::onKeyEvent(const int keyCode, const KeyState::eKeyEventType eventType)
{
    vInfo("VMainActivity::OnKeyEvent - default handler called");
    return false;
}

bool VMainActivity::onVrWarningDismissed(const bool accepted)
{
    vInfo("VMainActivity::OnVrWarningDismissed - default handler called");
    return false;
}

bool VMainActivity::showLoadingIcon() const
{
    return true;
}

bool VMainActivity::wantSrgbFramebuffer() const
{
    return false;
}

bool VMainActivity::wantProtectedFramebuffer() const
{
    return false;
}

jclass VMainActivity::javaClass() const
{
    return d->activityClass;
}

jobject VMainActivity::javaObject() const
{
    return d->activityObject;
}

NV_NAMESPACE_END

NV_USING_NAMESPACE

extern "C"
{

void Java_com_vrseen_nervgear_VrActivity_nativeSurfaceChanged(JNIEnv *jni, jclass, jobject surface)
{
    vApp->eventLoop().send("surfaceChanged", reinterpret_cast<int>(surface ? ANativeWindow_fromSurface(jni, surface) : nullptr));
}

void Java_com_vrseen_nervgear_VrActivity_nativeSurfaceDestroyed(JNIEnv *jni, jclass clazz)
{
    if (vApp == 0)
    {
        // Android may call surfaceDestroyed() after onDestroy().
        vInfo("nativeSurfaceChanged was called after onDestroy. We cannot destroy the surface now because we don't have a valid app pointer.");
        return;
    }

    vApp->eventLoop().send("surfaceDestroyed");
}

void Java_com_vrseen_nervgear_VrActivity_nativePopup(JNIEnv *, jclass,
        jint width, jint height, jfloat seconds)
{
    VJson args(VJson::Array);
    args << width << height << seconds;
    vApp->eventLoop().post("popup", args);
}

jobject Java_com_vrseen_nervgear_VrActivity_nativeGetPopupSurfaceTexture(JNIEnv *, jclass)
{
    return vApp->dialogTexture()->javaObject;
}

void Java_com_vrseen_nervgear_VrActivity_nativePause(JNIEnv *jni, jclass clazz,
        jlong appPtr)
{
    vApp->eventLoop().send("pause");
}

void Java_com_vrseen_nervgear_VrActivity_nativeResume(JNIEnv *jni, jclass clazz)
{
    vApp->eventLoop().send("resume");
}

void Java_com_vrseen_nervgear_VrActivity_nativeDestroy(JNIEnv *, jclass)
{
    // First kill the VrThread.
    vApp->stopVrThread();
    // Then delete the VrAppInterface derived class.
    // Last delete AppLocal.
    delete vApp;

    vInfo("ExitOnDestroy is true, exiting");
    ovr_ExitActivity(nullptr, EXIT_TYPE_EXIT);
}

void Java_com_vrseen_nervgear_VrActivity_nativeJoypadAxis(JNIEnv *jni, jclass clazz,
        jfloat lx, jfloat ly, jfloat rx, jfloat ry)
{
    // Suspend input until OneTimeInit() has finished to avoid overflowing the message queue on long loads.
    if (vApp->oneTimeInitCalled) {
        VJson args(VJson::Array);
        args << lx << ly << rx << ry;
        vApp->eventLoop().post("joy", args);
    }
}

void Java_com_vrseen_nervgear_VrActivity_nativeTouch(JNIEnv *, jclass,
        jint action, jfloat x, jfloat y)
{
    // Suspend input until OneTimeInit() has finished to avoid overflowing the message queue on long loads.
    if (vApp->oneTimeInitCalled) {
        VJson args(VJson::Array);
        args << action << x << y;
        vApp->eventLoop().post("touch", args);
    }
}

void Java_com_vrseen_nervgear_VrActivity_nativeKeyEvent(JNIEnv *jni, jclass clazz,
        jint key, jboolean down, jint repeatCount)
{
    // Suspend input until OneTimeInit() has finished to avoid overflowing the message queue on long loads.
    if (vApp->oneTimeInitCalled) {
        VJson args(VJson::Array);
        args << key << down << repeatCount;
        vApp->eventLoop().post("key", args);
    }
}

void Java_com_vrseen_nervgear_VrActivity_nativeNewIntent(JNIEnv *jni, jclass clazz,
        jstring fromPackageName, jstring command, jstring uriString)
{
    VString packageName = JniUtils::Convert(jni, fromPackageName);
    VString uri = JniUtils::Convert(jni, uriString);
    VString json = JniUtils::Convert(jni, command);

    VJson args(VJson::Array);
    args << packageName << uri << json;
    vApp->eventLoop().post("intent", args);
}

}	// extern "C"
