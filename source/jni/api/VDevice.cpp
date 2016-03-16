#include "VDevice.h"

#include "App.h"
#include "VLog.h"
#include "android/VOsBuild.h"

#include <jni.h>

NV_NAMESPACE_BEGIN

struct VDevice::Private
{
    JNIEnv *uiJni;
    JNIEnv *vrJni;
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
}

NV_NAMESPACE_END
