package com.vrseen.sensor;

import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.os.Build;

public class InternalSensorListener  implements SensorEventListener {
	private RotationSensor mSensor = null;

	public InternalSensorListener(RotationSensor sensor) {
		mSensor = sensor;
	}
	
	@Override
	public void onSensorChanged(SensorEvent event) {
		// TODO Auto-generated method stub
		if (event.sensor.getType() == Sensor.TYPE_ROTATION_VECTOR) {
			float w;
			float x = event.values[0];
			float y = event.values[1];
			float z = event.values[2];

			if (Build.VERSION.SDK_INT < 18) {
				w = getQuaternionW(event.values[0], event.values[1],
						event.values[2]);
			} else {
				w = event.values[3];
			}

			mSensor.onInternalRotationSensor(VrseenTime.getCurrentTime(), w, x, y, z, 0, 0, 0);
		}
		
	}

	@Override
	public void onAccuracyChanged(Sensor sensor, int accuracy) {
		// TODO Auto-generated method stub
		
	}
	
	private float getQuaternionW(float x, float y, float z) {
		return (float) Math.cos(Math.asin(Math.sqrt(x * x + y * y + z * z)));
	}
 

}
