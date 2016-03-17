#pragma once

#include "vglobal.h"

#include <jni.h>

NV_NAMESPACE_BEGIN

class VMainActivity
{
public:
    VMainActivity(JNIEnv *jni, jobject activityObject);
    ~VMainActivity();

    void finishActivity();

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VMainActivity)
};

NV_NAMESPACE_END
