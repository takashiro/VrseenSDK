package com.vrseen.unittest;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;

public class MainActivity extends Activity {

	static {
        System.loadLibrary("unittest");
    }
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		int result = exec();
		if (result != 0) {
			Log.e("UnitTest", "Unit Test terminated unexpectedly.");
		} else {
			Log.i("UnitTest", "Unit Test succeeded.");
		}
		super.onCreate(savedInstanceState);
	}

	private native int exec();
}
