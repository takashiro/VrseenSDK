#pragma once

#include "vglobal.h"

#include "Types.h"

NV_NAMESPACE_BEGIN

class HIDDeviceBase
{
public:

    virtual ~HIDDeviceBase() { }

    virtual bool SetFeatureReport(UByte* data, UInt32 length) = 0;
    virtual bool GetFeatureReport(UByte* data, UInt32 length) = 0;
};

NV_NAMESPACE_END
