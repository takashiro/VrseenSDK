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
	private final Sensor mInternalSensor;								//define internal sensor
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
		
		mInternalSensor = mInternalSensorManager.getDefaultSensor(
                Sensor.TYPE_ROTATION_VECTOR);
		
		if (mInternalSensor != null) {
			mInternalSensorManager.registerListener(mInternalSensorListener,
					mInternalSensor, SensorManager.SENSOR_DELAY_FASTEST); 
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
		if (mInternalSensor != null) {
			mInternalSensorManager.registerListener(mInternalSensorListener,
					mInternalSensor, SensorManager.SENSOR_DELAY_FASTEST); 
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
	
	
	void onInternalRotationSensor(long timeStamp, float w, float x, float y,
			float z, float gyroX, float gyroY, float gyroZ) {
		if (mCurrentSensor == SensorType.INTERNAL) {
			mListener.onRotationSensor(timeStamp, w, x, y, z, gyroX, gyroY,
					gyroZ);
		}
	}
	
	void onUSensor(long timeStamp, float w, float x, float y, float z,
			float gyroX, float gyroY, float gyroZ) {
		mCurrentSensor = SensorType.USBHOST;
		mListener.onRotationSensor(timeStamp, w, x, y, z, gyroX, gyroY, gyroZ);
		
		//System.out.println("usensor:"+gyroX);
		//System.out.println("on usensor");
	}
	
	void onUSensorError() {
		mCurrentSensor = SensorType.INTERNAL;
		System.out.println("change to internal");
	}

	public int getCurrentSensor() {
		return mCurrentSensor;
	}
}
