#include "VDevice.h"

#include "App.h"
#include "VLog.h"
#include "android/VOsBuild.h"

#include <jni.h>
#include "../core/android/JniUtils.h"

NV_NAMESPACE_BEGIN

struct VDevice::Private
{
    JNIEnv *uiJni;
    JNIEnv *vrJni;
    void InitHmdInfo();
};

VDevice *VDevice::instance()
{
    static VDevice device;
    return &device;
}

VDevice::~VDevice()
{
    delete d;
}

int VDevice::screenBrightness() const
{
    return 0;
}

void VDevice::setScreenBrightness(int brightness)
{
    if (brightness < 0) {
        brightness = 0;
    } else if (brightness > 255) {
        brightness = 255;
    }

    //@to-do:
}

bool VDevice::isComfortMode() const
{
    //@to-do:
    return false;
}

void VDevice::setComfortMode(bool enabled)
{
    (void) enabled;
    //@to-do:
}

bool VDevice::isDoNotDisturbMode() const
{
    //@to-do:
    return false;
}

void VDevice::setDoNotDisturbMode(bool enabled)
{
    (void) enabled;
    //@to-do:
}


VDevice::VDevice()
    : d(new Private)
{
    d->uiJni = vApp->uiJni();
    d->vrJni = vApp->vrJni();

    lens.initDistortionParmsByMobileType();
    refreshRate = 60.0f;
    eyeDisplayResolution[0] = 1024;
    eyeDisplayResolution[1] = 1024;
    eyeDisplayFov[0] = 90.0f;
    eyeDisplayFov[1] = 90.0f;

    // Screen params.
    lensDistance = 0.063f;	// JDC: measured on 8/23/2014
    eyeDisplayFov[0] = 90.0f;
    eyeDisplayFov[1] = 90.0f;

    widthbyMeters = 0.125f;		// not reported correctly by display metrics!
    heightbyMeters = 0.0707f;
}

NV_NAMESPACE_END
