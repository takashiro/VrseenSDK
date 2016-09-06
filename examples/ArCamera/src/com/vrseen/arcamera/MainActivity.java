package com.vrseen.arcamera;

import android.os.Bundle;

import com.vrseen.VrActivity;

public class MainActivity extends VrActivity {

	ArCamera video = null;
	static {
		System.loadLibrary("arcamera");
	}

	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		video = new ArCamera(this);
		video.start();
	}

	@Override
	protected void onDestroy() {
		video.stop();
		super.onDestroy();
	}

	@Override
	protected void onPause() {
        super.onPause();
		video.pause();
	}

	@Override
	protected void onResume()  {
        super.onResume();
        if (video != null) {
			video.resume();
		}
	}
}
