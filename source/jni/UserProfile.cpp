/************************************************************************************

Filename    :   UserProfile.cpp
Content     :   Container for user profile data.
Created     :   November 10, 2014
Authors     :   Caleb Leak

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#include "UserProfile.h"

#include "json.h"
#include "Android/LogUtils.h"

#include <fstream>

static const char* PROFILE_PATH = "/sdcard/Oculus/userprofile.json";

using namespace NervGear;

namespace NervGear
{

UserProfile LoadProfile()
{
    // TODO: Switch this over to using a content provider when available.


    Json root;
    std::ifstream fp(PROFILE_PATH, std::ios::binary);
    fp >> root;

    UserProfile profile;

    if (root.isInvalid()) {
        WARN("Failed to load user profile \"%s\". Using defaults.", PROFILE_PATH);
    } else {
        profile.Ipd = root.value("ipd").toDouble();
        profile.EyeHeight = root.value("eyeHeight").toDouble();
        profile.HeadModelHeight = root.value("headModelHeight").toDouble();
        profile.HeadModelDepth = root.value("headModelDepth").toDouble();
    }

    return profile;
}

void SaveProfile(const UserProfile& profile)
{
    Json root(Json::Object);

    root["ipd"] = profile.Ipd;
    root["eyeHeight"] = profile.EyeHeight;
    root["headModelHeight"] = profile.HeadModelHeight;
    root["headModelDepth"] = profile.HeadModelDepth;

    std::ofstream fp(PROFILE_PATH, std::ios::binary);
    if (!fp.is_open())
        WARN("Failed to save user profile %s", PROFILE_PATH);
    else
    	fp << root;
}

}	// namespace NervGear
