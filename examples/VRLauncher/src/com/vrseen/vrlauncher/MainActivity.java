package com.vrseen.vrlauncher;

import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import com.vrseen.VrActivity;

public class MainActivity extends VrActivity {

	VRLauncher launcher = null;

	/** Load jni .so on initialization */
	static {
		System.loadLibrary("vrlauncher");
	}

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		Log.d(TAG, "onCreate");
		super.onCreate(savedInstanceState);
		launcher = new VRLauncher(this);
		String background = Environment.getExternalStorageDirectory().getAbsolutePath() + "/VRSeen/SDK/360Photos/1.jpg";
		launcher.setBackground(background);
	}
}
