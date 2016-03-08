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

NV_NAMESPACE_END

// Use this EVERYWHERE and you can insert your own catch here if you have string references leaking.
// Even better, use the JavaString / JavaUTFChars classes instead and they will free resources for
// you automatically.
jobject ovr_NewStringUTF( JNIEnv * jni, char const * str );

//==============================================================
// JavaObject
//
// Releases a java object reference on destruction
class JavaObject
{
public:
	JavaObject( JNIEnv * jni_, jobject const JObject_ ) :
		jni( jni_ ),
		JObject( JObject_ )
	{
		OVR_ASSERT( jni != NULL );
	}
	~JavaObject()
	{
		if ( jni->ExceptionOccurred() )
		{
			LOG( "JNI exception before DeleteLocalRef!" );
			jni->ExceptionClear();
		}
		OVR_ASSERT( jni != NULL && JObject != NULL );
		jni->DeleteLocalRef( JObject );
		if ( jni->ExceptionOccurred() )
		{
			LOG( "JNI exception occured calling DeleteLocalRef!" );
			jni->ExceptionClear();
		}
		jni = NULL;
		JObject = NULL;
	}

	jobject			GetJObject() const { return JObject; }

	JNIEnv *		GetJNI() const { return jni; }

protected:
	void			SetJObject( jobject const & obj ) { JObject = obj; }

private:
	JNIEnv *		jni;
	jobject			JObject;
};

//==============================================================
// JavaClass
class JavaClass : public JavaObject
{
public:
	JavaClass( JNIEnv * jni_, jobject const object ) :
		JavaObject( jni_, object )
	{
	}

	jclass			GetJClass() const { return static_cast< jclass >( GetJObject() ); }
};

//==============================================================
// JavaString
//
// Creates a java string on construction and releases it on destruction.
class JavaString : public JavaObject
{
public:
	JavaString( JNIEnv * jni_, char const * str ) :
		JavaObject( jni_, NULL )
	{
		SetJObject( ovr_NewStringUTF( GetJNI(), str ) );
		if ( GetJNI()->ExceptionOccurred() )
		{
			LOG( "JNI exception occured calling NewStringUTF!" );
		}
	}

	JavaString( JNIEnv * jni_, jstring JString_ ) :
		JavaObject( jni_, JString_ )
	{
		OVR_ASSERT( JString_ != NULL );
	}

	jstring			GetJString() const { return static_cast< jstring >( GetJObject() ); }
};

// This must be called by a function called directly from a java thread,
// preferably from JNI_OnLoad().  It will fail if called from a pthread created
// in native code, or from a NativeActivity due to the class-lookup issue:
// http://developer.android.com/training/articles/perf-jni.html#faq_FindClass
jclass		ovr_GetGlobalClassReference( JNIEnv * jni, const char * className );

jmethodID	ovr_GetMethodID( JNIEnv * jni, jclass jniclass, const char * name, const char * signature );
jmethodID	ovr_GetStaticMethodID( JNIEnv * jni, jclass jniclass, const char * name, const char * signature );

// get the code path of the current package.
void ovr_GetPackageCodePath(JNIEnv * jni, jclass activityClass, jobject activityObject, const NervGear::VString &packageCodePath);

void ovr_LoadDevConfig( bool const forceReload );
const char * ovr_GetHomePackageName( char * packageName, int const maxLen );


