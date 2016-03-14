/************************************************************************************

PublicHeader:   None
Filename    :   OVR_Profile.cpp
Content     :   Structs and functions for loading and storing device profile settings
Created     :   February 14, 2013
Notes       :

   Profiles are used to store per-user settings that can be transferred and used
   across multiple applications.  For example, player IPD can be configured once
   and reused for a unified experience across games.  Configuration and saving of profiles
   can be accomplished in game via the Profile API or by the official Oculus Configuration
   Utility.

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#include "Profile.h"

#include "VJson.h"
#include "Types.h"
#include "SysFile.h"
//#include "VSysFile.h"
#include "Allocator.h"
#include "Array.h"
#include <fstream>

#ifdef OVR_OS_WIN32
#include <Shlobj.h>
#else
#include <dirent.h>
#include <sys/stat.h>

#ifdef OVR_OS_LINUX
#include <unistd.h>
#include <pwd.h>
#endif

#endif

#define PROFILE_VERSION 1.0
#define MAX_PROFILE_MAJOR_VERSION 1

using namespace NervGear;

namespace NervGear {

//-----------------------------------------------------------------------------
// Returns the pathname of the JSON file containing the stored profiles
VString GetBaseOVRPath(bool create_dir)
{
    VString path;

#if defined(OVR_OS_WIN32)

    TCHAR data_path[MAX_PATH];
    SHGetFolderPath(0, CSIDL_LOCAL_APPDATA, NULL, 0, data_path);
    path = String(data_path);

    path += "/Oculus";

    if (create_dir)
    {   // Create the Oculus directory if it doesn't exist
        WCHAR wpath[128];
        NervGear::UTF8Util::DecodeString(wpath, path.toCString());

        DWORD attrib = GetFileAttributes(wpath);
        bool exists = attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY);
        if (!exists)
        {
            CreateDirectory(wpath, NULL);
        }
    }

#elif defined(OVR_OS_MAC)

    const char* home = getenv("HOME");
    path = home;
    path += "/Library/Preferences/Oculus";

    if (create_dir)
    {   // Create the Oculus directory if it doesn't exist
        DIR* dir = opendir(path);
        if (dir == NULL)
        {
            mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);
        }
        else
        {
            closedir(dir);
        }
    }

#elif defined(OVR_OS_ANDROID)

    // TODO: We probably should use the location of Environment.getExternalStoragePublicDirectory()
    const char* home = "/sdcard";
    path = home;
    path += "/Oculus";

    if (create_dir)
    {   // Create the Oculus directory if it doesn't exist
        DIR* dir = opendir(path.toCString());
        if (dir == NULL)
        {
            mkdir(path.toCString(), S_IRWXU | S_IRWXG | S_IRWXO);
        }
        else
        {
            closedir(dir);
        }
    }

#else

    // Note that getpwuid is not safe - it sometimes returns NULL for users from LDAP.
    const char* home = getenv("HOME");
    path = home;
    path += "/.config/Oculus";

    if (create_dir)
    {   // Create the Oculus directory if it doesn't exist
        DIR* dir = opendir(path);
        if (dir == NULL)
        {
            mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);
        }
        else
        {
            closedir(dir);
        }
    }

#endif

    return path;
}

VString GetProfilePath(bool create_dir)
{
    VString path = GetBaseOVRPath(create_dir);
    path += "/Profiles.json";
    return path;
}

//-----------------------------------------------------------------------------
// ***** ProfileManager

ProfileManager::ProfileManager()
{
    Changed = false;
    CacheDevice = Profile_Unknown;
}

ProfileManager::~ProfileManager()
{
    // If the profiles have been altered then write out the profile file
    if (Changed)
        SaveCache();

    ClearCache();
}

ProfileManager* ProfileManager::Create()
{
    return new ProfileManager();
}

Profile* ProfileManager::CreateProfileObject(const char* user,
                                             ProfileType device,
                                             const char** device_name)
{
    Lock::Locker lockScope(&ProfileLock);

    Profile* profile = NULL;
    switch (device)
    {
        case Profile_GenericHMD:
            *device_name = NULL;
            profile = new HMDProfile(Profile_GenericHMD, user);
            break;
        case Profile_RiftDK1:
            *device_name = "RiftDK1";
            profile = new RiftDK1Profile(user);
            break;
        case Profile_RiftDKHD:
            *device_name = "RiftDKHD";
            profile = new RiftDKHDProfile(user);
            break;
        case Profile_Unknown:
            break;
    }

    return profile;
}


// Clear the local profile cache
void ProfileManager::ClearCache()
{
    Lock::Locker lockScope(&ProfileLock);

    ProfileCache.clear();
    CacheDevice = Profile_Unknown;
}

// Poplulates the local profile cache.  This occurs on the first access of the profile
// data.  All profile operations are performed against the local cache until the
// ProfileManager is released or goes out of scope at which time the cache is serialized
// to disk.
void ProfileManager::LoadCache(ProfileType device)
{
    Lock::Locker lockScope(&ProfileLock);

    ClearCache();

    VString path = GetProfilePath(false);

    Json root = Json::Load(path.toCString());
    if (!root.isObject() || root.size() < 3)
        return;

    if (!root.contains("Oculus Profile Version"))
        return;

    const Json &item0 = root.value("Oculus Profile Version");
    int major = item0.toInt();
    if (major > MAX_PROFILE_MAJOR_VERSION)
        return;   // don't parse the file on unsupported major version number

    DefaultProfile = root.value("CurrentProfile").toString();

    // Read a number of profiles
    const Json &profileArray = root.value("Profiles");
    const JsonArray &profiles = profileArray.toArray();
    for (const Json &profileItem : profiles) {
        // Read the required Name field
        if (!profileItem.contains("Name"))
            return;// invalid field

        const std::string profileName = profileItem.value("Name").toString();

        const char*   deviceName  = 0;
        bool          deviceFound = false;
        Ptr<Profile>  profile     = *CreateProfileObject(profileName.c_str(), device, &deviceName);

        // Read the base profile fields.
        if (profile && profileItem.isObject())
        {
            const JsonObject &map = profileItem.toObject();
            for (const std::pair<std::string, Json> &i : map) {
                const Json &item = i.second;
                if (!item.isObject()) {
                    profile->ParseProperty(i.first.c_str(), i.second.toString().c_str());
                } else {   // Search for the matching device to get device specific fields
                    if (!deviceFound && deviceName && i.first == deviceName) {
                        deviceFound = true;

                        const JsonObject &deviceObject = item.toObject();
                        for (const std::pair<std::string, Json> &i : deviceObject) {
                            profile->ParseProperty(i.first.c_str(), i.second.toString().c_str());
                        }
                    }
                }
            }
        }

        // Add the new profile
        ProfileCache.append(profile);
    }

    CacheDevice = device;
}

// Serializes the profiles to disk.
void ProfileManager::SaveCache()
{
	VString path = GetProfilePath(true);

    Lock::Locker lockScope(&ProfileLock);

    Json oldroot = Json::Load(path.toCString());
    if (oldroot.isObject()) {
        if (oldroot.size() >= 3) {
            const Json &item0 = oldroot.value("Oculus Profile Version");
            //JSON* item1 = oldroot->nextItem(item0);
            //oldroot->nextItem(item1);

            int major = item0.toInt();
            if (major > MAX_PROFILE_MAJOR_VERSION)
                oldroot.clear();   // don't use the file on unsupported major version number
        } else {
            oldroot.clear();
        }
    }

    // Create a new json root
    Json root(Json::Object);
    root.insert("Oculus Profile Version", PROFILE_VERSION);
    root.insert("CurrentProfile", DefaultProfile);

    Json profiles(Json::Array);
    // Generate a JSON subtree for each profile
    for (unsigned int i = 0; i < ProfileCache.size(); i++) {
        Profile* profile = ProfileCache[i];

        // Write the base profile information
        Json json_profile(Json::Object);
        //json_profile->Name = "Profile";
        json_profile.insert("Name", profile->Name);
        if (profile->CloudUser != NULL) {
            json_profile.insert("CloudUser", profile->CloudUser);
        }
        const char* gender;
        switch (profile->GetGender())
        {
            case Profile::Gender_Male:   gender = "Male"; break;
            case Profile::Gender_Female: gender = "Female"; break;
            default: gender = "Unspecified"; break;
        }
        json_profile.insert("Gender", gender);
        json_profile.insert("PlayerHeight", profile->PlayerHeight);
        json_profile.insert("IPD", profile->IPD);
        json_profile.insert("NeckEyeHori", profile->NeckEyeHori);
        json_profile.insert("NeckEyeVert", profile->NeckEyeVert);

        const char* device_name = NULL;
        // Create a device-specific subtree for the cached device
        if (profile->Type == Profile_RiftDK1)
        {
            device_name = "RiftDK1";

            RiftDK1Profile* rift = (RiftDK1Profile*)profile;
            Json json_rift(Json::Object);

            const char* eyecup = "A";
            switch (rift->EyeCups)
            {
            case EyeCup_BlackA:  eyecup = "A"; break;
            case EyeCup_BlackB:  eyecup = "B"; break;
            case EyeCup_BlackC:  eyecup = "C"; break;
            case EyeCup_OrangeA: eyecup = "Orange A"; break;
            case EyeCup_RedA:    eyecup = "Red A"; break;
            case EyeCup_BlueA:   eyecup = "Blue A"; break;
            default: OVR_ASSERT ( false ); break;
            }
            json_rift.insert("EyeCup", std::string(eyecup));
            json_rift.insert("LL", rift->LL);
            json_rift.insert("LR", rift->LR);
            json_rift.insert("RL", rift->RL);
            json_rift.insert("RR", rift->RR);

            json_profile.insert(device_name, json_rift);
        }
        else if (profile->Type == Profile_RiftDKHD)
        {
            device_name = "RiftDKHD";

            RiftDKHDProfile* rift = (RiftDKHDProfile*)profile;
            Json json_rift(Json::Object);
            json_profile.insert(device_name, json_rift);

            const char* eyecup = "A";
            switch (rift->EyeCups)
            {
            case EyeCup_BlackA:  eyecup = "A"; break;
            case EyeCup_BlackB:  eyecup = "B"; break;
            case EyeCup_BlackC:  eyecup = "C"; break;
            case EyeCup_OrangeA: eyecup = "Orange A"; break;
            case EyeCup_RedA:    eyecup = "Red A"; break;
            case EyeCup_BlueA:   eyecup = "Blue A"; break;
            default: OVR_ASSERT ( false ); break;
            }
            json_rift.insert("EyeCup", eyecup);
            //json_rift->AddNumberItem("LL", rift->LL);
            //json_rift->AddNumberItem("LR", rift->LR);
            //json_rift->AddNumberItem("RL", rift->RL);
            //json_rift->AddNumberItem("RR", rift->RR);
        }

        // There may be multiple devices stored per user, but only a single
        // device is represented by this root.  We don't want to overwrite
        // the other devices so we need to examine the older root
        // and merge previous devices into new json root
        Json old_profile_item = oldroot.value("Profiles");
        const JsonArray &old_profiles = old_profile_item.toArray();
        for (const Json &old_profile : old_profiles) {
            const Json &profile_name = old_profile.value("Name");
            if (profile_name.isString() && profile->Name == profile_name.toString()) {
                // Now that we found the user in the older root, add all the
                // object children to the new root - except for the one for the
                // current device
                const JsonObject &old_item_object = old_profile.toObject();
                for (const std::pair<std::string, Json> &pair : old_item_object) {
                    const Json &old_item = pair.second;
                    if (old_item.isObject() && (device_name == NULL || pair.first != device_name)) {
                        // add the node pointer to the new root
                        json_profile.insert(pair.first, old_item);
                    }
                }

                break;
            }
        }

        // Add the completed user profile to the new root
        profiles.append(json_profile);
    }

    root.insert("Profiles", profiles);

    // Save the profile to disk
    std::ofstream fp(path.toCString(), std::ios::binary);
    fp << root;
}

// Returns the number of stored profiles for this device type
int ProfileManager::GetProfileCount(ProfileType device)
{
    Lock::Locker lockScope(&ProfileLock);

    if (CacheDevice == Profile_Unknown)
        LoadCache(device);

    return (int)ProfileCache.size();
}

// Returns the profile name of a specific profile in the list.  The returned
// memory is locally allocated and should not be stored or deleted.  Returns NULL
// if the index is invalid
const char* ProfileManager::GetProfileName(ProfileType device, unsigned int index)
{
    Lock::Locker lockScope(&ProfileLock);

    if (CacheDevice == Profile_Unknown)
        LoadCache(device);

    if (index < ProfileCache.size())
    {
        Profile* profile = ProfileCache[index];
        OVR_strcpy(NameBuff, Profile::MaxNameLen, profile->Name);
        return NameBuff;
    }
    else
    {
        return NULL;
    }
}

bool ProfileManager::HasProfile(ProfileType device, const char* name)
{
    Lock::Locker lockScope(&ProfileLock);

    if (CacheDevice == Profile_Unknown)
        LoadCache(device);

    for (unsigned i = 0; i< ProfileCache.size(); i++)
    {
        if (ProfileCache[i] && strcmp(ProfileCache[i]->Name, name) == 0)
            return true;
    }
    return false;
}


// Returns a specific profile object in the list.  The returned memory should be
// encapsulated in a Ptr<> object or released after use.  Returns NULL if the index
// is invalid
Profile* ProfileManager::LoadProfile(ProfileType device, unsigned int index)
{
    Lock::Locker lockScope(&ProfileLock);

    if (CacheDevice == Profile_Unknown)
        LoadCache(device);

    if (index < ProfileCache.size())
    {
        Profile* profile = ProfileCache[index];
        return profile->Clone();
    }
    else
    {
        return NULL;
    }
}

// Returns a profile object for a particular device and user name.  The returned
// memory should be encapsulated in a Ptr<> object or released after use.  Returns
// NULL if the profile is not found
Profile* ProfileManager::LoadProfile(ProfileType device, const char* user)
{
    if (user == NULL)
        return NULL;

    Lock::Locker lockScope(&ProfileLock);

    if (CacheDevice == Profile_Unknown)
        LoadCache(device);

    for (unsigned int i=0; i<ProfileCache.size(); i++)
    {
        if (strcmp(user, ProfileCache[i]->Name) == 0)
        {   // Found the requested user profile
            Profile* profile = ProfileCache[i];
            return profile->Clone();
        }
    }

    return NULL;
}

// Returns a profile with all system default values
Profile* ProfileManager::GetDeviceDefaultProfile(ProfileType device)
{
    const char* device_name = NULL;
    return CreateProfileObject("default", device, &device_name);
}

// Returns the name of the profile that is marked as the current default user.
const char* ProfileManager::GetDefaultProfileName(ProfileType device)
{
    Lock::Locker lockScope(&ProfileLock);

    if (CacheDevice == Profile_Unknown)
        LoadCache(device);

    if (ProfileCache.size() > 0)
    {
        OVR_strcpy(NameBuff, Profile::MaxNameLen, DefaultProfile.c_str());
        return NameBuff;
    }
    else
    {
        return NULL;
    }
}

// Marks a particular user as the current default user.
bool ProfileManager::SetDefaultProfileName(ProfileType device, const char* name)
{
    Lock::Locker lockScope(&ProfileLock);

    if (CacheDevice == Profile_Unknown)
        LoadCache(device);
// TODO: I should verify that the user is valid
    if (ProfileCache.size() > 0)
    {
        DefaultProfile = name;
        Changed = true;
        return true;
    }
    else
    {
        return false;
    }
}


// Saves a new or existing profile.  Returns true on success or false on an
// invalid or failed save.
bool ProfileManager::Save(const Profile* profile)
{
    Lock::Locker lockScope(&ProfileLock);

    if (strcmp(profile->Name, "default") == 0)
        return false;  // don't save a default profile

    // TODO: I should also verify that this profile type matches the current cache
    if (CacheDevice == Profile_Unknown)
        LoadCache(profile->Type);

    // Look for the pre-existence of this profile
    bool added = false;
    for (unsigned int i=0; i<ProfileCache.size(); i++)
    {
        int compare = strcmp(profile->Name, ProfileCache[i]->Name);

        if (compare == 0)
        {
            // TODO: I should do a proper field comparison to avoid unnecessary
            // overwrites and file saves

            // Replace the previous instance with the new profile
            ProfileCache[i] = *profile->Clone();
            added   = true;
            Changed = true;
            break;
        }
    }

    if (!added)
    {
        ProfileCache.append(*profile->Clone());
        if (ProfileCache.size() == 1)
            CacheDevice = profile->Type;

        Changed = true;
    }

    return true;
}

// Removes an existing profile.  Returns true if the profile was found and deleted
// and returns false otherwise.
bool ProfileManager::Delete(const Profile* profile)
{
    Lock::Locker lockScope(&ProfileLock);

    if (strcmp(profile->Name, "default") == 0)
        return false;  // don't delete a default profile

    if (CacheDevice == Profile_Unknown)
        LoadCache(profile->Type);

    // Look for the existence of this profile
    for (unsigned int i=0; i<ProfileCache.size(); i++)
    {
        if (strcmp(profile->Name, ProfileCache[i]->Name) == 0)
        {
            if (profile->Name == DefaultProfile)
                DefaultProfile.clear();

            ProfileCache.removeAt(i);
            Changed = true;
            return true;
        }
    }

    return false;
}



//-----------------------------------------------------------------------------
// ***** Profile

Profile::Profile(ProfileType device, const char* name)
{
    Type         = device;
    Gender       = Gender_Unspecified;
    PlayerHeight = 1.778f;    // 5'10" inch man
    IPD          = 0.064f;
    NeckEyeHori  = 0.12f;
    NeckEyeVert  = 0.12f;

    OVR_strcpy(Name, MaxNameLen, name);
    OVR_strcpy(CloudUser, MaxNameLen, name);
}


bool Profile::ParseProperty(const char* prop, const char* sval)
{
    if (strcmp(prop, "Name") == 0)
    {
        OVR_strcpy(Name, MaxNameLen, sval);
        return true;
    }
    else if (strcmp(prop, "CloudUser") == 0)
        {
            OVR_strcpy(CloudUser, MaxNameLen, sval);
            return true;
        }
    else if (strcmp(prop, "Gender") == 0)
    {
        if (strcmp(sval, "Male") == 0)
            Gender = Gender_Male;
        else if (strcmp(sval, "Female") == 0)
            Gender = Gender_Female;
        else
            Gender = Gender_Unspecified;

        return true;
    }
    else if (strcmp(prop, "PlayerHeight") == 0)
    {
        PlayerHeight = (float)atof(sval);
        return true;
    }
    else if (strcmp(prop, "IPD") == 0)
    {
        IPD = (float)atof(sval);
        return true;
    }
    else if (strcmp(prop, "NeckEyeHori") == 0)
    {
        NeckEyeHori = (float)atof(sval);
        return true;
    }
    else if (strcmp(prop, "NeckEyeVert") == 0)
    {
        NeckEyeVert = (float)atof(sval);
        return true;
    }

    return false;
}


// Computes the eye height from the metric head height
float Profile::GetEyeHeight() const
{
    const float EYE_TO_HEADTOP_RATIO =   0.44538f;
    const float MALE_AVG_HEAD_HEIGHT =   0.232f;
    const float FEMALE_AVG_HEAD_HEIGHT = 0.218f;

    // compute distance from top of skull to the eye
    float head_height;
    if (Gender == Gender_Female)
        head_height = FEMALE_AVG_HEAD_HEIGHT;
    else
        head_height = MALE_AVG_HEAD_HEIGHT;

    float skull = EYE_TO_HEADTOP_RATIO * head_height;

    float eye_height  = PlayerHeight - skull;
    return eye_height;
}

//-----------------------------------------------------------------------------
// ***** HMDProfile

HMDProfile::HMDProfile(ProfileType type, const char* name) : Profile(type, name)
{
    LL = 0;
    LR = 0;
    RL = 0;
    RR = 0;
    EyeCups = EyeCup_BlackA;
}

bool HMDProfile::ParseProperty(const char* prop, const char* sval)
{
    if (strcmp(prop, "LL") == 0)
    {
        LL = atoi(sval);
        return true;
    }
    else if (strcmp(prop, "LR") == 0)
    {
        LR = atoi(sval);
        return true;
    }
    else if (strcmp(prop, "RL") == 0)
    {
        RL = atoi(sval);
        return true;
    }
    else if (strcmp(prop, "RR") == 0)
    {
        RR = atoi(sval);
        return true;
    }

    if (strcmp(prop, "EyeCup") == 0)
    {
        if      ( 0 == strcmp ( sval, "A"        ) ) { EyeCups = EyeCup_BlackA; }
        else if ( 0 == strcmp ( sval, "B"        ) ) { EyeCups = EyeCup_BlackB; }
        else if ( 0 == strcmp ( sval, "C"        ) ) { EyeCups = EyeCup_BlackC; }
        else if ( 0 == strcmp ( sval, "Orange A" ) ) { EyeCups = EyeCup_OrangeA; }
        else if ( 0 == strcmp ( sval, "Red A"    ) ) { EyeCups = EyeCup_RedA; }
        else if ( 0 == strcmp ( sval, "Blue A"   ) ) { EyeCups = EyeCup_BlueA; }
        else
        {
            OVR_ASSERT ( !"Unknown lens type in profile" );
            EyeCups = EyeCup_BlackA;
        }
        return true;
    }

    return Profile::ParseProperty(prop, sval);
}

Profile* HMDProfile::Clone() const
{
    HMDProfile* profile = new HMDProfile(*this);
    return profile;
}

//-----------------------------------------------------------------------------
// ***** RiftDK1Profile

RiftDK1Profile::RiftDK1Profile(const char* name) : HMDProfile(Profile_RiftDK1, name)
{
}

bool RiftDK1Profile::ParseProperty(const char* prop, const char* sval)
{
    return HMDProfile::ParseProperty(prop, sval);
}

Profile* RiftDK1Profile::Clone() const
{
    RiftDK1Profile* profile = new RiftDK1Profile(*this);
    return profile;
}

//-----------------------------------------------------------------------------
// ***** RiftDKHDProfile

RiftDKHDProfile::RiftDKHDProfile(const char* name) : HMDProfile(Profile_RiftDKHD, name)
{
}

bool RiftDKHDProfile::ParseProperty(const char* prop, const char* sval)
{
    return HMDProfile::ParseProperty(prop, sval);
}

Profile* RiftDKHDProfile::Clone() const
{
    RiftDKHDProfile* profile = new RiftDKHDProfile(*this);
    return profile;
}

}  // OVR
