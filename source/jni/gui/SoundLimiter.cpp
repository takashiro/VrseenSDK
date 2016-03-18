/************************************************************************************

Filename    :   SoundLimiter.cpp
Content     :   Utility class for limiting how often sounds play.
Created     :   June 23, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#include "SoundLimiter.h"

#include "Types.h"
#include "TypesafeNumber.h"
#include "VMath.h"
#include "Android/LogUtils.h"
#include "api/VrApi.h"		// ovrPoseStatef

#include "../Input.h"
#include "../VrCommon.h"
#include "../App.h"
#include "../SoundManager.h"

namespace NervGear {

//==============================
// SoundLimiter::playSound
void SoundLimiter::playSound( App * app, char const * soundName, double const limitSeconds )
{
	double curTime = ovr_GetTimeInSeconds();
	double t = curTime - m_lastPlayTime;
    //DROIDLOG( "VrMenu", "playSound( '%s', %.2f ) - t == %.2f : %s", soundName, limitSeconds, t, t >= limitSeconds ? "PLAYING" : "SKIPPING" );
	if ( t >= limitSeconds )
	{
		app->playSound( soundName );
		m_lastPlayTime = curTime;
	}
}

void SoundLimiter::playMenuSound( class App * app, char const * appendKey, char const * soundName, double const limitSeconds )
{
	char overrideSound[ 1024 ];
	OVR_sprintf( overrideSound, 1024, "%s_%s", appendKey, soundName );

	if ( app->soundMgr().HasSound( overrideSound ) )
	{
		playSound( app, overrideSound, limitSeconds );
	}
	else
	{
		playSound( app, soundName, limitSeconds );
	}
}

} // namespace NervGear
