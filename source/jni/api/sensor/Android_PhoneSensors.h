#pragma once

#include "vglobal.h"
#include "PhoneSensors.h"
#include "VBasicmath.h"

#include <android/sensor.h>

NV_NAMESPACE_BEGIN

namespace Android {

class PhoneSensors : public NervGear::PhoneSensors
{
public:

	PhoneSensors();
	virtual ~PhoneSensors();

    virtual void GetLatestUncalibratedMagAndBiasValue(V3Vectf* mag, V3Vectf* bias);

private:
    V3Vectf			LatestMagUncalibrated;
    V3Vectf			LatestMagUncalibratedBias;
	bool				IsFirstExecution;

	ASensorEventQueue* 	pQueue;
	ASensorRef 			MagSensorUncalibrated;
};

}

NV_NAMESPACE_END
