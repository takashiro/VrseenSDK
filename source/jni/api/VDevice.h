#pragma once

#include "VLensDistortion.h"

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

    VLensDistortion lens;
    float lensDistance;
    float widthbyMeters;
    float heightbyMeters;
    int widthbyPixels;
    int heightbyPixels;
    float xxOffsetbyMeters;
    float refreshRate;
    int eyeDisplayResolution[2];
    float eyeDisplayFov[2];

private:
    VDevice();

    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VDevice)
};

NV_NAMESPACE_END
