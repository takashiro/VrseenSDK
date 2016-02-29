/************************************************************************************

PublicHeader:   OVR.h
Filename    :   OVR_GyroTempCalibration.h
Content     :   Used to store gyro temperature calibration to local storage.
Created     :   June 26, 2014
Notes       :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#ifndef OVR_GyroTempCalibration_h
#define OVR_GyroTempCalibration_h

#include "Device.h"
#include "Array.h"

namespace NervGear {

class GyroTempCalibration
{
public:
	GyroTempCalibration();

	void Initialize(const VString& deviceSerialNumber);

	void GetAllTemperatureReports(Array<Array<TemperatureReport> >* tempReports);
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
	void TokenizeString(Array<VString>* tokens, const VString& str, char separator);
	void GyroCalibrationFromString(const VString& str);
	VString GyroCalibrationToString();
	void GetTemperatureReport(int binIndex, int sampleIndex, TemperatureReport* tempReport);

	void LoadFile();
	void SaveFile();

	VString DeviceSerialNumber;
	GyroCalibrationEntry GyroCalibration[GyroCalibrationNumBins][GyroCalibrationNumSamples];
};

} // namespace NervGear

#endif // OVR_GyroTempCalibration_h
