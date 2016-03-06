/************************************************************************************

Filename    :   SoundManager.cpp
Content     :   Sound asset manager via json definitions
Created     :   October 22, 2013
Authors     :   Warsam Osman

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/
#include "SoundManager.h"

#include "VJson.h"
#include "Android/LogUtils.h"

#include "VStandardPath.h"
#include "PackageFiles.h"


#include <list>
#include <fstream>
#include <sstream>

using namespace NervGear;

namespace NervGear {

static const char * DEV_SOUNDS_RELATIVE = "Oculus/sound_assets.json";
static const char * VRLIB_SOUNDS = "res/raw/sound_assets.json";
static const char * APP_SOUNDS = "assets/sound_assets.json";

void OvrSoundManager::LoadSoundAssets()
{
    Array<VString> searchPaths;
	searchPaths.append( "/storage/extSdCard/" );
	searchPaths.append( "/sdcard/" );

	// First look for sound definition using SearchPaths for dev
	VString foundPath;
	if ( GetFullPath( searchPaths, DEV_SOUNDS_RELATIVE, foundPath ) )
	{
		std::ifstream fp(foundPath.toCString(), std::ios::binary);
		Json dataFile;
		fp >> dataFile;
		if (dataFile.isInvalid())
		{
			FAIL( "OvrSoundManager::LoadSoundAssets failed to load JSON meta file: %s", foundPath.toCString( ) );
		}
		foundPath.stripTrailing( "sound_assets.json" );
		LoadSoundAssetsFromJsonObject( foundPath, dataFile );
	}
	else // if that fails, we are in release - load sounds from vrlib/res/raw and the assets folder
	{
		if ( ovr_PackageFileExists( VRLIB_SOUNDS ) )
		{
			LoadSoundAssetsFromPackage( "res/raw/", VRLIB_SOUNDS );
		}
		if ( ovr_PackageFileExists( APP_SOUNDS ) )
		{
			LoadSoundAssetsFromPackage( "", APP_SOUNDS );
		}
	}

	if ( SoundMap.empty() )
	{
#if defined( OVR_BUILD_DEBUG )
		FAIL( "SoundManger - failed to load any sound definition files!" );
#else
		WARN( "SoundManger - failed to load any sound definition files!" );
#endif
	}
}

bool OvrSoundManager::HasSound( const char * soundName )
{
	auto soundMapping = SoundMap.find( soundName );
	return ( soundMapping != SoundMap.end() );
}

bool OvrSoundManager::GetSound( const char * soundName, VString & outSound )
{
	auto soundMapping = SoundMap.find( std::string(soundName) );
	if ( soundMapping != SoundMap.end() )
	{
		outSound = VString(soundMapping->second.c_str());
		return true;
	}
	else
	{
		WARN( "OvrSoundManager::GetSound failed to find %s", soundName );
	}

	return false;
}

void OvrSoundManager::LoadSoundAssetsFromPackage( const VString & url, const char * jsonFile )
{
	int bufferLength = 0;
	void * 	buffer = NULL;
	ovr_ReadFileFromApplicationPackage( jsonFile, bufferLength, buffer );
	if ( !buffer )
	{
		FAIL( "OvrSoundManager::LoadSoundAssetsFromPackage failed to read %s", jsonFile );
	}

	std::stringstream s;
	s << reinterpret_cast< char * >( buffer );
	Json dataFile;
	s >> dataFile;
	if ( dataFile.isInvalid() )
	{
		FAIL( "OvrSoundManager::LoadSoundAssetsFromPackage failed json parse on %s", jsonFile );
	}
	free( buffer );

	LoadSoundAssetsFromJsonObject( url, dataFile );
}

void OvrSoundManager::LoadSoundAssetsFromJsonObject( const VString & url, const Json &dataFile )
{
	OVR_ASSERT( dataFile.isValid() );

	// Read in sounds - add to map
	Json sounds = dataFile.value("Sounds");
	OVR_ASSERT( sounds.isObject() );

    const JsonObject &pairs = sounds.toObject();
    for (const std::pair<std::string, Json> &pair : pairs) {
        const Json &sound = pair.second;
		OVR_ASSERT( sound.isValid() );

        std::string fullPath(url.toStdString());
		fullPath += sound.toString();

		// Do we already have this sound?
        std::map<std::string, std::string>::const_iterator soundMapping = SoundMap.find(pair.first);
		if ( soundMapping != SoundMap.end() )
		{
            LOG( "SoundManger - adding Duplicate sound %s with asset %s", pair.first.c_str( ), fullPath.c_str( ) );
            SoundMap[pair.first] = fullPath;
		}
		else // add new sound
		{
            LOG( "SoundManger read in: %s -> %s", pair.first.c_str(), fullPath.c_str( ) );
            SoundMap[pair.first] = fullPath;
		}
	}
}

}
