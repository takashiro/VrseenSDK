#pragma once

#include "vglobal.h"

#include "Types.h"

NV_NAMESPACE_BEGIN

class HIDDeviceBase
{
public:

    virtual ~HIDDeviceBase() { }

    virtual bool SetFeatureReport(uchar* data, vuint32 length) = 0;
    virtual bool GetFeatureReport(uchar* data, vuint32 length) = 0;
};

NV_NAMESPACE_END
