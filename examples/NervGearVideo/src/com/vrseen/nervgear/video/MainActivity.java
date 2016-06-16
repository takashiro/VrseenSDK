package com.vrseen.nervgear.video;

import android.os.Bundle;
import android.os.Environment;
import android.content.Intent;

import com.vrseen.nervgear.VrActivity;
import com.vrseen.nervgear.VrLib;

public class MainActivity extends VrActivity {

	PanoVideo video = null;

	static {
		System.loadLibrary("nervgearvideo");
	}

	public static native void nativeSetAppInterface(VrActivity act, String fromPackageNameString, String commandString, String uriString);

	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		Intent intent = getIntent();
		String commandString = VrLib.getCommandStringFromIntent(intent);
		String fromPackageNameString = VrLib.getPackageStringFromIntent(intent);
		String uriString = VrLib.getUriStringFromIntent(intent);
		if (uriString.isEmpty()) {
			uriString = Environment.getExternalStorageDirectory().getAbsolutePath() + "/Oculus/360Videos/[Samsung] 360 video demo.mp4";
		}

		nativeSetAppInterface(this, fromPackageNameString, commandString, uriString);

		video = new PanoVideo(this);
		video.start(uriString);
	}

	@Override
	protected void onDestroy() {
		video.releaseAudioFocus();
		super.onDestroy();
	}
	
	public void startMovieFromNative( final String pathName ) {
		runOnUiThread(new Runnable() {
			@Override
    		public void run() {
				video.start(pathName);
    		}
    	});
	}
}
