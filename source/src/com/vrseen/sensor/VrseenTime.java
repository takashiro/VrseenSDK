package com.vrseen.sensor;


public class VrseenTime {
	static long getCurrentTime() {
		return NativeTime.getCurrentTime();
	}
}

class NativeTime {
    static native long getCurrentTime();
}
