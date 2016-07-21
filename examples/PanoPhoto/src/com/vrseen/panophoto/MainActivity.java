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
import android.view.KeyEvent;
import java.io.File;
import java.lang.String;
import java.util.*;


public class MainActivity extends VrActivity {

	PanoPhoto photo = null;

	/** Load jni .so on initialization */
	static {
		System.loadLibrary("panophoto");
	}
	public String[] picture ;
	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		Log.d(TAG, "onCreate");
		super.onCreate(savedInstanceState);
		Intent intent = getIntent();
		Uri uri = intent.getData();
		String photoPath = uri != null ? uri.toString() : null;
		if (photoPath == null || photoPath.isEmpty()) {
			photoPath = Environment.getExternalStorageDirectory().getAbsolutePath() + "/VRSeen/SDK/360Photos/";
			picture =GetVideoFileName(photoPath);
			photoPath =photoPath + picture[0];
		}

		photo = new PanoPhoto(this);
		photo.start(photoPath);
	}
	public static String[] GetVideoFileName(String fileAbsolutePath) {
		List<String> vecFile =new ArrayList<String>();
		File file = new File(fileAbsolutePath);
		File[] subFile = file.listFiles();
		for (int iFileLength = 0; iFileLength < subFile.length; iFileLength++) {
			// 判断是否为文件夹
			if (!subFile[iFileLength].isDirectory()) {
				String filename = subFile[iFileLength].getName();
				// 判断是否为MP4结尾
				if (filename.trim().toLowerCase().endsWith("nz.jpg")) {
					vecFile.add(filename);
				}
				else if (!filename.trim().toLowerCase().endsWith("z.jpg")&& !filename.trim().toLowerCase().endsWith("y.jpg")&& !filename.trim().toLowerCase().endsWith("x.jpg")&& filename.trim().toLowerCase().endsWith("jpg")){
					vecFile.add(filename);
				}
			}
		}
		String[] newStr =  vecFile.toArray(new String[1]);
		return newStr;
	}

    public int i =0;
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		// TODO Auto-generated method stub
		if (keyCode == KeyEvent.KEYCODE_VOLUME_UP) {
			i = i - 1;
			if (i==-1){ i =picture.length-1;}
			photo.start(Environment.getExternalStorageDirectory().getAbsolutePath() +"/VRSeen/SDK/360Photos/"+picture[i]);
			return true;
		} else if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
			i = i + 1;
			if (i==picture.length){ i = 0;}
			photo.start(Environment.getExternalStorageDirectory().getAbsolutePath() +"/VRSeen/SDK/360Photos/"+picture[i]);
			return true;
		}else {
			return super.onKeyDown(keyCode, event);
		}
	}

}
