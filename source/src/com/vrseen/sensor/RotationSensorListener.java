package com.vrseen.sensor;

public interface RotationSensorListener {
	 void onRotationSensor(long timeStamp, float w, float x, float y, float z,
	            float gyroX, float gyroY, float gyroZ);
}
