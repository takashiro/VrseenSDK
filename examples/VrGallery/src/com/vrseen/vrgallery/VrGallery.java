package com.vrseen.vrgallery;

import android.app.Activity;

public class VrGallery {
    native void construct(Activity activity);
    native void onStart(String path);

    public VrGallery(Activity activity) {
        construct(activity);
    }

    public void start(String pathName) {
        onStart(pathName);
    }
}
