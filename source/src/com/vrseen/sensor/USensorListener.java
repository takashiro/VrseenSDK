package com.vrseen.sensor;

public class USensorListener {
	private final RotationSensor mSensor;

	USensorListener(RotationSensor sensor) {
        mSensor = sensor;
    }

    void onAttached() {
        mSensor.onUSensorAttached();
    }

    void onDetached() {
        mSensor.onUSensorDetached();
    }

    void onSensorChanged(long timeStamp, float w, float x, float y, float z,
            float gyroX, float gyroY, float gyroZ) {
        mSensor.onUSensor(timeStamp, w, x, y, z, gyroX, gyroY, gyroZ);
    }

    void onSensorErrorDetected() {
        mSensor.onUSensorError();
    }
}
