#pragma once

#include "vglobal.h"
#include "VString.h"

NV_NAMESPACE_BEGIN

class App;

class SoundLimiter
{
public:
	SoundLimiter() :
        m_lastPlayTime( 0 )
	{
	}

    void playSound(App * app, char const * soundName, double const limitSeconds );
	// Checks if menu specific sounds exists before playing the default vrlib sound passed in
    void playMenuSound(App * app,  const VString &menuName, char const * soundName, double const limitSeconds );

private:
    double			m_lastPlayTime;
};

NV_NAMESPACE_END

