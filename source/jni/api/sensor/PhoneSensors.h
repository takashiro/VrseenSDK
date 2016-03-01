#pragma once

#include "vglobal.h"

#include "VMath.h"
#include "RefCount.h"

NV_NAMESPACE_BEGIN

class PhoneSensors : public RefCountBase<PhoneSensors>
{
public:
	virtual void GetLatestUncalibratedMagAndBiasValue(Vector3f* mag, Vector3f* bias) = 0;

	static PhoneSensors* Create();
	static PhoneSensors* GetInstance();

protected:
	PhoneSensors()
	{}

private:
	static PhoneSensors *Instance;
};

NV_NAMESPACE_END
