#pragma once

#include "Lockless.h"	// for LocklessUpdater

// Application code should not interact with this, all timing information
// should be taken from VrShell.

// The Java_me_takashiro_nervgear_VrLib_nativeVsync() function is called by a java choreographer doFrame() callback
// to synchronize timing with vsync, and allow us to estimate the scan period.
// It doesn't matter if some frames are dropped or delayed, the timing
// information should be good for a prolonged period after getting two good
// samples. We may want to save some CPU by cutting the choreographer callbacks to
// once a second after timing is established.
//	void Java_me_takashiro_nervgear_VrLib_nativeVsync( JNIEnv *jni, jclass clazz, jlong nanoSeconds );

#pragma once

NV_NAMESPACE_BEGIN

class VsyncState
{
public:
	long long vsyncCount;
	double	vsyncPeriodNano;
	double	vsyncBaseNano;
};

// This can be read without any locks, so a high priority rendering thread doesn't
// have to worry about being blocked by a sensor thread that got preempted.
extern LocklessUpdater<VsyncState> UpdatedVsyncState;

// Estimates the current vsync count and fraction based on the most
// current timing provided from java.  This does not interact with
// the JVM at all.
double			GetFractionalVsync();

// Returns a time in seconds for the given fractional frame.
double			FramePointTimeInSeconds( const double framePoint );

// Returns the fractional frame for the given time in seconds.
double			FramePointTimeInSecondsWithBlanking( const double framePoint );

// Does a nanosleep() that will wake shortly after the given targetSeconds.
// Returns the seconds that were requested to sleep, which will be <=
// the time actually slept, which may be negative if already past the
// framePoint.
float 			SleepUntilTimePoint( const double targetSeconds, const bool busyWait );

NV_NAMESPACE_END


