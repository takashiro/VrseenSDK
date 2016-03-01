#pragma once

#include "vglobal.h"
#include "VString.h"

#include <map>
#include <string>

NV_NAMESPACE_BEGIN

class Json;

class OvrSoundManager
{
public:
	OvrSoundManager() {}

	void	LoadSoundAssets();
	bool	HasSound( const char * soundName );
	bool	GetSound( const char * soundName, VString & outSound );

private:
    void	LoadSoundAssetsFromJsonObject( const VString & url, const Json &dataFile );
	void	LoadSoundAssetsFromPackage( const VString & url, const char * jsonFile );

	std::map<std::string, std::string> SoundMap;	// Maps hashed sound name to sound asset url
};

NV_NAMESPACE_END

