#pragma once

#include "vglobal.h"

#include "Device.h"
#include "VArray.h"
#include "VArray.h"

NV_NAMESPACE_BEGIN

class GyroTempCalibration
{
public:
	GyroTempCalibration();

	void Initialize(const VString& deviceSerialNumber);

	void GetAllTemperatureReports(VArray<VArray<TemperatureReport> >* tempReports);
	void SetTemperatureReport(const TemperatureReport& tempReport);

private:
	enum { GyroCalibrationNumBins = 7 };
	enum { GyroCalibrationNumSamples = 5 };

	struct GyroCalibrationEntry
	{
		UInt32		Version;
		double	    ActualTemperature;
		UInt32      Time;
		Vector3d    Offset;
	};
	VString GetBaseOVRPath(bool create_dir);
	VString GetCalibrationPath(bool create_dir);
	void TokenizeString(VArray<VString>* tokens, const VString& str, char separator);
	void GyroCalibrationFromString(const VString& str);
	VString GyroCalibrationToString();
	void GetTemperatureReport(int binIndex, int sampleIndex, TemperatureReport* tempReport);

	void LoadFile();
	void SaveFile();

	VString DeviceSerialNumber;
	GyroCalibrationEntry GyroCalibration[GyroCalibrationNumBins][GyroCalibrationNumSamples];
};

NV_NAMESPACE_END
