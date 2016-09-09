package com.vrseen.panovideo;

import java.io.IOException;

import android.app.Activity;
import android.content.Context;
import android.graphics.SurfaceTexture;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.util.Log;
import android.view.Surface;

public class PanoVideo {
	private static final String TAG = "PanoVideo";

	private SurfaceTexture movieTexture = null;
	private Surface movieSurface = null;
	private MediaPlayer mediaPlayer = null;
	private AudioManager audioManager = null;

	public PanoVideo(Activity activity) {
		audioManager = (AudioManager) activity.getSystemService(Context.AUDIO_SERVICE);
		construct(activity);
	}

	public void start(String pathName) {
		onStart(pathName);
		
		requestAudioFocus();

		movieTexture = createMovieTexture();
		movieTexture.setOnFrameAvailableListener(frameAvailableListener);
		movieSurface = new Surface(movieTexture);

		if (mediaPlayer != null) {
			mediaPlayer.release();
		}

		Log.v(TAG, "MediaPlayer.create");

		synchronized (this) {
			mediaPlayer = new MediaPlayer();
		}
		mediaPlayer.setOnVideoSizeChangedListener(videoSizeChangedListener);
		mediaPlayer.setOnCompletionListener(completionListener);

		mediaPlayer.setSurface(movieSurface);

		try {
			mediaPlayer.setDataSource(pathName);
		} catch (IOException t) {
			Log.e(TAG, "Failed to set data source.");
		}

		try {
			mediaPlayer.prepare();
		} catch (IOException t) {
			Log.e(TAG, "start():" + t.getMessage());
		}
		mediaPlayer.setLooping(false);

		try {
			mediaPlayer.start();
		} catch (IllegalStateException ise) {
			Log.d(TAG, "start(): " + ise.toString());
		}

		mediaPlayer.setVolume(1.0f, 1.0f);
	}

	public void stop() {
		if (mediaPlayer != null) {
			Log.d(TAG, "movie stopped");
			mediaPlayer.stop();
		}

		releaseAudioFocus();
	}

	public int getDuration()
	{
		return mediaPlayer.getDuration();
	}

	public  void setPos(int cur)
	{
		mediaPlayer.seekTo(cur);
	}

	public boolean isPlaying() {
		try {
			if (mediaPlayer != null) {
				return mediaPlayer.isPlaying();
			}
			return false;
		} catch (IllegalStateException ise) {
			Log.d(TAG, "isPlaying(): " + ise.toString());
		}
		return false;
	}

	public void pause() {
		try {
			if (mediaPlayer != null) {
				mediaPlayer.pause();
			}
		} catch (IllegalStateException ise) {
			Log.d(TAG, "pause(): Caught illegalStateException: " + ise.toString());
		}
	}

	public void resume() {
		try {
			if (mediaPlayer != null) {
				mediaPlayer.start();
				mediaPlayer.setVolume(1.0f, 1.0f);
			}
		} catch (IllegalStateException ise) {
			Log.d(TAG, "resume(): " + ise.toString());
		}
	}

	AudioManager.OnAudioFocusChangeListener audioFocusChangeLisener = new AudioManager.OnAudioFocusChangeListener() {
		public void onAudioFocusChange(int focusChange) {
			switch (focusChange) {
			case AudioManager.AUDIOFOCUS_GAIN:
				// resume() if coming back from transient loss, raise stream
				// volume if duck applied
				Log.d(TAG, "onAudioFocusChangedListener: AUDIOFOCUS_GAIN");
				break;
			case AudioManager.AUDIOFOCUS_LOSS:// focus lost permanently
				// stop() if isPlaying
				Log.d(TAG, "onAudioFocusChangedListener: AUDIOFOCUS_LOSS");
				break;
			case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:// focus lost
														// temporarily
				// pause() if isPlaying
				Log.d(TAG, "onAudioFocusChangedListener: AUDIOFOCUS_LOSS_TRANSIENT");
				break;
			case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:// focus lost
																	// temporarily
				// lower stream volume
				Log.d(TAG, "onAudioFocusChangedListener: AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK");
				break;
			default:
				break;
			}
		}
	};

	void requestAudioFocus() {
		int result = audioManager.requestAudioFocus(audioFocusChangeLisener, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
		if (result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
			Log.d(TAG, "startMovie(): GRANTED audio focus");
		}
	}

	void releaseAudioFocus() {
		audioManager.abandonAudioFocus(audioFocusChangeLisener);
	}

	MediaPlayer.OnErrorListener errorListener = new MediaPlayer.OnErrorListener() {
		public boolean onError(MediaPlayer mp, int what, int extra) {
			Log.e(TAG, "MediaPlayer.OnErrorListener - what : " + what + ", extra : " + extra);
			return false;
		}
	};


	native SurfaceTexture createMovieTexture();
	
	native void construct(Activity activity);
	
	native void onStart(String path);

	native void onVideoSizeChanged(int width, int height);

	MediaPlayer.OnVideoSizeChangedListener videoSizeChangedListener = new MediaPlayer.OnVideoSizeChangedListener() {
		@Override
		public void onVideoSizeChanged(MediaPlayer mp, int width, int height) {
			PanoVideo.this.onVideoSizeChanged(width, height);
		}
	};

	native void onFrameAvailable();
	native void getCurrentPos(int cur, int dur);
	native boolean mediaPause();

	SurfaceTexture.OnFrameAvailableListener frameAvailableListener = new SurfaceTexture.OnFrameAvailableListener() {
		@Override
		public void onFrameAvailable(SurfaceTexture surfaceTexture) {
			PanoVideo.this.onFrameAvailable();
			PanoVideo.this.getCurrentPos(mediaPlayer.getCurrentPosition(), mediaPlayer.getDuration());

		}
	};

	native void onCompletion();

	MediaPlayer.OnCompletionListener completionListener = new MediaPlayer.OnCompletionListener() {
		@Override
		public void onCompletion(MediaPlayer mp) {
			PanoVideo.this.onCompletion();
		}
	};
}
