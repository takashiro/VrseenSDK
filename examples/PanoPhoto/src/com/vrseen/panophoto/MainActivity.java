/************************************************************************************

Filename    :   MainActivity.java
Content     :
Created     :
Authors     :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Oculus360Photos/ directory. An additional grant
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/
package com.vrseen.panophoto;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import com.vrseen.VrActivity;

public class MainActivity extends VrActivity {

	PanoPhoto photo = null;

	/** Load jni .so on initialization */
	static {
		System.loadLibrary("panophoto");
	}

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		Log.d(TAG, "onCreate");
		super.onCreate(savedInstanceState);

		Intent intent = getIntent();
		Uri uri = intent.getData();
		String photoPath = uri != null ? uri.toString() : null;
		if (photoPath == null || photoPath.isEmpty()) {
			photoPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/VRSeen/SDK/360Photos/1.jpg";
			//photoPath = "/storage/extSdCard/VRSeen/SDK/360Photos/1.jpg";
		}

		photo = new PanoPhoto(this);
		photo.start(photoPath);
	}
}
