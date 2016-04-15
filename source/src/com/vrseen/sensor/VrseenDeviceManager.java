package com.vrseen.sensor;

import android.app.Activity;
import android.content.Context;

public class VrseenDeviceManager implements RotationSensorListener{
	Context mContext = null;
	Activity mActivity = null;
	protected RotationSensor mRotationSensor;
	
	public class SensorData {
		public long timeStamp;
		public float w;
		public float x; 
		public float y;
		public float z;
	}
	
	SensorData mSensorData;

	public SensorData getmSensorData() {
		return mSensorData;
	}

	public VrseenDeviceManager(Activity activity) {
		mActivity = activity;
		mContext = activity.getApplicationContext();
		
		mRotationSensor = new RotationSensor(mContext, this);
		mSensorData = new SensorData();
	}

	@Override
	public void onRotationSensor(long timeStamp, float w, float x, float y,
			float z, float gyroX, float gyroY, float gyroZ) {
		// TODO Auto-generated method stub
		mSensorData.timeStamp = timeStamp;
		mSensorData.w = w;
		mSensorData.x = x;
		mSensorData.y = y;
		mSensorData.z = z;
		
		
		//System.out.println(mRotationSensor.getCurrentSensor()+"time=" + mSensorData.timeStamp + "x=" + mSensorData.x );
	}
	
	
	public int getSensorType() {
		return mRotationSensor.getCurrentSensor();
	}
	
	public void onPause() {
		mRotationSensor.onPause();
	}

	public void onResume() {
		mRotationSensor.onResume();
	}

	/**
	 * The final call you receive before your activity is destroyed.
	 */
	public void onDestroy() {

		mRotationSensor.onDestroy();
	}
}
