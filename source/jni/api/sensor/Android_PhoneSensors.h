#pragma once

#include "vglobal.h"
#include "PhoneSensors.h"
#include "VMath.h"
#include "VBasicmath.h"

#include <android/sensor.h>

NV_NAMESPACE_BEGIN

namespace Android {

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

}

NV_NAMESPACE_END
