#include "JniUtils.h"
#include "VJson.h"
#include "VLog.h"

#include <fstream>
#include <list>

NV_NAMESPACE_BEGIN

namespace JniUtils {
    VString Convert(JNIEnv *jni, jstring jstr)
    {
        VString str;
        jsize length = jni->GetStringLength(jstr);
        if (length > 0) {
            str.resize(length);
            const jchar *chars = jni->GetStringChars(jstr, nullptr);
            for (jsize i = 0; i < length; i++) {
                str[i] = chars[i];
            }
            jni->ReleaseStringChars(jstr, chars);
        }
        return str;
    }

    jstring Convert(JNIEnv *jni, const VString &str)
    {
        jsize len = str.size();
        jchar *chars = new jchar[len];
        for (jsize i = 0; i < len; i++) {
            chars[i] = str[i];
        }
        jstring jstr = jni->NewString(chars, len);
        delete[] chars;
        return jstr;
    }

    VString GetCurrentPackageName(JNIEnv * jni, jobject activityObject)
    {
        JavaClass curActivityClass( jni, jni->GetObjectClass( activityObject ) );
        jmethodID getPackageNameId = jni->GetMethodID( curActivityClass.toJClass(), "getPackageName", "()Ljava/lang/String;");
        if ( getPackageNameId != 0 )
        {
            VString packageName = Convert(jni, (jstring) jni->CallObjectMethod(activityObject, getPackageNameId));
            if (!jni->ExceptionOccurred()) {
                vInfo("GetCurrentPackageName() =" << packageName);
                return packageName;
            } else {
                jni->ExceptionClear();
                vInfo("Cleared JNI exception");
            }
        }
        return VString();
    }

    VString GetCurrentActivityName(JNIEnv *jni, jobject activityObject)
    {
        JavaClass curActivityClass( jni, jni->GetObjectClass( activityObject ) );
        jmethodID getClassMethodId = jni->GetMethodID(curActivityClass.toJClass(), "getClass", "()Ljava/lang/Class;" );
        if (getClassMethodId != 0) {
            JavaObject classObj(jni, jni->CallObjectMethod(activityObject, getClassMethodId));
            JavaClass activityClass(jni, jni->GetObjectClass(classObj.toJObject()));

            jmethodID getNameMethodId = jni->GetMethodID(activityClass.toJClass(), "getName", "()Ljava/lang/String;" );
            if (getNameMethodId != 0) {
                VString name = Convert(jni, (jstring)jni->CallObjectMethod(classObj.toJObject(), getNameMethodId));
                vInfo("GetCurrentActivityName() =" << name);
                return name;
            }
        }
        return VString();
    }

    jclass GetGlobalClassReference( JNIEnv * jni, const char * className )
    {
        jclass lc = jni->FindClass(className);
        if (lc == 0) {
            vFatal("FindClass(" << className << ") failed");
        }

        jclass gc = (jclass)jni->NewGlobalRef( lc );
        jni->DeleteLocalRef( lc );
        return gc;
    }

    jmethodID GetMethodID( JNIEnv * jni, jclass jniclass, const char * name, const char * signature )
    {
        const jmethodID methodId = jni->GetMethodID( jniclass, name, signature );
        if (!methodId) {
            vFatal("couldn't get" << name << signature);
        }
        return methodId;
    }

    jmethodID GetStaticMethodID( JNIEnv * jni, jclass jniclass, const char * name, const char * signature )
    {
        const jmethodID method = jni->GetStaticMethodID( jniclass, name, signature );
        if (!method) {
            vFatal("couldn't get" << name << signature);
        }
        return method;
    }

    std::list<Loader> &Loaders()
    {
        static std::list<Loader> loaders;
        return loaders;
    }

    void RegisterLoader(Loader loader)
    {
        Loaders().push_back(loader);
    }

    JavaVM *GlobalJavaVM = nullptr;
    JavaVM *GetJavaVM()
    {
        return GlobalJavaVM;
    }
}

extern "C" {
    JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *)
    {
        JniUtils::GlobalJavaVM = vm;

        JNIEnv *jni;
        bool privateEnv = false;
        if (JNI_OK != vm->GetEnv(reinterpret_cast<void**>(&jni), JNI_VERSION_1_6)) {
            privateEnv = true;
            const jint result = vm->AttachCurrentThread(&jni, 0);
            vAssert(result != JNI_OK);
        }

        const std::list<JniUtils::Loader> &loaders = JniUtils::Loaders();
        for (JniUtils::Loader loader : loaders) {
            loader(vm, jni);
        }

        if (privateEnv) {
            vm->DetachCurrentThread();
        }

        return JNI_VERSION_1_6;
    }
}

JavaObject::JavaObject(JNIEnv *jni, const jobject jObject)
    : m_jni(jni)
    , m_jobject(jObject)
{
}

JavaObject::~JavaObject()
{
    if(m_jni->ExceptionOccurred()) {
        vError("JNI exception before DeleteLocalRef!");
        m_jni->ExceptionClear();
    }
    m_jni->DeleteLocalRef(m_jobject);
    if (m_jni->ExceptionOccurred()) {
        vError("JNI exception occured calling DeleteLocalRef!");
        m_jni->ExceptionClear();
    }
}

JavaString::JavaString(JNIEnv *jni, const VString &str)
    : JavaObject( jni, nullptr )
{
    setJObject(JniUtils::Convert(jni, str));
    if (jni->ExceptionOccurred()) {
        vError("JNI exception occured calling NewStringUTF!");
    }
}

NV_NAMESPACE_END
