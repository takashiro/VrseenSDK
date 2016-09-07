package com.vrseen.vrlauncher;

import android.app.Activity;

public class VRLauncher {
    native void construct(Activity activity);
    native void setBackground(String path);

    public VRLauncher(Activity activity) {
        construct(activity);
    }
}
