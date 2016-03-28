#pragma once

#include "vglobal.h"
#include "../api/VLensDistortion.h"		// for LensConfig

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

    float	lensSeparation;		// in meters

    // These values are always as if the display is in landscape
    // mode, being swapped from the system values if the manifest
    // is configured for portrait.
    float	widthMeters;		// in meters
    float	heightMeters;		// in meters
    int		widthPixels;		// in pixels
    int		heightPixels;		// in pixels
    float	horizontalOffsetMeters; // the horizontal offset between the screen center and midpoint between lenses

    // Refresh rate of the display.
    float	displayRefreshRate;

    // Currently returns a conservative 1024x1024
    int		eyeTextureResolution[2];

    float	eyeTextureFov[2];

private:
    VDevice();

    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VDevice)
};

NV_NAMESPACE_END
