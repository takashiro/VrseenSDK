package com.vrseen.panocinema;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import com.vrseen.VrActivity;

public class MainActivity extends VrActivity {

	PanoCinema cinema = null;
	static
	{
		System.loadLibrary( "panocinema" );
	}

	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		Intent intent = getIntent();
		Uri uri = intent.getData();
		String videoPath = uri != null ? uri.toString() : null;
		if (videoPath == null || videoPath.isEmpty()) {
			videoPath = Environment.getExternalStorageDirectory().getAbsolutePath()+"/VRSeen/SDK/360Cinema/cinema_test.mp4";
		}

		cinema = new PanoCinema(this);
		cinema.start(videoPath);
	}

	@Override
	protected void onDestroy() {
		cinema.releaseAudioFocus();
		super.onDestroy();
	}
}
