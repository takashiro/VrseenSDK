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
	
	@Override
	public void onSensorChanged(SensorEvent event) {
		// TODO Auto-generated method stub
		if (event.sensor.getType() == Sensor.TYPE_ROTATION_VECTOR) {
			float[] quaternion = new float[4];
			SensorManager.getQuaternionFromVector(quaternion, event.values);
			float w = quaternion[0];
			float x = quaternion[1];
			float y = quaternion[2];
			float z = quaternion[3];

			mSensor.onInternalRotationSensor(VrseenTime.getCurrentTime(), w, x, y, z, 0, 0, 0);
		}
	}

	@Override
	public void onAccuracyChanged(Sensor sensor, int accuracy) {
		// TODO Auto-generated method stub
		
	}
}
