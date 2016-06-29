package com.vrseen.panophoto;

import android.app.Activity;

public class PanoPhoto {
    native void construct(Activity activity);
    native void onStart(String path);

    public PanoPhoto(Activity activity) {
        construct(activity);
    }

    public void start(String pathName) {
        onStart(pathName);
    }
}
