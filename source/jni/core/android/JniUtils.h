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

    VString GetPackageCodePath(JNIEnv *jni, jclass activityClass, jobject activityObject);
    VString GetCurrentPackageName(JNIEnv *jni, jobject activityObject);
    VString GetCurrentActivityName(JNIEnv * jni, jobject activityObject);
}

class JavaObject
{
public:
    JavaObject(JNIEnv *jni, const jobject jObject)
        : m_jni(jni)
        , m_jobject(jObject)
    {
        OVR_ASSERT(m_jni != nullptr);
    }

    ~JavaObject()
    {
        if(m_jni->ExceptionOccurred()) {
            LOG( "JNI exception before DeleteLocalRef!" );
            m_jni->ExceptionClear();
        }
        OVR_ASSERT( m_jni != NULL && m_jobject != NULL );
        m_jni->DeleteLocalRef( m_jobject );
        if ( m_jni->ExceptionOccurred() )
        {
            LOG( "JNI exception occured calling DeleteLocalRef!" );
            m_jni->ExceptionClear();
        }
    }

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
    JavaString( JNIEnv * jni, const VString &str) :
        JavaObject( jni, NULL )
    {
        setJObject(JniUtils::Convert(jni, str));
        if (jni->ExceptionOccurred()) {
            LOG( "JNI exception occured calling NewStringUTF!" );
        }
    }

    JavaString(JNIEnv *jni, jstring string)
        : JavaObject(jni, string)
    {
        OVR_ASSERT(string != nullptr);
    }

    jstring toJString() const { return static_cast<jstring>(toJObject()); }
};

NV_NAMESPACE_END

// Use this EVERYWHERE and you can insert your own catch here if you have string references leaking.
// Even better, use the JavaString / JavaUTFChars classes instead and they will free resources for
// you automatically.
jobject ovr_NewStringUTF( JNIEnv * jni, char const * str );

//==============================================================
// JavaObject
//
// Releases a java object reference on destruction

// This must be called by a function called directly from a java thread,
// preferably from JNI_OnLoad().  It will fail if called from a pthread created
// in native code, or from a NativeActivity due to the class-lookup issue:
// http://developer.android.com/training/articles/perf-jni.html#faq_FindClass
jclass		ovr_GetGlobalClassReference( JNIEnv * jni, const char * className );

jmethodID	ovr_GetMethodID( JNIEnv * jni, jclass jniclass, const char * name, const char * signature );
jmethodID	ovr_GetStaticMethodID( JNIEnv * jni, jclass jniclass, const char * name, const char * signature );

void ovr_LoadDevConfig( bool const forceReload );


