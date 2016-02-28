/************************************************************************************

Filename    :   OVR_Android_PhoneSensors.h
Content     :   Android sensor interface.
Created     :
Authors     :	Lee Cooper

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#ifndef OVR_Android_PhoneSensors_h
#define OVR_Android_PhoneSensors_h

#include "PhoneSensors.h"
#include "VMath.h"
#include "Threads.h"

#include <android/sensor.h>

namespace NervGear { namespace Android {

class PhoneSensors : public NervGear::PhoneSensors
{
public:

	PhoneSensors();
	virtual ~PhoneSensors();

	virtual void GetLatestUncalibratedMagAndBiasValue(Vector3f* mag, Vector3f* bias);

private:
	Vector3f			LatestMagUncalibrated;
	Vector3f			LatestMagUncalibratedBias;
	bool				IsFirstExecution;

	ASensorEventQueue* 	pQueue;
	ASensorRef 			MagSensorUncalibrated;
};

}} // namespace NervGear::Android

#endif // OVR_Android_PhoneSensors_h
