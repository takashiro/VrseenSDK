#include "VSoundManager.h"

#include "VJson.h"
#include "VLog.h"
#include "VResource.h"
#include "VStandardPath.h"
#include "VModule.h"
#include "VMap.h"
#include "App.h"

#include <list>
#include <fstream>
#include <sstream>

NV_NAMESPACE_BEGIN

static const char * DEV_SOUNDS_RELATIVE = "Oculus/sound_assets.json";
static const char * VRLIB_SOUNDS = "res/raw/sound_assets.json";
static const char * APP_SOUNDS = "assets/sound_assets.json";

namespace
{
    class VSoundModule : public VModule
    {
        void onStart() override
        {
            VSoundManager::instance()->loadSoundAssets();
        }
    };
}
NV_ADD_MODULE(VSoundModule)

struct VSoundManager::Private
{
    VMap<VString, VString> soundMap;

    void loadSoundAssetsFromJsonObject(const VString &url, const VJson &dataFile)
    {
        vAssert(dataFile.isObject());

        // Read in sounds - add to map
        VJson sounds = dataFile.value("Sounds");
        vAssert(sounds.isObject());

        const VJsonObject &pairs = sounds.toObject();
        for (const std::pair<VString, VJson> &pair : pairs) {
            const VJson &sound = pair.second;
            vAssert(sound.isString());

            VString fullPath = url + sound.toString();

            // Do we already have this sound?
            if (soundMap.contains(pair.first)) {
                vInfo("SoundManger - adding Duplicate sound" << pair.first << "with asset" << fullPath);
                soundMap[pair.first] = fullPath;
            // add new sound
            } else {
                vInfo("SoundManger read in:" << pair.first << "->" << fullPath);
                soundMap[pair.first] = fullPath;
            }
        }
    }

    void loadSoundAssetsFromPackage(const VString &url, const VString &jsonFilePath)
    {
        VResource jsonFile(jsonFilePath);
        if (!jsonFile.exists()) {
            vFatal("VSoundManager::LoadSoundAssetsFromPackage failed to read" << jsonFilePath);
        }

        std::stringstream buffer;
        buffer << jsonFile.data();
        VJson data;
        buffer >> data;
        if (data.isNull()) {
            vFatal("OvrSoundManager::LoadSoundAssetsFromPackage failed json parse on" << jsonFilePath);
        }

        loadSoundAssetsFromJsonObject(url, data);
    }
};

VSoundManager::VSoundManager()
    : d(new Private)
{
}

VSoundManager *VSoundManager::instance()
{
    static VSoundManager manager;
    return &manager;
}

VSoundManager::~VSoundManager()
{
    delete d;
}

void VSoundManager::loadSoundAssets()
{
	VArray<VString> searchPaths;
    searchPaths.append("/storage/extSdCard/");
    searchPaths.append("/sdcard/");

	// First look for sound definition using SearchPaths for dev
	VString foundPath;
    if (GetFullPath(searchPaths, DEV_SOUNDS_RELATIVE, foundPath)) {
        std::ifstream fp(foundPath.toStdString(), std::ios::binary);
		VJson dataFile;
		fp >> dataFile;
        if (dataFile.isNull()) {
            vFatal("OvrSoundManager::LoadSoundAssets failed to load JSON meta file:" << foundPath);
		}

        foundPath.stripTrailing("sound_assets.json");
        d->loadSoundAssetsFromJsonObject(foundPath, dataFile);

    // if that fails, we are in release - load sounds from vrlib/res/raw and the assets folder
    } else {
        if (VResource::Exist(VRLIB_SOUNDS)) {
            d->loadSoundAssetsFromPackage("res/raw/", VRLIB_SOUNDS);
		}
        if (VResource::Exist(APP_SOUNDS)) {
            d->loadSoundAssetsFromPackage("", APP_SOUNDS);
		}
	}

    if (d->soundMap.empty()) {
        vFatal("SoundManger - failed to load any sound definition files!");
	}
}

bool  VSoundManager::hasSound(const VString &soundName)
{
    auto soundMapping = d->soundMap.find(soundName);
    return soundMapping != d->soundMap.end();
}

bool  VSoundManager::getSound(const VString &soundName, VString & outSound)
{
    std::map<VString, VString>::const_iterator soundMapping = d->soundMap.find(soundName);
    if (soundMapping != d->soundMap.end()) {
        outSound = soundMapping->second;
		return true;
    } else {
        vWarn("OvrSoundManager::GetSound failed to find" << soundName);
	}
	return false;
}

NV_NAMESPACE_END
