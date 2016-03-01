#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

//==============================================================
// SoundLimiter
class SoundLimiter
{
public:
	SoundLimiter() :
        m_lastPlayTime( 0 )
	{
	}

    void			playSound( class App * app, char const * soundName, double const limitSeconds );
	// Checks if menu specific sounds exists before playing the default vrlib sound passed in
    void			playMenuSound( class App * app,  char const * menuName, char const * soundName, double const limitSeconds );

private:
    double			m_lastPlayTime;
};

NV_NAMESPACE_END

