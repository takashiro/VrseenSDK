package com.vrseen.sensor;

 

import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorManager;

public class RotationSensor {
	private static abstract class SensorType {							//define sensor type
		public static final int INTERNAL = 1;
 		public static final int USBHOST = 2;
	}

	private static final int DEFAULT_SENSOR = SensorType.INTERNAL;		//default use internal sensor
	private int mCurrentSensor = DEFAULT_SENSOR;
	
	private final RotationSensorListener mListener;						//define rotation sensor listener
	
	private final SensorManager mInternalSensorManager;					//define internal sensor manager
	private final Sensor mInternalRotationSensor;								//define internal sensor
	private final Sensor mInternalGyroSensor;
	private final InternalSensorListener mInternalSensorListener;		//define internal sensor listener

	private final USensor mUSensor;										//define usb sensor
	private final USensorListener mUSensorListener;						//define usb sensor listener
	
	
	RotationSensor(Context context, RotationSensorListener listener) {	
		mListener = listener;
		mInternalSensorManager = (SensorManager) context
				.getSystemService(Context.SENSOR_SERVICE);
			

		mInternalSensorListener = new InternalSensorListener(this);

		//mInternalSensor = mInternalSensorManager
		//		.getDefaultSensor(Sensor.TYPE_GAME_ROTATION_VECTOR);
		
		mInternalRotationSensor = mInternalSensorManager.getDefaultSensor(
                Sensor.TYPE_ROTATION_VECTOR);
		
		if (mInternalRotationSensor != null) {
			mInternalSensorManager.registerListener(mInternalSensorListener,
					mInternalRotationSensor, SensorManager.SENSOR_DELAY_FASTEST);
		}

		mInternalGyroSensor = mInternalSensorManager.getDefaultSensor(
				Sensor.TYPE_ROTATION_VECTOR);

		if (mInternalGyroSensor != null) {
			mInternalSensorManager.registerListener(mInternalSensorListener,
					mInternalGyroSensor, SensorManager.SENSOR_DELAY_FASTEST);
		}
		
		mUSensor = new USensor(context);
		if (mUSensor.Open()) {
			mCurrentSensor = SensorType.USBHOST;
	 
		}
		else {
			mCurrentSensor = SensorType.INTERNAL;
			
		}
		mUSensorListener = new USensorListener(this);
		mUSensor.registerListener(mUSensorListener);

	}
	
	void onResume() {
		if (mInternalRotationSensor != null) {
			mInternalSensorManager.registerListener(mInternalSensorListener,
					mInternalRotationSensor, SensorManager.SENSOR_DELAY_FASTEST);
		}
		if (mInternalGyroSensor != null) {
			mInternalSensorManager.registerListener(mInternalSensorListener,
					mInternalGyroSensor, SensorManager.SENSOR_DELAY_FASTEST);
		}
		mUSensor.resume();
	}
	
	void onPause() {
		mInternalSensorManager.unregisterListener(mInternalSensorListener);
	 
		mUSensor.pause();
	}
	
	void onDestroy() {
		mUSensor.close();
	}
	
	void onInternalRotationSensor(long timeStamp, float w, float x, float y, float z, float gyroX, float gyroY, float gyroZ) {
		if (mCurrentSensor == SensorType.INTERNAL) {
			update(timeStamp, w, x, y, z, gyroX, gyroY, gyroZ);
			mListener.onRotationSensor(timeStamp, w, x, y, z, gyroX, gyroY, gyroZ);
		}
	}
	
	void onUSensor(long timeStamp, float w, float x, float y, float z,
			float gyroX, float gyroY, float gyroZ) {
		mCurrentSensor = SensorType.USBHOST;
		mListener.onRotationSensor(timeStamp, w, x, y, z, gyroX, gyroY, gyroZ);
	}
	
	void onUSensorError() {
		mCurrentSensor = SensorType.INTERNAL;
		System.out.println("change to internal");
	}

	public int getCurrentSensor() {
		return mCurrentSensor;
	}

	native void update(long timeStamp, float w, float x, float y, float z, float gyroX, float gyroY, float gyroZ);
}
