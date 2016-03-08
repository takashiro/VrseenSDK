#include "JniUtils.h"

#include "Std.h"
#include "VJson.h"
#include "VLog.h"

#include <fstream>

NV_NAMESPACE_BEGIN

namespace JniUtils {
    VString Convert(JNIEnv *jni, jstring jstr)
    {
        VString str;
        jsize length = jni->GetStringLength(jstr);
        if (length > 0) {
            str.resize(length);
            const jchar *chars = jni->GetStringChars(jstr, NULL);
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
        jstring jstr = jni->NewString(chars, len);
        delete[] chars;
        return jstr;
    }

    VString GetPackageCodePath(JNIEnv *jni, jclass activityClass, jobject activityObject)
    {
        jmethodID getPackageCodePathId = jni->GetMethodID( activityClass, "getPackageCodePath", "()Ljava/lang/String;" );
        if (getPackageCodePathId == 0) {
            vInfo("Failed to find getPackageCodePath on class" << (ulonglong) activityClass << ", object" << (ulonglong) activityObject);
            return VString();
        }

        VString packageCodePath = Convert(jni, (jstring) jni->CallObjectMethod(activityObject, getPackageCodePathId));
        if (!jni->ExceptionOccurred()) {
            vInfo("GetPackageCodePath() =" << packageCodePath);
            return packageCodePath;
        } else {
            jni->ExceptionClear();
            vInfo("Cleared JNI exception");
        }

        return VString();
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
}

NV_NAMESPACE_END

jclass ovr_GetGlobalClassReference( JNIEnv * jni, const char * className )
{
	jclass lc = jni->FindClass(className);
	if ( lc == 0 )
	{
		FAIL( "FindClass( %s ) failed", className );
	}
	// Turn it into a global ref, so we can safely use it in other threads
	jclass gc = (jclass)jni->NewGlobalRef( lc );

	jni->DeleteLocalRef( lc );

	return gc;
}

jmethodID ovr_GetMethodID( JNIEnv * jni, jclass jniclass, const char * name, const char * signature )
{
	const jmethodID methodId = jni->GetMethodID( jniclass, name, signature );
	if ( !methodId )
	{
		FAIL( "couldn't get %s, %s", name, signature );
	}
	return methodId;
}

jmethodID ovr_GetStaticMethodID( JNIEnv * jni, jclass jniclass, const char * name, const char * signature )
{
	const jmethodID method = jni->GetStaticMethodID( jniclass, name, signature );
	if ( !method )
	{
		FAIL( "couldn't get %s, %s", name, signature );
	}
	return method;
}

static NervGear::Json *DevConfig = NULL;

void ovr_LoadDevConfig( bool const forceReload )
{
#if !defined( RETAIL )
	if ( DevConfig != NULL )
	{
		if ( !forceReload )
		{
			return;	// already loading and not forcing a reload
		}
		delete DevConfig;
		DevConfig = NULL;
	}

	// load the dev config if possible
	std::ifstream fp("/storage/extSdCard/Oculus/dev.cfg", std::ios::binary);
	if (fp.is_open())
	{
		DevConfig = new NervGear::Json;
		fp >> (*DevConfig);
	}
#endif
}
