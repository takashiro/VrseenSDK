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

static PhoneTypeEnum IdentifyHmdType( const char * buildModel ) {
    if (strcmp(buildModel, "GT-I9506") == 0) {
        return HMD_GALAXY_S4;
    }

    if ((strcmp(buildModel, "SM-G900F") == 0) || (strcmp(buildModel, "SM-G900X") == 0)) {
        return HMD_GALAXY_S5;
    }

    if (strcmp(buildModel, "SM-G906S") == 0) {
        return HMD_GALAXY_S5_WQHD;
    }

    if ((strstr(buildModel, "SM-N910") != NULL) || (strstr(buildModel, "SM-N916") != NULL)) {
        return HMD_NOTE_4;
    }

    LOG("IdentifyHmdType: Model %s not found. Defaulting to Note4", buildModel);
    return HMD_NOTE_4;
}

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

    const char *buildModel = VOsBuild::getString(VOsBuild::Model).toCString();
    PhoneTypeEnum type = IdentifyHmdType( buildModel );

    lens.initLensByPhoneType(type);
    displayRefreshRate = 60.0f;
    eyeTextureResolution[0] = 1024;
    eyeTextureResolution[1] = 1024;
    eyeTextureFov[0] = 90.0f;
    eyeTextureFov[1] = 90.0f;

    // Screen params.
    switch( type )
    {
        case HMD_GALAXY_S4:			// Galaxy S4 in Samsung's holder
            lensSeparation = 0.062f;
            eyeTextureFov[0] = 95.0f;
            eyeTextureFov[1] = 95.0f;
            break;

        case HMD_GALAXY_S5:      // Galaxy S5 1080 paired with version 2 lenses
            lensSeparation = 0.062f;
            eyeTextureFov[0] = 90.0f;
            eyeTextureFov[1] = 90.0f;
            break;

        case HMD_GALAXY_S5_WQHD:            // Galaxy S5 1440 paired with version 2 lenses
            lensSeparation = 0.062f;
            eyeTextureFov[0] = 90.0f;  // 95.0f
            eyeTextureFov[1] = 90.0f;  // 95.0f
            break;

        default:
        case HMD_NOTE_4:      // Note 4
            lensSeparation = 0.063f;	// JDC: measured on 8/23/2014
            eyeTextureFov[0] = 90.0f;
            eyeTextureFov[1] = 90.0f;

            widthMeters = 0.125f;		// not reported correctly by display metrics!
            heightMeters = 0.0707f;
            break;
    }
    delete[] buildModel;
}

NV_NAMESPACE_END
