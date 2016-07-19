package com.vrseen.sensor;

import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;

public class InternalSensorListener implements SensorEventListener {
	private RotationSensor mSensor = null;

	public InternalSensorListener(RotationSensor sensor) {
		mSensor = sensor;
	}

	private float w = 1.0f;
	private float x = 0.0f;
	private float y = 0.0f;
	private float z = 0.0f;
	private float gyroX = 0.0f;
	private float gyroY = 0.0f;
	private float gyroZ = 0.0f;

	@Override
	public void onSensorChanged(SensorEvent event) {
		// TODO Auto-generated method stub
		if (event.sensor.getType() == Sensor.TYPE_ROTATION_VECTOR) {
			float[] quaternion = new float[4];
			SensorManager.getQuaternionFromVector(quaternion, event.values);
			w = quaternion[0];
			x = quaternion[1];
			y = quaternion[2];
			z = quaternion[3];

			mSensor.onInternalRotationSensor(VrseenTime.getCurrentTime(), w, x, y, z, gyroX, gyroY, gyroZ);
		} else if (event.sensor.getType() == Sensor.TYPE_GYROSCOPE) {
			gyroX = event.values[0];
			gyroY = event.values[1];
			gyroZ = event.values[2];

			mSensor.onInternalRotationSensor(VrseenTime.getCurrentTime(), w, x, y, z, gyroX, gyroY, gyroZ);
		}
	}

	@Override
	public void onAccuracyChanged(Sensor sensor, int accuracy) {
		// TODO Auto-generated method stub
		
	}
}
