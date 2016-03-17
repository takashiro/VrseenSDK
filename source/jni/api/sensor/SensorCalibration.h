#pragma once

#include "Device.h"
#include "SensorFilter.h"
#include "GyroTempCalibration.h"

#define USE_LOCAL_TEMPERATURE_CALIBRATION_STORAGE 1

NV_NAMESPACE_BEGIN

class OffsetInterpolator
{
public:
    void Initialize(VArray<VArray<TemperatureReport> > const& temperatureReports, int coord);
    double GetOffset(double targetTemperature, double autoTemperature, double autoValue);

    VArray<double> Temperatures;
    VArray<double> Values;
};

class SensorCalibration : public NewOverrideBase
{
public:
    SensorCalibration(SensorDevice* pSensor);

    // Load data from the HW and perform the necessary preprocessing
    void Initialize(const VString& deviceSerialNumber);
    // Apply the calibration
    void Apply(MessageBodyFrame& msg);

protected:
    void StoreAutoOffset();
    void AutocalibrateGyro(MessageBodyFrame const& msg);

	void DebugPrintLocalTemperatureTable();
	void DebugClearHeadsetTemperatureReports();

    SensorDevice* pSensor;

    // Factory calibration data
    Matrix4f AccelMatrix, GyroMatrix;
    Vector3f AccelOffset;

    // Temperature based data
    VArray<VArray<TemperatureReport> > TemperatureReports;
    OffsetInterpolator Interpolators[3];

    // Autocalibration data
    SensorFilterf GyroFilter;
    Vector3f GyroAutoOffset;
    float GyroAutoTemperature;

#ifdef USE_LOCAL_TEMPERATURE_CALIBRATION_STORAGE
	GyroTempCalibration GyroCalibration;
#endif
};

NV_NAMESPACE_END

