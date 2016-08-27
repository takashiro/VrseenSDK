package android.app;

public interface IVRSeenManager {

	public static String VR_MANAGER ="VRSeenService";

	String vrManagerVersion();
	boolean vrenableVRMode(int mode);
	boolean setThreadSchedFifo( String pkg, int pid, int tid, int prio );
	boolean setFreq( String pkg, int gpuFreqLev, int cpuFreqLev );
	boolean relFreq( String pkg );
	int GetPowerLevelState();
}
