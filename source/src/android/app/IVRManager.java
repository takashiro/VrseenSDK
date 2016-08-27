package android.app;

import android.content.Intent;
import android.graphics.Bitmap;

public interface IVRManager {
	public static String VR_MANAGER ="vr";
	public static String VR_COMFORT_VIEW = "comfortable_view";
	public static String VR_BRIGHTNESS = "bright";
	public static String VR_DO_NOT_DISTURB = "do_not_disturb_mode";

	boolean isActive();
	String vrManagerVersion();
	String vrOVRVersion();
	void reportApplicationInVR( String pkg, boolean activeInVR );
	boolean setThreadSchedFifo( String pkg, int pid, int tid, int prio );
	boolean elevateProcessThread( String pkg, int pid, int tid, int prio );
	boolean demoteProcessThread( String pkg, int pid, int tid, int prio );
	boolean setFreq( String pkg, int gpuFreqLev, int cpuFreqLev );
	int[] SetVrClocks( String pkg, int cpuLevel, int gpuLevel );
	boolean relFreq( String pkg );
	int[] return2EnableFreqLev();
	int GetPowerLevelState();
	void setOption( String optionName, String value );
	String getOption( String optionName );
	void setSystemOption( String optionName, String value );
	String getSystemOption( String optionName );
	Bitmap compositeSystemNotifications();

}