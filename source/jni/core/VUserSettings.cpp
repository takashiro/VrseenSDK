#include "VUserSettings.h"

#include "VJson.h"
#include "VLog.h"

#include <fstream>

NV_NAMESPACE_BEGIN

static const char* PROFILE_PATH = "/sdcard/Oculus/userprofile.json";

void VUserSettings::load()
{
    // TODO: Switch this over to using a content provider when available.
    VJson root;
    std::ifstream fp(PROFILE_PATH, std::ios::binary);
    if (fp.is_open()) {
        fp >> root;
    }

    if (root.isNull()) {
        vWarn("Failed to load user profile \"" << PROFILE_PATH << "\". Using defaults.");
    } else {
        ipd = root.value("ipd").toDouble();
        eyeHeight = root.value("eyeHeight").toDouble();
        headModelHeight = root.value("headModelHeight").toDouble();
        headModelDepth = root.value("headModelDepth").toDouble();
    }
}

void VUserSettings::save()
{
    VJsonObject root;
    root.insert("ipd", ipd);
    root.insert("eyeHeight", eyeHeight);
    root.insert("headModelHeight", headModelHeight);
    root.insert("headModelDepth", headModelDepth);

    std::ofstream fp(PROFILE_PATH, std::ios::binary);
    if (!fp.is_open()) {
        vWarn("Failed to save user profile" << PROFILE_PATH);
    } else {
        fp << VJson(std::move(root));
    }
}

NV_NAMESPACE_END
