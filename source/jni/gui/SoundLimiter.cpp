/************************************************************************************

Filename    :   SoundLimiter.cpp
Content     :   Utility class for limiting how often sounds play.
Created     :   June 23, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#include "SoundLimiter.h"
#include "core/VTimer.h"
#include "TypesafeNumber.h"
#include "VBasicmath.h"
#include "api/VKernel.h"		// ovrPoseStatef

#include "VFrame.h"
#include "App.h"
#include "VSoundManager.h"

namespace NervGear {

//==============================
// SoundLimiter::playSound
void SoundLimiter::playSound(App * app, char const * soundName, double const limitSeconds )
{
    double curTime = VTimer::Seconds();
	double t = curTime - m_lastPlayTime;
    vInfo("playSound(" << soundName << ", " << limitSeconds << ") - t ==" << t << ":" << (t >= limitSeconds ? "PLAYING" : "SKIPPING"));
	if ( t >= limitSeconds )
	{
		app->playSound( soundName );
		m_lastPlayTime = curTime;
	}
}

void SoundLimiter::playMenuSound(App * app, const VString &appendKey, char const * soundName, double const limitSeconds )
{
    VString overrideSound = appendKey;
    overrideSound.append('_');
    overrideSound.append(soundName);

    if (app->soundMgr().hasSound(overrideSound)) {
        playSound(app, overrideSound.toUtf8().data(), limitSeconds);
    } else {
        playSound(app, soundName, limitSeconds);
	}
}

} // namespace NervGear
