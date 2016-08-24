package com.vrseen.arcamera;

import java.io.IOException;
import java.util.List;

import android.app.Activity;
import android.content.Context;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.util.Log;
import android.view.Surface;

public class ArCamera implements android.graphics.SurfaceTexture.OnFrameAvailableListener {
	private static final String TAG = "ArCamera";

	private Surface movieSurface = null;
//	private MediaPlayer mediaPlayer = null;
//	private AudioManager audioManager = null;

	//passthrough
	SurfaceTexture movieTexture;
	Camera	camera;

	boolean gotFirstFrame;

	boolean previewStarted;

	long	appPtr = 0;	// this must be cached for the onFrameAvailable callback :(
	
	boolean	hackVerticalFov = false;	// 60 fps preview forces 16:9 aspect, but doesn't report it correctly
	long	startPreviewTime;
	
/*	public native SurfaceTexture nativeGetCameraSurfaceTexture(long appPtr);
	public native void nativeSetCameraFov(long appPtr, float fovHorizontal, float fovVertical);
	*/
	public void onFrameAvailable(SurfaceTexture surfaceTexture) {
		if (gotFirstFrame) {
			return;
		}
		if ( camera == null ) {
			return;
		}
		gotFirstFrame = true;

		// Now that there is an image ready, tell native code to display it
		Camera.Parameters parms = camera.getParameters();
		float fovH = parms.getHorizontalViewAngle();
		float fovV = parms.getVerticalViewAngle();
		
		// hack for 60/120 fps camera, recalculate fovV from 16:9 ratio fovH
		if ( hackVerticalFov ) {
			fovV = 2.0f * (float)( Math.atan( Math.tan( fovH/180.0 * Math.PI * 0.5f ) / 16.0f * 9.0f ) / Math.PI * 180.0f ); 
		}

		Log.v(TAG, "camera view angles:" + fovH + " " + fovV + " from " + parms.getVerticalViewAngle() );

		long now = System.nanoTime();
		Log.v(TAG,  "seconds to first frame: " + (now-startPreviewTime) * 10e-10f);
		onFrameAvailable();
	}

	public void startExposureLock( long appPtr_, boolean locked ) {
		//Log.d( TAG, "startExposureLock appPtr_ is " + Long.toHexString( appPtr ) + " : " + Long.toHexString( appPtr_ ) );
/*		if ( BuildConfig.DEBUG && ( appPtr != appPtr_ ) && ( appPtr != 0 ) )
		{ 
			//Log.d( TAG, "startExposureLock: appPtr changed!" );
			assert false; // if this asserts then the wrong instance is being called
		} */

		Log.v(TAG, "startExposureLock:" + locked);
		
		if ( camera == null ) {
			return;
		}
		
		// Magic set of parameters from jingoolee@samsung.com
		Camera.Parameters parms = camera.getParameters();

		parms.setAutoExposureLock( locked );
		parms.setAutoWhiteBalanceLock( locked );
		
		camera.setParameters( parms );
	}

	//passthrough
	
	public ArCamera(Activity activity) {
//		audioManager = (AudioManager) activity.getSystemService(Context.AUDIO_SERVICE);
		construct(activity);
		Log.v(TAG, "New ArCamera");
	}

