#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

class VDevice
{
public:
    static VDevice *instance();
    ~VDevice();

    int screenBrightness() const;
    void setScreenBrightness(int brightness);

    bool isComfortMode() const;
    void setComfortMode(bool enabled);

    bool isDoNotDisturbMode() const;
    void setDoNotDisturbMode(bool enabled);

private:
    VDevice();

    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VDevice)
};

NV_NAMESPACE_END
