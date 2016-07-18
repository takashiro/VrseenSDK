#include "VMainActivity.h"

#include <android/native_window_jni.h>

#include "App.h"
#include "SurfaceTexture.h"
#include "VKernel.h"

#include "android/JniUtils.h"
#include "VLog.h"

NV_NAMESPACE_BEGIN

struct VMainActivity::Private
{
    JNIEnv *jni;
    jobject activityObject;
    jclass activityClass;

    JavaVM *javaVM;
    pthread_t threadHandle;
    VEventLoop eventLoop;

    Private()
        : javaVM(nullptr)
        , threadHandle(0)
        , eventLoop(1000)
    {
    }

    ~Private()
    {
        if (threadHandle) {
            // Get the background thread to kill itself.
            eventLoop.post("quit");
            const int ret = pthread_join(threadHandle, NULL);
            if (ret != 0) {
                vWarn("failed to join TtjThread (" << ret << ")");
            }
        }
    }

    jmethodID getMethodID(const char *name, const char *signature) const
    {
        jmethodID mid = jni->GetMethodID(activityClass, name, signature);
        if (!mid) {
            vFatal("couldn't get" << name);
        }
        return mid;
    }

    void exec()
    {
        JNIEnv *Jni = nullptr;
        // The Java VM needs to be attached on each thread that will use it.
        vInfo("TalkToJava: Jvm->AttachCurrentThread");
        const jint returnAttach = javaVM->AttachCurrentThread(&Jni, 0);
        if (returnAttach != JNI_OK) {
            vInfo("javaVM->AttachCurrentThread returned" << returnAttach);
        }

        // Process all queued messages
        forever {
            VEvent event = eventLoop.next();
            if (!event.isValid()) {
                // Go dormant until something else arrives.
                eventLoop.wait();
                continue;
            }

            if (event.name == "quit") {
                break;
            }

            // Set up a local frame with room for at least 100
            // local references that will be auto-freed.
            Jni->PushLocalFrame(100);

            // Let whoever initialized us do what they want.
            if (event.isExecutable()) {
                event.execute();
            }

            // If we don't clean up exceptions now, later
            // calls may fail.
            if (Jni->ExceptionOccurred()) {
                Jni->ExceptionClear();
                vInfo("JNI exception after:" << event.name);
            }

            // Free any local references
            Jni->PopLocalFrame(NULL);
        }

        vInfo("TalkToJava: Jvm->DetachCurrentThread");
        const jint returnDetach = javaVM->DetachCurrentThread();
        if (returnDetach != JNI_OK) {
            vInfo("javaVM->DetachCurrentThread returned" << returnDetach);
        }
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

    if (vApp == nullptr) {
        vInfo("new AppLocal()");
        new App(jni, activity, this);
        vApp->execute();
    } else {
        // Just update the activity object.
        vInfo("Update AppLocal");
        if (vApp->javaObject() != nullptr)
        {
            jni->DeleteGlobalRef(vApp->javaObject());
        }
        vApp->javaObject() = jni->NewGlobalRef(activity);
    }

    // Send the intent and wait for it to complete.
    VVariantArray args;
    args << fromPackage << uri << json;
    vApp->eventLoop().send("intent", args);
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

VMatrix4f VMainActivity::onNewFrame(VFrame vrFrame)
{
    vInfo("VMainActivity::Frame - default handler called");
    return VMatrix4f();
}

void VMainActivity::configureVrMode(VKernel* kernel)
{
    vInfo("VMainActivity::ConfigureVrMode - default handler called");
}

VMatrix4f VMainActivity::drawEyeView(const int eye, const float fovDegrees)
{
    vInfo("VMainActivity::DrawEyeView - default handler called");
    return VMatrix4f();
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

void VMainActivity::Init(JavaVM *javaVM)
{
    d->javaVM = javaVM;

    auto ThreadStarter = [](void *d)->void *
    {
        int result = pthread_setname_np(pthread_self(), "TalkToJava");
        if (result != 0) {
            vWarn("TalkToJava: pthread_setname_np failed" << strerror(result));
        }
        ((VMainActivity::Private *) d)->exec();
        return NULL;
    };

    // spawn the VR thread
    const int createError = pthread_create(&d->threadHandle, NULL /* default attributes */, ThreadStarter, d);
    if (createError != 0) {
        vFatal("pthread_create returned" << createError);
    } else {
        pthread_setname_np(d->threadHandle, "TalkToJava");
    }
}

VEventLoop &VMainActivity::eventLoop()
{
    return d->eventLoop;
}

NV_NAMESPACE_END

NV_USING_NAMESPACE

extern "C"
{

void Java_com_vrseen_VrActivity_nativeSurfaceChanged(JNIEnv *jni, jclass, jobject surface)
{
    vApp->eventLoop().send("surfaceChanged", surface ? ANativeWindow_fromSurface(jni, surface) : nullptr);
}

void Java_com_vrseen_VrActivity_nativeSurfaceDestroyed(JNIEnv *jni, jclass clazz)
{
    if (vApp == 0)
    {
        // Android may call surfaceDestroyed() after onDestroy().
        vInfo("nativeSurfaceChanged was called after onDestroy. We cannot destroy the surface now because we don't have a valid app pointer.");
        return;
    }

    vApp->eventLoop().send("surfaceDestroyed");
}

void Java_com_vrseen_VrActivity_nativePopup(JNIEnv *, jclass,
        jint width, jint height, jfloat seconds)
{
    VVariantArray args;
    args << width << height << seconds;
    vApp->eventLoop().post("popup", args);
}

jobject Java_com_vrseen_VrActivity_nativeGetPopupSurfaceTexture(JNIEnv *, jclass)
{
    return vApp->dialogTexture()->javaObject;
}

void Java_com_vrseen_VrActivity_nativePause(JNIEnv *jni, jclass clazz,
        jlong appPtr)
{
    vApp->eventLoop().send("pause");
}

void Java_com_vrseen_VrActivity_nativeResume(JNIEnv *jni, jclass clazz)
{
    vApp->eventLoop().send("resume");
}

void Java_com_vrseen_VrActivity_nativeDestroy(JNIEnv *, jclass)
{
    vApp->quit();
    delete vApp;

    vInfo("ExitOnDestroy is true, exiting");
    VKernel::instance()->destroy(EXIT_TYPE_EXIT);
    exit(0);
}

void Java_com_vrseen_VrActivity_nativeJoypadAxis(JNIEnv *jni, jclass clazz,
        jfloat lx, jfloat ly, jfloat rx, jfloat ry)
{
    // Suspend input until OneTimeInit() has finished to avoid overflowing the message queue on long loads.
    if (vApp->isRunning()) {
        VVariantArray args;
        args << lx << ly << rx << ry;
        vApp->eventLoop().post("joy", args);
    }
}

void Java_com_vrseen_VrActivity_nativeTouch(JNIEnv *, jclass,
        jint action, jfloat x, jfloat y)
{
    // Suspend input until OneTimeInit() has finished to avoid overflowing the message queue on long loads.
    if (vApp->isRunning()) {
        VVariantArray args;
        args << action << x << y;
        vApp->eventLoop().post("touch", args);
    }
}

void Java_com_vrseen_VrActivity_nativeKeyEvent(JNIEnv *jni, jclass clazz,
        jint key, jboolean down, jint repeatCount)
{
    // Suspend input until OneTimeInit() has finished to avoid overflowing the message queue on long loads.
    if (vApp->isRunning()) {
        VVariantArray args;
        args << key << down << repeatCount;
        vApp->eventLoop().post("key", args);
    }
}

void Java_com_vrseen_VrActivity_nativeNewIntent(JNIEnv *jni, jclass clazz,
        jstring fromPackageName, jstring command, jstring uriString)
{
    VString packageName = JniUtils::Convert(jni, fromPackageName);
    VString uri = JniUtils::Convert(jni, uriString);
    VString json = JniUtils::Convert(jni, command);

    VVariantArray args;
    args << packageName << uri << json;
    vApp->eventLoop().post("intent", args);
}

}	// extern "C"
