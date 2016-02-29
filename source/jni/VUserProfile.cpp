#include "VUserProfile.h"

#include "json.h"
#include "android/LogUtils.h"

#include <fstream>

NV_NAMESPACE_BEGIN

static const char* PROFILE_PATH = "/sdcard/Oculus/userprofile.json";

void VUserProfile::load()
{
    // TODO: Switch this over to using a content provider when available.
    Json root;
    std::ifstream fp(PROFILE_PATH, std::ios::binary);
    fp >> root;

    if (root.isInvalid()) {
        WARN("Failed to load user profile \"%s\". Using defaults.", PROFILE_PATH);
    } else {
        ipd = root.value("ipd").toDouble();
        eyeHeight = root.value("eyeHeight").toDouble();
        headModelHeight = root.value("headModelHeight").toDouble();
        headModelDepth = root.value("headModelDepth").toDouble();
    }
}

void VUserProfile::save()
{
    Json root(Json::Object);

    root["ipd"] = ipd;
    root["eyeHeight"] = eyeHeight;
    root["headModelHeight"] = headModelHeight;
    root["headModelDepth"] = headModelDepth;

    std::ofstream fp(PROFILE_PATH, std::ios::binary);
    if (!fp.is_open())
        WARN("Failed to save user profile %s", PROFILE_PATH);
    else
        fp << root;
}

NV_NAMESPACE_END
