package com.vrseen.nervgear;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.media.AudioManager;
import android.media.SoundPool;
import android.util.Log;

public class SoundManager {
	private static String TAG = "Sound";
	
	// For trivial feedback sound effects
	private Context context;
	private SoundPool soundPool;
	private List<Integer> soundPoolSoundIds;
	private List<String> soundPoolSoundNames;
	
	@SuppressWarnings("deprecation")
	SoundManager(Context context) {
		this.context = context;
		soundPool = new SoundPool(3 /* voices */, AudioManager.STREAM_MUSIC, 100);
		soundPoolSoundIds = new ArrayList<Integer>();
		soundPoolSoundNames = new ArrayList<String>();
	}
	
	@Override
	protected void finalize() {
		soundPoolSoundIds.clear();
		soundPoolSoundNames.clear();
	}
	
	public void playSoundPoolSound(String name) {
		for (int i = 0; i < soundPoolSoundNames.size(); i++) {
			if (soundPoolSoundNames.get(i).equals(name)) {
				soundPool.play(soundPoolSoundIds.get(i), 1.0f, 1.0f, 1, 0, 1);
				return;
			}
		}

		Log.d(TAG, "playSoundPoolSound: loading " + name);

		// check first if this is a raw resource
		int soundId = 0;
		if (name.indexOf("res/raw/") == 0) {
			String resourceName = name.substring(4, name.length() - 4);
			int id = context.getResources().getIdentifier(resourceName, "raw", context.getPackageName());
			if (id == 0) {
				Log.e(TAG, "No resource named " + resourceName);
			} else {
				AssetFileDescriptor afd = context.getResources().openRawResourceFd(id);
				soundId = soundPool.load(afd, 1);
			}
		} else {
			try {
				AssetFileDescriptor afd = context.getAssets().openFd(name);
				soundId = soundPool.load(afd, 1);
			} catch (IOException t) {
				Log.e(TAG, "Couldn't open " + name + " because " + t.getMessage());
			}
		}

		if (soundId == 0) {
			// Try to load the sound directly - works for absolute path - for
			// wav files for sdcard for ex.
			soundId = soundPool.load(name, 1);
		}

		soundPoolSoundNames.add(name);
		soundPoolSoundIds.add(soundId);

		soundPool.play(soundPoolSoundIds.get(soundPoolSoundNames.size() - 1), 1.0f, 1.0f, 1, 0, 1);
	}

}
