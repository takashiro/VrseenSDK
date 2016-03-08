#include "VOsBuild.h"
#include "VLog.h"

NV_NAMESPACE_BEGIN

namespace {
    VString BuildStrings[VOsBuild::NameTypeCount];

    VString GetFieldString(JNIEnv *env, jclass BuildClass, const char * name)
    {
        jfieldID field = env->GetStaticFieldID(BuildClass, name, "Ljava/lang/String;");

        // get reference to the string
        jstring jstr = (jstring) env->GetStaticObjectField(BuildClass, field);
        const jchar *chars = env->GetStringChars(jstr, nullptr);
        jsize length = env->GetStringLength(jstr);

        VString	str;
        str.resize(length);
        for (jsize i = 0; i < length; i++) {
            str[i] = chars[i];
        }

        env->ReleaseStringChars(jstr, chars);
        env->DeleteLocalRef(jstr);

        return str;
    }
}

const VString &VOsBuild::getString(NameType name)
{
    return BuildStrings[name];
}

void VOsBuild::Init(JNIEnv *env)
{
    // find the class
    jclass buildClass = env->FindClass("android/os/Build");

    // find and copy out the fields
    BuildStrings[Brand] = GetFieldString(env, buildClass, "BRAND");
    BuildStrings[Device] = GetFieldString(env, buildClass, "DEVICE");
    BuildStrings[Display] = GetFieldString(env, buildClass, "DISPLAY");
    BuildStrings[Fingerprint] = GetFieldString(env, buildClass, "FINGERPRINT");
    BuildStrings[Hardware] = GetFieldString(env, buildClass, "HARDWARE");
    BuildStrings[Host] = GetFieldString(env, buildClass, "HOST");
    BuildStrings[Id] = GetFieldString(env, buildClass, "ID");
    BuildStrings[Model] = GetFieldString(env, buildClass, "MODEL");
    BuildStrings[Product] = GetFieldString(env, buildClass, "PRODUCT");
    BuildStrings[Serial] = GetFieldString(env, buildClass, "SERIAL");
    BuildStrings[Tags] = GetFieldString(env, buildClass, "TAGS");
    BuildStrings[Type] = GetFieldString(env, buildClass, "TYPE");

    // don't pollute the local reference table
    env->DeleteLocalRef(buildClass);
}

NV_NAMESPACE_END

