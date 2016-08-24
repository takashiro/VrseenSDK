package com.vrseen.arcamera;

import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.content.Intent;
import android.net.Uri;

import com.vrseen.VrActivity;

public class MainActivity extends VrActivity {

	ArCamera video = null;
	static {
		System.loadLibrary("arcamera");
	}

	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		Intent intent = getIntent();
		Uri uri = intent.getData();
		String videoPath = uri != null ? uri.toString() : null;
		if (videoPath == null || videoPath.isEmpty()) {
			videoPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/VRSeen/SDK/360Videos/[Samsung] 360 video demo.mp4";
		}

		video = new ArCamera(this);
		video.start(videoPath);
	}

	@Override
	protected void onDestroy() {
		video.releaseAudioFocus();
		super.onDestroy();
	}

	@Override
	protected void onPause() {
        super.onPause();
        if (video != null && video.isPlaying()) {
			video.pause();
		}
	}

	@Override
	protected void onResume()  {
        super.onResume();
        if (video != null && !video.isPlaying()) {
			video.resume();
		}
	}
}
