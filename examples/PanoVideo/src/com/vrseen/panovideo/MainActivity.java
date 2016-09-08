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
			videoPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/VRSeen/SDK/360Videos/[Samsung] 360 video demo.mp4";
		}

		video = new PanoVideo(this);
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

	public void Pause()
	{
		video.pause();
	}

	public boolean IsPlaying()
	{
		return video.isPlaying();
	}

	public  void Resume()
	{
		video.resume();
	}

	public void setPos(float ratio)
	{
		video.setPos((int)(video.getDuration() * ratio));
	}
}
