#pragma once

#include "vglobal.h"

#include <jni.h>

#include "Types.h"
#include "LogUtils.h"

#include "VString.h"

NV_NAMESPACE_BEGIN

namespace JniUtils {
    VString Convert(JNIEnv *jni, jstring jstr);
    jstring Convert(JNIEnv *jni, const VString &str);
    inline jstring Convert(JNIEnv *jni, char const * str) { return jni->NewStringUTF(str); }

    VString GetPackageCodePath(JNIEnv *jni, jclass activityClass, jobject activityObject);
    VString GetCurrentPackageName(JNIEnv *jni, jobject activityObject);
    VString GetCurrentActivityName(JNIEnv * jni, jobject activityObject);

    jclass GetGlobalClassReference(JNIEnv *jni, const char *className);
    jmethodID GetMethodID(JNIEnv *jni, jclass jniclass, const char *name, const char *signature);
    jmethodID GetStaticMethodID(JNIEnv *jni, jclass jniclass, const char *name, const char *signature);
    void LoadDevConfig(bool const forceReload);
}

class JavaObject
{
public:
    JavaObject(JNIEnv *jni, const jobject jObject);
    ~JavaObject();

    jobject toJObject() const { return m_jobject; }
    JNIEnv *jni() const { return m_jni; }

protected:
    void setJObject(const jobject &obj) { m_jobject = obj; }

private:
    JNIEnv *m_jni;
    jobject m_jobject;
};

class JavaClass : public JavaObject
{
public:
    JavaClass(JNIEnv * jni, const jobject object)
        : JavaObject(jni, object)
    {
    }

    jclass toJClass() const { return static_cast<jclass>(toJObject()); }
};

class JavaString : public JavaObject
{
public:
    JavaString( JNIEnv * jni, const VString &str);

    JavaString(JNIEnv *jni, jstring string)
        : JavaObject(jni, string)
    {
    }

    jstring toJString() const { return static_cast<jstring>(toJObject()); }
};

NV_NAMESPACE_END
