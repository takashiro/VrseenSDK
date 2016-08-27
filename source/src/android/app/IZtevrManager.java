package android.app;

public interface IZtevrManager{

	public static String VR_MANAGER = "ztevr";

	public String vrManagerVersion();
	public String vrOVRVersion();

	public boolean vrSetSingleBuf(int open);
	public boolean vrenableVRMode(int mode);
	public void vrdefaultFreq();
	public void vrfullFreq();

	public boolean setThreadSchedFifo( String pkg, int pid, int tid, int prio );
}