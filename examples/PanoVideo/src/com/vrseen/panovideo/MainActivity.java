package com.vrseen.panovideo;

import android.os.Bundle;
import android.os.Environment;
import android.content.Intent;
import android.net.Uri;

import com.vrseen.VrActivity;

public class MainActivity extends VrActivity {

	PanoVideo video = null;
	static {
		System.loadLibrary("panovideo");
	}

	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		Intent intent = getIntent();
		Uri uri = intent.getData();
		String videoPath = uri != null ? uri.toString() : null;
		if (videoPath == null || videoPath.isEmpty()) {
			videoPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/Oculus/360Videos/[Samsung] 360 video demo.mp4";
		}

		video = new PanoVideo(this);
		video.start(videoPath);
	}

	@Override
	protected void onDestroy() {
		video.releaseAudioFocus();
		super.onDestroy();
	}
}
