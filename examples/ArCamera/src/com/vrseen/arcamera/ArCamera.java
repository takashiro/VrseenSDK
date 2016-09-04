package com.vrseen.arcamera;

import java.io.IOException;
import java.util.List;

import android.app.Activity;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.util.Log;

public class ArCamera implements android.graphics.SurfaceTexture.OnFrameAvailableListener {
	private static final String TAG = "ArCamera";

	SurfaceTexture cameraTexture;
	Camera	camera;

	boolean previewStarted;

	boolean	hackVerticalFov = false;	// 60 fps preview forces 16:9 aspect, but doesn't report it correctly
	long	startPreviewTime;

	public void onFrameAvailable(SurfaceTexture surfaceTexture) {
		if ( camera == null ) {
			return;
		}
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
	}
	
	public ArCamera(Activity activity) {
		construct(activity);
		Log.v(TAG, "New ArCamera");
	}

	public void start() {
		cameraTexture = createCameraTexture();
		if (cameraTexture == null) {
			Log.e(TAG, "createCameraTexture returned NULL");
			return; // not set up yet
		}
		
		cameraTexture.setOnFrameAvailableListener(this);

		startPreviewTime = System.nanoTime();
		if (camera != null) 
		{
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
			
			//mark set preview size
			parms.setPreviewSize(1024, 1024);
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

			// set the preview size to something small
			List<Camera.Size> previewSizes = parms.getSupportedPreviewSizes();
			for (int i = 0; i < previewSizes.size(); i++) {
				Log.v(TAG, "preview size: " + previewSizes.get(i).width + ","
						+ previewSizes.get(i).height);
			}

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
			camera.setPreviewTexture(cameraTexture);
		} catch (IOException e) {
			Log.v(TAG, "setPreviewTexture exception");
		}
		
		Log.v(TAG, "camera.startPreview");
		camera.startPreview();
		previewStarted = true;
	}

	public void stop() {

		previewStarted = false;
		if ( cameraTexture != null )
		{
			cameraTexture.setOnFrameAvailableListener( null );
		}
		if ( camera != null )
		{
			Log.v(TAG, "camera.stopPreview");
			camera.stopPreview();
			camera.release();
			camera = null;
		}
	}

	public void pause() {
		if (camera != null) {
			camera.stopPreview();
		}
	}

	public void resume() {
		if (camera != null) {
			camera.startPreview();
		}
	}

	native SurfaceTexture createCameraTexture();
	
	native void construct(Activity activity);

}
