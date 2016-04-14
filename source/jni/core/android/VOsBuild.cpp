#include "VOsBuild.h"
#include "VLog.h"
#include "android/JniUtils.h"

NV_NAMESPACE_BEGIN

namespace {
    VString BuildStrings[VOsBuild::NameTypeCount];

    VString GetFieldString(JNIEnv *env, jclass BuildClass, const char * name)
    {
        jfieldID field = env->GetStaticFieldID(BuildClass, name, "Ljava/lang/String;");
        jstring jstr = (jstring) env->GetStaticObjectField(BuildClass, field);
        return JniUtils::Convert(env, jstr);
    }
}

const VString &VOsBuild::getString(NameType name)
{
    return BuildStrings[name];
}

void VOsBuildInit(JavaVM *, JNIEnv *env)
{
    // find the class
    jclass buildClass = env->FindClass("android/os/Build");

    // find and copy out the fields
    BuildStrings[VOsBuild::Brand] = GetFieldString(env, buildClass, "BRAND");
    BuildStrings[VOsBuild::Device] = GetFieldString(env, buildClass, "DEVICE");
    BuildStrings[VOsBuild::Display] = GetFieldString(env, buildClass, "DISPLAY");
    BuildStrings[VOsBuild::Fingerprint] = GetFieldString(env, buildClass, "FINGERPRINT");
    BuildStrings[VOsBuild::Hardware] = GetFieldString(env, buildClass, "HARDWARE");
    BuildStrings[VOsBuild::Host] = GetFieldString(env, buildClass, "HOST");
    BuildStrings[VOsBuild::Id] = GetFieldString(env, buildClass, "ID");
    BuildStrings[VOsBuild::Model] = GetFieldString(env, buildClass, "MODEL");
    BuildStrings[VOsBuild::Product] = GetFieldString(env, buildClass, "PRODUCT");
    BuildStrings[VOsBuild::Serial] = GetFieldString(env, buildClass, "SERIAL");
    BuildStrings[VOsBuild::Tags] = GetFieldString(env, buildClass, "TAGS");
    BuildStrings[VOsBuild::Type] = GetFieldString(env, buildClass, "TYPE");

    // don't pollute the local reference table
    env->DeleteLocalRef(buildClass);
}

NV_REGISTER_JNI_LOADER(VOsBuildInit)

NV_NAMESPACE_END

