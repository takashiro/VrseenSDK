#pragma once

#include "vglobal.h"
#include "VString.h"

#include <jni.h>

NV_NAMESPACE_BEGIN

class VOsBuild
{
public:
    enum NameType
    {
        Brand,
        Device,
        Display,
        Fingerprint,
        Hardware,
        Host,
        Id,
        Model,
        Product,
        Serial,
        Tags,
        Type,

        NameTypeCount
    };

    static const VString &getString(NameType name);
};

NV_NAMESPACE_END
