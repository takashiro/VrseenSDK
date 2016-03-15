#include "VMainActivity.h"

#include <jni.h>
#include <android/native_window_jni.h>

#include "App.h"
#include "SurfaceTexture.h"

#include "android/JniUtils.h"
#include "VLog.h"
#include "VString.h"

NV_NAMESPACE_BEGIN

struct VMainActivity::Private
{
    JNIEnv *jni;
    jobject activityObject;
};

VMainActivity::VMainActivity(JNIEnv *jni, jobject activityObject)
    : d(new Private)
{
    d->jni = jni;
    d->activityObject = jni->NewGlobalRef(activityObject);
}

VMainActivity::~VMainActivity()
{
    d->jni->DeleteGlobalRef(d->activityObject);
    delete d;
}

NV_NAMESPACE_END

NV_USING_NAMESPACE

//@to-do: remove this
static VString ComposeIntentMessage(const VString &packageName, const VString &uri, const VString &jsonText)
{
    VString out = "intent ";
    out.append(packageName);
    out.append(' ');
    out.append(uri);
    out.append(' ');
    out.append(jsonText);
    return out;
}

extern "C"
{

void Java_com_vrseen_nervgear_VrActivity_nativeSurfaceChanged(JNIEnv *jni, jclass clazz,
        jlong appPtr, jobject surface)
{
    ((App *)appPtr)->messageQueue().SendPrintf("surfaceChanged %p",
            surface ? ANativeWindow_fromSurface(jni, surface) : nullptr);
}

void Java_com_vrseen_nervgear_VrActivity_nativeSurfaceDestroyed(JNIEnv *jni, jclass clazz,
        jlong appPtr, jobject)
{
    if (appPtr == 0)
    {
        // Android may call surfaceDestroyed() after onDestroy().
        vInfo("nativeSurfaceChanged was called after onDestroy. We cannot destroy the surface now because we don't have a valid app pointer.");
        return;
    }

    ((App *)appPtr)->messageQueue().SendPrintf("surfaceDestroyed ");
}

void Java_com_vrseen_nervgear_VrActivity_nativePopup(JNIEnv *jni, jclass clazz,
        jlong appPtr, jint width, jint height, jfloat seconds)
{
    ((App *)appPtr)->messageQueue().PostPrintf("popup %i %i %f", width, height, seconds);
}

jobject Java_com_vrseen_nervgear_VrActivity_nativeGetPopupSurfaceTexture(JNIEnv *jni, jclass clazz,
        jlong appPtr)
{
    return ((App *)appPtr)->dialogTexture()->javaObject;
}

void Java_com_vrseen_nervgear_VrActivity_nativePause(JNIEnv *jni, jclass clazz,
        jlong appPtr)
{
    ((App *)appPtr)->messageQueue().SendPrintf("pause ");
}

void Java_com_vrseen_nervgear_VrActivity_nativeResume(JNIEnv *jni, jclass clazz,
        jlong appPtr)
{
    ((App *)appPtr)->messageQueue().SendPrintf("resume ");
}

void Java_com_vrseen_nervgear_VrActivity_nativeDestroy(JNIEnv *jni, jclass clazz,
        jlong appPtr)
{
    App * localPtr = (App *)appPtr;
    const bool exitOnDestroy = localPtr->exitOnDestroy;

    // First kill the VrThread.
    localPtr->stopVrThread();
    // Then delete the VrAppInterface derived class.
    delete localPtr->appInterface();
    // Last delete AppLocal.
    delete localPtr;

    // Execute ovr_Shutdown() here on the Java thread because ovr_Initialize()
    // was also called from the Java thread through JNI_OnLoad().
    if (exitOnDestroy) {
        vInfo("ExitOnDestroy is true, exiting");
        ovr_ExitActivity(nullptr, EXIT_TYPE_EXIT);
    } else {
        vInfo("ExitOnDestroy was false, returning normally.");
    }
}

void Java_com_vrseen_nervgear_VrActivity_nativeJoypadAxis(JNIEnv *jni, jclass clazz,
        jlong appPtr, jfloat lx, jfloat ly, jfloat rx, jfloat ry)
{
    App * local = ((App *)appPtr);
    // Suspend input until OneTimeInit() has finished to avoid overflowing the message queue on long loads.
    if (local->oneTimeInitCalled) {
        local->messageQueue().PostPrintf("joy %f %f %f %f", lx, ly, rx, ry);
    }
}

void Java_com_vrseen_nervgear_VrActivity_nativeTouch(JNIEnv *jni, jclass clazz,
        jlong appPtr, jint action, jfloat x, jfloat y)
{
    App * local = ((App *)appPtr);
    // Suspend input until OneTimeInit() has finished to avoid overflowing the message queue on long loads.
    if (local->oneTimeInitCalled) {
        local->messageQueue().PostPrintf("touch %i %f %f", action, x, y);
    }
}

void Java_com_vrseen_nervgear_VrActivity_nativeKeyEvent(JNIEnv *jni, jclass clazz,
        jlong appPtr, jint key, jboolean down, jint repeatCount)
{
    App * local = ((App *)appPtr);
    // Suspend input until OneTimeInit() has finished to avoid overflowing the message queue on long loads.
    if (local->oneTimeInitCalled) {
        local->messageQueue().PostPrintf("key %i %i %i", key, down, repeatCount);
    }
}

void Java_com_vrseen_nervgear_VrActivity_nativeNewIntent(JNIEnv *jni, jclass clazz,
        jlong appPtr, jstring fromPackageName, jstring command, jstring uriString)
{
    VString utfPackageName = JniUtils::Convert(jni, fromPackageName);
    VString utfUri = JniUtils::Convert(jni, uriString);
    VString utfJson = JniUtils::Convert(jni, command);

    VString intentMessage = ComposeIntentMessage(utfPackageName, utfUri, utfJson);
    vInfo("nativeNewIntent:" << intentMessage);
    VByteArray utf8Message = intentMessage.toUtf8();
    ((App *) appPtr)->messageQueue().PostPrintf(utf8Message.data());
}

}	// extern "C"