	public void start(String pathName) {
		onStart(pathName);
		movieTexture = createMovieTexture();
		if (movieTexture == null) {
			Log.e(TAG, "nativeGetCameraSurfaceTexture returned NULL");
			return; // not set up yet
		}
		
		movieTexture.setOnFrameAvailableListener(this);

		startPreviewTime = System.nanoTime();
		if (camera != null) 
		{
			gotFirstFrame = false;
			camera.startPreview();		
			previewStarted = true;			
			return;
		}

		camera = Camera.open();
		Camera.Parameters parms = camera.getParameters();
		if ("true".equalsIgnoreCase(parms.get("vrmode-supported"))) 
		{
			Log.v(TAG, "VR Mode supported!");
			
			//set vr mode
			parms.set("vrmode", 1); 
	 
			// true if the apps intend to record videos using MediaRecorder
			parms.setRecordingHint(true); 
			this.hackVerticalFov = true;	// will always be 16:9			
			
			// set preview size 
			parms.setPreviewSize(960, 540); 
			parms.set("fast-fps-mode", 2); // 2 for 120fps 
			parms.setPreviewFpsRange(120000, 120000); 
			
			parms.set("focus-mode", "continuous-video");
			
		} else { // not support vr mode }
			Camera.Size preferredSize = parms.getPreferredPreviewSizeForVideo();
			Log.v(TAG, "preferredSize: " + preferredSize.width + " x " + preferredSize.height );
			
			List<Integer> formats = parms.getSupportedPreviewFormats();
			for (int i = 0; i < formats.size(); i++) {
				Log.v(TAG, "preview format: " + formats.get(i) );
			}
		
			// YV12 format, documented YUV format exposed to software
//			parms.setPreviewFormat( 842094169 );

			// set the preview size to something small
			List<Camera.Size> previewSizes = parms.getSupportedPreviewSizes();
			for (int i = 0; i < previewSizes.size(); i++) {
				Log.v(TAG, "preview size: " + previewSizes.get(i).width + ","
						+ previewSizes.get(i).height);
			}

			Log.v(TAG, "isAutoExposureLockSupported: " + parms.isAutoExposureLockSupported() );
			Log.v(TAG, "isAutoWhiteBalanceLockSupported: " + parms.isAutoWhiteBalanceLockSupported() );
			Log.v(TAG, "minExposureCompensation: " + parms.getMinExposureCompensation() );
			Log.v(TAG, "maxExposureCompensation: " + parms.getMaxExposureCompensation() );
			
			float fovH = parms.getHorizontalViewAngle();
			float fovV = parms.getVerticalViewAngle();
			Log.v(TAG, "camera view angles:" + fovH + " " + fovV);
			
			 parms.setPreviewSize(800, 480);
			List<int[]> fpsRanges = parms.getSupportedPreviewFpsRange();
			for (int i = 0; i < fpsRanges.size(); i++) 
			{
				Log.v(TAG, "fps range: " + fpsRanges.get(i)[0] + ","
								+ fpsRanges.get(i)[1]);
			}
	
			int[] maxFps = fpsRanges.get(fpsRanges.size() - 1);
			this.hackVerticalFov = false;
			parms.setPreviewFpsRange(maxFps[0], maxFps[1]);						
		}
		
		Log.v(TAG, "camera.getVideoStabilization: " + parms.getVideoStabilization() );
		Log.v(TAG, "camera.ois-supported: " + parms.get("ois-supported") );
		Log.v(TAG, "camera.ois: " + parms.get("ois") );
		
		// Definitely don't want video stabilization, digital or optical!
		parms.setVideoStabilization( false );
		if ("true".equalsIgnoreCase(parms.get("ois-supported"))) {
			parms.set( "ois",  "center" );
		}
		Log.v(TAG, "camera.ois: " + parms.get("ois") );

		
		camera.setParameters(parms);
		
		Log.v(TAG, "camera.setPreviewTexture");
		try {
			camera.setPreviewTexture(movieTexture);
		} catch (IOException e) {
			Log.v(TAG, "startCameraPreviewToTextureId: setPreviewTexture exception");
		}
		
		Log.v(TAG, "camera.startPreview");
		gotFirstFrame = false;
		camera.startPreview();
		previewStarted = true;
	}

	public void stop() {

		previewStarted = false;
		if ( movieTexture != null )
		{
			movieTexture.setOnFrameAvailableListener( null );
		}
		if ( camera != null )
		{
			Log.v(TAG, "camera.stopPreview");
			camera.stopPreview();
			camera.release();
			camera = null;
		}
	}

	public boolean isPlaying() {
/*		try {
			if (mediaPlayer != null) {
				return mediaPlayer.isPlaying();
			}
			return false;
		} catch (IllegalStateException ise) {
			Log.d(TAG, "isPlaying(): " + ise.toString());
		}*/
		return false;
	}

	public void pause() {
/*		try {
			if (mediaPlayer != null) {
				mediaPlayer.pause();
			}
		} catch (IllegalStateException ise) {
			Log.d(TAG, "pause(): Caught illegalStateException: " + ise.toString());
		}*/
	}

	public void resume() {
/*		try {
			if (mediaPlayer != null) {
				mediaPlayer.start();
				mediaPlayer.setVolume(1.0f, 1.0f);
			}
		} catch (IllegalStateException ise) {
			Log.d(TAG, "resume(): " + ise.toString());
		}*/
	}

/*	AudioManager.OnAudioFocusChangeListener audioFocusChangeLisener = new AudioManager.OnAudioFocusChangeListener() {
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
	};*/

	void requestAudioFocus() {
/*		int result = audioManager.requestAudioFocus(audioFocusChangeLisener, AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN);
		if (result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
			Log.d(TAG, "startMovie(): GRANTED audio focus");
		}*/
	}

	void releaseAudioFocus() {
/*		audioManager.abandonAudioFocus(audioFocusChangeLisener);*/
	}

/*	MediaPlayer.OnErrorListener errorListener = new MediaPlayer.OnErrorListener() {
		public boolean onError(MediaPlayer mp, int what, int extra) {
			Log.e(TAG, "MediaPlayer.OnErrorListener - what : " + what + ", extra : " + extra);
			return false;
		}
	};*/

	native SurfaceTexture createMovieTexture();
	
	native void construct(Activity activity);
	
	native void onStart(String path);

	native void onVideoSizeChanged(int width, int height);

/*	MediaPlayer.OnVideoSizeChangedListener videoSizeChangedListener = new MediaPlayer.OnVideoSizeChangedListener() {
		@Override
		public void onVideoSizeChanged(MediaPlayer mp, int width, int height) {
			ArCamera.this.onVideoSizeChanged(width, height);
		}
	};*/

	native void onFrameAvailable();

/*	SurfaceTexture.OnFrameAvailableListener frameAvailableListener = new SurfaceTexture.OnFrameAvailableListener() {
		@Override
		public void onFrameAvailable(SurfaceTexture surfaceTexture) {
			ArCamera.this.onFrameAvailable();
		}
	};*/

	native void onCompletion();

/*	MediaPlayer.OnCompletionListener completionListener = new MediaPlayer.OnCompletionListener() {
		@Override
		public void onCompletion(MediaPlayer mp) {
			ArCamera.this.onCompletion();
		}
	};*/
}
