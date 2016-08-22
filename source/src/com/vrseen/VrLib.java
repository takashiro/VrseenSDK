package com.vrseen;

import android.app.Activity;
import android.app.IVRManager;
import android.app.IVRSeenManager;
import android.app.IZtevrManager;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ContentResolver;
import android.media.AudioManager;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.Uri;
import android.os.Build;
import android.text.format.Time;
import android.util.Log;
import android.view.Choreographer;
import android.view.Surface;
import android.os.Bundle;
import android.os.Environment;
import android.os.StatFs;
import android.provider.Settings;
import android.view.WindowManager;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.PackageInfo;
import android.content.ComponentName;
import android.widget.Toast;

import java.io.File;
import java.util.Locale;

/*
 * Static methods holding java code needed by VrLib.
 */
public class VrLib implements android.view.Choreographer.FrameCallback,
		AudioManager.OnAudioFocusChangeListener {

	public static final String TAG = "VrLib";
	public static VrLib handler = new VrLib();

	public static final String INTENT_KEY_CMD = "intent_cmd";
	public static final String INTENT_KEY_FROM_PKG = "intent_pkg";

	public enum VRDeviceType
	{
		VRDeviceType_Unknown(-1),
		VRDeviceType_Common(0),
		VRDeviceType_Sumsung(1),
		VRDeviceType_ZTE(2),
		VRDeviceType_VIVO(3)
		{
		};

		private int value;
		private VRDeviceType(int value)
		{
			this.value = value;
		}

		public int getValue()
		{
			return value;
		}

		public static VRDeviceType valueOf(int value)
		{
			switch (value)
			{
				case -1:
					return VRDeviceType_Unknown;
				case 0:
					return VRDeviceType_Common;
				case 1:
					return VRDeviceType_Sumsung;
				case 2:
					return VRDeviceType_ZTE;
				case 3:
					return VRDeviceType_VIVO;
				default:
					return VRDeviceType_Unknown;
			}
		}
	}

	public static VRDeviceType getVRDeviceType() {
		if ((Build.MODEL.contains("SM-N910"))
				|| (Build.MODEL.contains("SM-N916"))
				|| (Build.MODEL.contains("SM-N920"))
				|| (Build.MODEL.contains("SM-G920"))
				|| (Build.MODEL.contains("SM-G925"))
				|| (Build.MODEL.contains("SM-G928"))
				) {
			return VRDeviceType.VRDeviceType_Sumsung;
		}else if(Build.MODEL.contains("ZTE A2017"))
		{
			return VRDeviceType.VRDeviceType_ZTE;
		} else if(Build.MODEL.contains("vivo Xplay5S")) {
            return VRDeviceType.VRDeviceType_VIVO;
		}
		return VRDeviceType.VRDeviceType_Common;
	}

	public static boolean isSupportedSingleBuffer()
	{
		if (       (Build.MODEL.contains("SM-N910"))
				|| (Build.MODEL.contains("SM-N916"))
				|| (Build.MODEL.contains("SM-N920"))
				|| (Build.MODEL.contains("SM-G920"))
				|| (Build.MODEL.contains("SM-G925"))
				|| (Build.MODEL.contains("SM-G928"))
				|| (Build.MODEL.contains("ZTE A2017"))
				|| (Build.MODEL.contains("vivo Xplay5S"))
			)
		{
			return true;
		}
		return false;
	}

	public static boolean isZteVRDevice()
	{
		return Build.MODEL.contains("ZTE A2017");
	}

	public static String getCommandStringFromIntent( Intent intent ) {
		String commandStr = "";
		if ( intent != null ) {
			commandStr = intent.getStringExtra( INTENT_KEY_CMD );
			if ( commandStr == null ) {
				commandStr = "";
			}
		}
		return commandStr;
	}

	public static String getPackageStringFromIntent( Intent intent ) {
		String packageStr = "";
		if ( intent != null ) {
			packageStr = intent.getStringExtra( INTENT_KEY_FROM_PKG );
			if ( packageStr == null ) {
				packageStr = "";
			}
		}
		return packageStr;
	}

	public static String getUriStringFromIntent( Intent intent ) {
		String uriString = "";
		if ( intent != null ) {
			Uri uri = intent.getData();
			if ( uri != null ) {
				uriString = uri.toString();
				if ( uriString == null ) {
					uriString = "";
				}
			}
		}
		return uriString;
	}

	public static void sendIntent( Activity activity, Intent intent ) {
		try {
			Log.d( TAG, "startActivity " + intent );
			intent.addFlags( Intent.FLAG_ACTIVITY_NO_ANIMATION );
			activity.startActivity( intent );
			activity.overridePendingTransition( 0, 0 );
		}
		catch( ActivityNotFoundException e ) {
			Log.d( TAG, "startActivity " + intent + " not found!" );
		}
		catch( Exception e ) {
			Log.e( TAG, "sendIntentFromNative threw exception " + e );
		}
	}

	// this creates an explicit intent
	public static void sendIntentFromNative( Activity activity, String actionName, String toPackageName, String toClassName, String commandStr, String uriStr ) {
		Log.d( TAG, "SendIntentFromNative: action: '" + actionName + "' toPackage: '" + toPackageName + "/" + toClassName + "' command: '" + commandStr + "' uri: '" + uriStr + "'" );

		Intent intent = new Intent( actionName );
		if ( toPackageName != null && !toPackageName.isEmpty() && toClassName != null && !toClassName.isEmpty() ) {
			// if there is no class name, this is an implicit intent. For launching another app with an
			// action + data, an implicit intent is required (see: http://developer.android.com/training/basics/intents/sending.html#Build)
			// i.e. we cannot send a non-launch intent
			ComponentName cname = new ComponentName( toPackageName, toClassName );
			intent.setComponent( cname );
			Log.d( TAG, "Sending explicit intent: " + cname.flattenToString() );
		}

		if ( uriStr.length() > 0 ) {
			intent.setData( Uri.parse( uriStr ) );
		}

		intent.putExtra( INTENT_KEY_CMD, commandStr );
		intent.putExtra( INTENT_KEY_FROM_PKG, activity.getApplicationContext().getPackageName() );

		sendIntent( activity, intent );
	}

	public static void broadcastIntent( Activity activity, String actionName, String toPackageName, String toClassName, String commandStr, String uriStr ) {
		Log.d( TAG, "broadcastIntent: action: '" + actionName + "' toPackage: '" + toPackageName + "/" + toClassName + "' command: '" + commandStr + "' uri: '" + uriStr + "'" );

		Intent intent = new Intent( actionName );
		if ( toPackageName != null && !toPackageName.isEmpty() && toClassName != null && !toClassName.isEmpty() ) {
			// if there is no class name, this is an implicit intent. For launching another app with an
			// action + data, an implicit intent is required (see: http://developer.android.com/training/basics/intents/sending.html#Build)
			// i.e. we cannot send a non-launch intent
			ComponentName cname = new ComponentName( toPackageName, toClassName );
			intent.setComponent( cname );
			Log.d( TAG, "Sending explicit broadcast: " + cname.flattenToString() );
		}

		if ( uriStr.length() > 0 ) {
			intent.setData( Uri.parse( uriStr ) );
		}

		intent.putExtra( INTENT_KEY_CMD, commandStr );
		intent.putExtra( INTENT_KEY_FROM_PKG, activity.getApplicationContext().getPackageName() );

		activity.sendBroadcast( intent );
	}

	// this gets the launch intent from the specified package name
	public static void sendLaunchIntent( Activity activity, String packageName, String commandStr, String uriStr ) {
		Log.d( TAG, "sendLaunchIntent: '" + packageName + "' command: '" + commandStr + "' uri: '" + uriStr + "'" );

		Intent launchIntent = activity.getPackageManager().getLaunchIntentForPackage( packageName );
		if ( launchIntent == null ) {
			Log.d( TAG, "sendLaunchIntent: null destination activity" );
			return;
		}

		launchIntent.putExtra( INTENT_KEY_CMD, commandStr );
		launchIntent.putExtra( INTENT_KEY_FROM_PKG, activity.getApplicationContext().getPackageName() );

		if ( uriStr.length() > 0 ) {
			launchIntent.setData( Uri.parse( uriStr ) );
		}

		sendIntent( activity, launchIntent );
	}

	public static String MOUNT_HANDLED_INTENT = "com.oculus.mount_handled";

	public static void notifyMountHandled( Activity activity )
	{
		Log.d( TAG, "notifyMountHandled" );
		Intent i = new Intent( MOUNT_HANDLED_INTENT );
		activity.sendBroadcast( i );
	}

	public static void logApplicationName( Activity act )
	{
		int stringId = act.getApplicationContext().getApplicationInfo().labelRes;
		String name = act.getApplicationContext().getString( stringId );
		Log.d( TAG, "APP = " + name );
	}

	public static void logApplicationVersion( Activity act )
	{
		String versionName = "<none>";
        String internalVersionName = "<none>";
		int versionCode = 0;
		final PackageManager packageMgr = act.getApplicationContext().getPackageManager();
		if ( packageMgr != null )
		{
			try {
				PackageInfo packageInfo = packageMgr.getPackageInfo( act.getApplicationContext().getPackageName(), 0 );
				versionName = packageInfo.versionName;
				versionCode = packageInfo.versionCode;
				ApplicationInfo appInfo = act.getPackageManager().getApplicationInfo( act.getApplicationContext().getPackageName(), PackageManager.GET_META_DATA );
				if ( ( appInfo != null ) && ( appInfo.metaData != null ) )
				{
					internalVersionName = appInfo.metaData.getString("internalVersionName", "<none>" );
				}
			} catch ( PackageManager.NameNotFoundException e ) {
				versionName = "<none>";
				versionCode = 0;
			}
		}

		Log.d( TAG, "APP VERSION = " + versionName + " versionCode " + versionCode + " internalVersionName " + internalVersionName);
	}

	public static void logApplicationVrType( Activity act )
	{
		String vrType = "<none>";
		final PackageManager packageMgr = act.getApplicationContext().getPackageManager();
		if ( packageMgr != null )
		{
			try {
				PackageInfo packageInfo = packageMgr.getPackageInfo( act.getApplicationContext().getPackageName(), 0 );
				ApplicationInfo appInfo = act.getPackageManager().getApplicationInfo( act.getApplicationContext().getPackageName(), PackageManager.GET_META_DATA );
				if ( ( appInfo != null ) && ( appInfo.metaData != null ) )
				{
					vrType = appInfo.metaData.getString( "com.samsung.android.vr.application.mode", "<none>" );
				}
			} catch ( PackageManager.NameNotFoundException e ) {
				vrType = "<none>";
			}
		}

		Log.d( TAG, "APP VR_TYPE = " + vrType );
	}

	public static void setActivityWindowFullscreen( final Activity act )
	{
		act.runOnUiThread( new Runnable()
		{
			@Override
			public void run()
			{
				Log.d( TAG, "getWindow().addFlags( WindowManager.LayoutParams.FLAG_FULLSCREEN )" );
				act.getWindow().addFlags( WindowManager.LayoutParams.FLAG_FULLSCREEN );
			}
		});
	}

	public static boolean isDeveloperMode( final Activity act ) {
		return Settings.Global.getInt( act.getContentResolver(), "vrmode_developer_mode", 0 ) != 0;
	}


	// zx_add_code 2016.8.1
	public static int vrEnableVRModeStatic(final Activity activity, int mode) {

		Log.v(TAG, "********vrEnableVRModeStatic******");

		if (VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_ZTE)
		{
			// use ZTE VR instead, in future we will call buildModelService
			android.app.IZtevrManager ztevr = (android.app.IZtevrManager) activity
					.getSystemService(IZtevrManager.VR_MANAGER);
			if (ztevr == null) {
				Log.d(TAG, "IZtevrManager not found");
				return 0;
			}

			try {
				if (ztevr.vrenableVRMode(mode)) {
					Log.d(TAG, "IZtevrManager vrenableVRMode " );
					if(mode == 1)
						ScreenBrightnessTool.enterVrMode(activity);
					else
						ScreenBrightnessTool.exitVrMode();
					return 1;
				} else {
					Log.d(TAG, "IZtevrManager failed to vrenableVRMode");
					return -1;
				}
			} catch (NoSuchMethodError e) {
				Log.d(TAG, "vrenableVRMode does not exist");
				return -2;
			}
		}else if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_VIVO)
		{
			android.app.IVRSeenManager vivovr = (android.app.IVRSeenManager) activity
					.getSystemService(IVRSeenManager.VR_MANAGER);
			if (vivovr == null) {
				Log.d(TAG, "IVRSeenManager not found");
				return 0;
			}

			try {
				if (vivovr.vrenableVRMode(mode)) {
					Log.d(TAG, "IVRSeenManager vrenableVRMode " );
					if(mode == 1)
						ScreenBrightnessTool.enterVrMode(activity);
					else
						ScreenBrightnessTool.exitVrMode();
					return 1;
				} else {
					Log.d(TAG, "IVRSeenManager failed to vrenableVRMode");
					return -1;
				}
			} catch (NoSuchMethodError e) {
				Log.d(TAG, "vrenableVRMode does not exist");
				return -2;
			}
		}

		return -2;
	}

	public static void destoryScreenBrightnessTool()
	{
		ScreenBrightnessTool.destory();
	}

	public static int setSchedFifoStatic( final Activity activity, int tid, int rtPriority ) {
        Log.d(TAG, "setSchedFifoStatic tid:" + tid + " pto:" + rtPriority);

        if (VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_Sumsung) {
            IVRManager vrmanager = (android.app.IVRManager) activity.getSystemService(android.app.IVRManager.VR_MANAGER);
            if (vrmanager == null) {
                Log.d(TAG, "VRManager was not found");
                return -1;
            }

            try {
                try {
                    if (vrmanager.setThreadSchedFifo(activity.getPackageName(), android.os.Process.myPid(), tid, rtPriority)) {
                        Log.d(TAG, "VRManager set thread priority to " + rtPriority);
                        return 1;
                    } else {
                        Log.d(TAG, "VRManager failed to set thread priority");
                        return -1;
                    }
                } catch (NoSuchMethodError e) {
                    Log.d(TAG, "Thread priority API does not exist");
                    return -2;
                }
            } catch (SecurityException s) {
                Log.d(TAG, "Thread priority security exception");
                //zx_note 2016.8.2
//				activity.runOnUiThread( new Runnable()
//				{
//					@Override
//					public void run()
//					{
//						Toast toast = Toast.makeText( activity.getApplicationContext(),
//								"VRManager case unknown error!",
//								Toast.LENGTH_SHORT );
//						toast.show();
//					}
//				} );
                // if we don't wait here, the app can exit before we see the toast
                long startTime = System.currentTimeMillis();
                do {
                } while (System.currentTimeMillis() - startTime < 100);

                return -3;
            }
        } else if (VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_ZTE) {
            IZtevrManager ztevr = (android.app.IZtevrManager) activity.getSystemService(IZtevrManager.VR_MANAGER);
            if (ztevr == null) {
                Log.d(TAG, "IZtevrManager was not found");
                return -1;
            }

            try {
                try {
                    if (ztevr.setThreadSchedFifo(activity.getPackageName(), android.os.Process.myPid(), tid, rtPriority )) {
                        Log.d(TAG, "IZtevrManager set thread priority to " + rtPriority);
                        return 1;
                    } else {
                        Log.d(TAG, "IZtevrManager failed to set thread priority");
                        return -1;
                    }
                } catch (NoSuchMethodError e) {
                    Log.d(TAG, "Thread priority API does not exist");
                    return -2;
                }
            } catch (SecurityException s) {
                Log.d(TAG, "Thread priority security exception");
                //zx_note 2016.8.2
//				activity.runOnUiThread( new Runnable()
//				{
//					@Override
//					public void run()
//					{
//						Toast toast = Toast.makeText( activity.getApplicationContext(),
//								"VRManager case unknown error!",
//								Toast.LENGTH_LONG );
//						toast.show();
//					}
//				} );VO
                // if we don't wait here, the app can exit before we see the toast
                long startTime = System.currentTimeMillis();
                do {
                } while (System.currentTimeMillis() - startTime < 100);

                return -3;
            }
        } else if (VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_VIVO) {
            IVRSeenManager vivovr = (android.app.IVRSeenManager) activity.getSystemService(IVRSeenManager.VR_MANAGER);
            if (vivovr == null) {
                Log.d(TAG, "IVRSeenManager was not found");
                return -1;
            }

            try {
                try {
                    if (vivovr.setThreadSchedFifo(activity.getPackageName(), android.os.Process.myPid(), tid, rtPriority)) {
                        Log.d(TAG, "IVRSeenManager set thread priority to " + rtPriority);
                        return 1;
                    } else {
                        Log.d(TAG, "IVRSeenManager failed to set thread priority");
                        return -1;
                    }
                } catch (NoSuchMethodError e) {
                    Log.d(TAG, "Thread priority API does not exist");
                    return -2;
                }
            } catch (SecurityException s) {
                Log.d(TAG, "Thread priority security exception");
                //zx_note 2016.8.2
//				activity.runOnUiThread( new Runnable()
//				{
//					@Override
//					public void run()
//					{
//						Toast toast = Toast.makeText( activity.getApplicationContext(),
//								"VRManager case unknown error!",
//								Toast.LENGTH_LONG );
//						toast.show();
//					}
//				} );
                // if we don't wait here, the app can exit before we see the toast
                long startTime = System.currentTimeMillis();
                do {
                } while (System.currentTimeMillis() - startTime < 100);

                return -3;
            }
        }

        return -1;
    }

	static int [] defaultClockLevels = { -1, -1, -1, -1 };
	public static int[] getAvailableFreqLevels(  Activity activity )
	{
		android.app.IVRManager vr = (android.app.IVRManager) activity.getSystemService(android.app.IVRManager.VR_MANAGER);
		if ( vr == null ) {
			Log.d(TAG, "VRManager was not found");
			return defaultClockLevels;
		}

		try {
			int [] values = vr.return2EnableFreqLev();
			// display available levels
			Log.d(TAG, "Available levels: {GPU MIN = " + values[0] + ", GPU MAX = " + values[1] + ", CPU MIN = " + values[2] + ", CPU MAX = " + values[3] + "}");
			for ( int i = 0; i < values.length; i++ ) {
				Log.d(TAG, "-> " + "/ " + values[i]);
			}
			return values;
		} catch (NoSuchMethodError e ) {
			return defaultClockLevels;
		}
	}

	static int [] defaultClockFreq = { -1, -1, 0, 0 };
	public static int [] setSystemPerformanceStatic( Activity activity,
			int cpuLevel, int gpuLevel )
	{
		Log.d(TAG, "setSystemPerformance cpu: " + cpuLevel + " gpu: " + gpuLevel);

		if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_Sumsung)
		{
			android.app.IVRManager vr = (android.app.IVRManager)activity.getSystemService(android.app.IVRManager.VR_MANAGER);
			if ( vr == null ) {
				Log.d(TAG, "VRManager was not found");
				return defaultClockFreq;
			}

			// lock the frequency
			try
			{
				int[] values = vr.SetVrClocks( activity.getPackageName(), cpuLevel, gpuLevel );
				Log.d(TAG, "SetVrClocks: {CPU CLOCK, GPU CLOCK, POWERSAVE CPU CLOCK, POWERSAVE GPU CLOCK}" );
				for ( int i = 0; i < values.length; i++ ) {
					Log.d(TAG, "-> " + "/ " + values[i]);
				}
				return values;
			} catch( NoSuchMethodError e ) {
				// G906S api differs from Note4
				int[] values = { 0, 0, 0, 0 };
				boolean success = vr.setFreq( activity.getPackageName(), cpuLevel, gpuLevel );
				Log.d(TAG, "setFreq returned " + success );
				return values;
			}

		}else if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_ZTE)
		{
			android.app.IZtevrManager ztevr = (android.app.IZtevrManager) activity
					.getSystemService(IZtevrManager.VR_MANAGER);
			if (ztevr == null) {
				Log.d(TAG, "IZtevrManager not found");
				return defaultClockFreq;
			}

			// lock the frequency
			try {
				int[] values = { 0, 0, 0, 0 };
				if( cpuLevel <= 1 && gpuLevel <= 1)
				{
					ztevr.vrdefaultFreq();
					Log.v(TAG, "********set to default freq **********");
				}
				else
				{
					ztevr.vrfullFreq();
					values[0] = values[1] = values[2] = values[3] = 3;
					Log.v(TAG, "***********set to full freq **********");
				}
				return values;
			} catch (NoSuchMethodError e) {
				// G906S api differs from Note4
				int[] values = { 0, 0, 0, 0 };
				Log.d(TAG, "failed to set frequency returned ");
				return values;
			}
		} else if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_VIVO)
        {
            android.app.IVRSeenManager vivovr = (android.app.IVRSeenManager) activity
                    .getSystemService(IVRSeenManager.VR_MANAGER);
            if (vivovr == null) {
                Log.d(TAG, "IVRSeenManager not found");
                return defaultClockFreq;
            }

            // lock the frequency
            try {
				int[] values = { 0, 0, 0, 0 };
				boolean success = vivovr.setFreq( activity.getPackageName(), cpuLevel, gpuLevel );
				Log.d(TAG, "setFreq returned " + success );
                return values;
            } catch (NoSuchMethodError e) {
                // G906S api differs from Note4
                int[] values = { 0, 0, 0, 0 };
                Log.d(TAG, "failed to set frequency returned ");
                return values;
            }
        }

		int[] values = { 0, 0, 0, 0 };
		return values;
	}

	public static void releaseSystemPerformanceStatic( Activity activity )
	{
		Log.d(TAG, "releaseSystemPerformanceStatic");

		if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_Sumsung)
		{
			android.app.IVRManager vr = (android.app.IVRManager)activity.getSystemService(IVRManager.VR_MANAGER);
			if ( vr == null ) {
				Log.d(TAG, "VRManager was not found");
				return;
			}
			// release the frequency locks
			vr.relFreq( activity.getPackageName() );
		}else if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_ZTE)
		{
			android.app.IZtevrManager ztevr = (android.app.IZtevrManager) activity
					.getSystemService(IZtevrManager.VR_MANAGER);
			if (ztevr == null) {
				Log.d(TAG, "IZtevrManager not found");
				return;
			}

			ztevr.vrdefaultFreq();
		} else if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_VIVO)
        {
            android.app.IVRSeenManager vivovr = (android.app.IVRSeenManager) activity
                    .getSystemService(IVRSeenManager.VR_MANAGER);
            if (vivovr == null) {
                Log.d(TAG, "IVivoVrManager not found");
                return;
            }

			boolean sucess = vivovr.relFreq(activity.getPackageName());//zx_note
			Log.d(TAG,"vivovr.relFreq return : "+sucess);
        }

		Log.d(TAG, "Releasing frequency lock");
	}

	public static int getPowerLevelState( Activity act ) {
		//Log.d(TAG, "getPowerLevelState" );

		int level = 0;
		if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_Sumsung)
		{
			android.app.IVRManager vr = (android.app.IVRManager)act.getSystemService(IVRManager.VR_MANAGER);
			if ( vr == null ) {
				Log.d(TAG, "VRManager was not found" );
				return level;
			}

			try {
				level = vr.GetPowerLevelState();
			} catch (NoSuchMethodError e) {
				//Log.d( TAG, "getPowerLevelState api does not exist");
			}
		}else if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_VIVO)
		{
			android.app.IVRSeenManager vivovr = (android.app.IVRSeenManager)act.getSystemService(IVRSeenManager.VR_MANAGER);
			if ( vivovr == null ) {
				Log.d(TAG, "IVRSeenManager was not found" );
				return level;
			}

			try {
				level = vivovr.GetPowerLevelState();
			} catch (NoSuchMethodError e) {
				//Log.d( TAG, "getPowerLevelState api does not exist");
			}
			return level;
		}
		return level;
	}

	// Get the system brightness level, return value in the [0,255] range
	public static int getSystemBrightness( Activity act ) {
		Log.d(TAG, "getSystemBrightness" );

		int bright = 50;

		if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_Sumsung)
		{
			// Get the current system brightness level by way of VrManager
			android.app.IVRManager vr = (android.app.IVRManager)act.getSystemService(IVRManager.VR_MANAGER);
			if ( vr == null ) {
				Log.d(TAG, "VRManager was not found" );
				return bright;
			}

			String result = vr.getSystemOption( android.app.IVRManager.VR_BRIGHTNESS );
			bright = Integer.parseInt( result );
		}else if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_ZTE)
		{

		}
		return bright;
	}

	// Set system brightness level, input value in the [0,255] range
	public static void setSystemBrightness( Activity act, int brightness ) {
		Log.d(TAG, "setSystemBrightness " + brightness );
		//assert brightness >= 0 && brightness <= 255;
		if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_Sumsung)
		{
			android.app.IVRManager vr = (android.app.IVRManager)act.getSystemService(IVRManager.VR_MANAGER);
			if ( vr == null ) {
				Log.d(TAG, "VRManager was not found" );
				return;
			}
			vr.setSystemOption( android.app.IVRManager.VR_BRIGHTNESS, Integer.toString( brightness ));
		}else if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_ZTE)
		{

		}

	}

	// Comfort viewing mode is a low blue light mode.

	// Returns true if system comfortable view mode is enabled
	public static boolean getComfortViewModeEnabled( Activity act ) {
		Log.d(TAG, "getComfortViewModeEnabled" );
		if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_Sumsung)
		{
			android.app.IVRManager vr = (android.app.IVRManager) act.getSystemService(IVRManager.VR_MANAGER);
			if ( vr == null ) {
				Log.d(TAG, "VRManager was not found" );
				return false;
			}

			String result = vr.getSystemOption( android.app.IVRManager.VR_COMFORT_VIEW );
			return ( result.equals( "1" ) );
		} else if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_ZTE)
		{

		}
		return  false;
	}

	// Enable system comfort view mode
	public static void enableComfortViewMode( Activity act, boolean enable ) {
		Log.d(TAG, "enableComfortableMode " + enable );

		if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_Sumsung)
		{
			android.app.IVRManager vr = (android.app.IVRManager) act.getSystemService(IVRManager.VR_MANAGER);
			if ( vr == null ) {
				Log.d(TAG, "VRManager was not found" );
				return;
			}
			vr.setSystemOption( android.app.IVRManager.VR_COMFORT_VIEW, enable ? "1" : "0" );
		}else if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_ZTE)
		{

		}

	}

	public static void setDoNotDisturbMode( Activity act, boolean enable )
	{
		Log.d( TAG, "setDoNotDisturbMode " + enable );
		if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_Sumsung)
		{
			android.app.IVRManager vr = (android.app.IVRManager) act.getSystemService(android.app.IVRManager.VR_MANAGER);
			if ( vr == null ) {
				Log.d(TAG, "VRManager was not found" );
				return;
			}

			vr.setSystemOption( android.app.IVRManager.VR_DO_NOT_DISTURB, ( enable ) ? "1" : "0" );

			String result = vr.getSystemOption( android.app.IVRManager.VR_DO_NOT_DISTURB );
			Log.d( TAG, "result after set = " + result );
		}else if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_ZTE)
		{

		}

	}

	public static boolean getDoNotDisturbMode( Activity act )
	{
		Log.d( TAG, "getDoNotDisturbMode " );

		if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_Sumsung)
		{
			android.app.IVRManager vr = (android.app.IVRManager) act.getSystemService(android.app.IVRManager.VR_MANAGER);
			if ( vr == null ) {
				Log.d(TAG, "VRManager was not found" );
				return false;
			}
			String result = vr.getSystemOption( android.app.IVRManager.VR_DO_NOT_DISTURB );
			//Log.d( TAG, "getDoNotDisturb result = " + result );
			return ( result.equals( "1" ) );
		}else if(VrLib.getVRDeviceType() == VRDeviceType.VRDeviceType_ZTE)
		{

		}
		return false;

	}

	// Note that displayMetrics changes in landscape vs portrait mode!
	public static float getDisplayWidth( Activity act ) {
		android.util.DisplayMetrics display = new android.util.DisplayMetrics();
		act.getWindowManager().getDefaultDisplay().getMetrics(display);
		final float METERS_PER_INCH = 0.0254f;
		return (display.widthPixels / display.xdpi) * METERS_PER_INCH;
	}

	public static float getDisplayHeight( Activity act ) {
		android.util.DisplayMetrics display = new android.util.DisplayMetrics();
		act.getWindowManager().getDefaultDisplay().getMetrics(display);
		final float METERS_PER_INCH = 0.0254f;
		return (display.heightPixels / display.ydpi) * METERS_PER_INCH;
	}

	public static boolean isLandscapeApp( Activity act ) {
		int r = act.getWindowManager().getDefaultDisplay().getRotation();
		Log.d(TAG, "getRotation():" + r );
		return ( r == Surface.ROTATION_90 ) || ( r == Surface.ROTATION_270 );
	}

	//--------------- Vsync ---------------
	public static native void nativeVsync(long lastVsyncNano);

	public static Choreographer choreographerInstance;

	public static void startVsync( Activity act ) {
    	act.runOnUiThread( new Thread()
    	{
		 @Override
    		public void run()
    		{
				// Look this up now, so the callback (which will be on the same thread)
				// doesn't have to.
				choreographerInstance = Choreographer.getInstance();

				// Make sure we never get multiple callbacks going.
				choreographerInstance.removeFrameCallback(handler);

				// Start up our vsync callbacks.
				choreographerInstance.postFrameCallback(handler);
    		}
    	});

	}

	// It is important to stop the callbacks when the app is paused,
	// because they consume several percent of a cpu core!
	public static void stopVsync( Activity act ) {
		// This may not need to be run on the UI thread, but it doesn't hurt.
    	act.runOnUiThread( new Thread()
    	{
		 @Override
    		public void run()
    		{
				choreographerInstance.removeFrameCallback(handler);
    		}
    	});
	}

	public void doFrame(long frameTimeNanos) {
		nativeVsync(frameTimeNanos);
		choreographerInstance.postFrameCallback(this);
	}

	//--------------- Audio Focus -----------------------

	public static void requestAudioFocus( Activity act )
	{
		AudioManager audioManager = (AudioManager)act.getSystemService( Context.AUDIO_SERVICE );

		// Request audio focus
		int result = audioManager.requestAudioFocus( handler, AudioManager.STREAM_MUSIC,
			AudioManager.AUDIOFOCUS_GAIN );
		if ( result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED )
		{
			Log.d(TAG,"requestAudioFocus(): GRANTED audio focus");
		}
		else if ( result == AudioManager.AUDIOFOCUS_REQUEST_FAILED )
		{
			Log.d(TAG,"requestAudioFocus(): FAILED to gain audio focus");
		}
	}

	public static void releaseAudioFocus( Activity act )
	{
		AudioManager audioManager = (AudioManager)act.getSystemService( Context.AUDIO_SERVICE );
		audioManager.abandonAudioFocus( handler );
	}

    public void onAudioFocusChange(int focusChange)
    {
		switch( focusChange )
		{
		case AudioManager.AUDIOFOCUS_GAIN:
			// resume() if coming back from transient loss, raise stream volume if duck applied
			Log.d(TAG, "onAudioFocusChangedListener: AUDIOFOCUS_GAIN");
			break;
		case AudioManager.AUDIOFOCUS_LOSS:				// focus lost permanently
			// stop() if isPlaying
			Log.d(TAG, "onAudioFocusChangedListener: AUDIOFOCUS_LOSS");
			break;
		case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:	// focus lost temporarily
			// pause() if isPlaying
			Log.d(TAG, "onAudioFocusChangedListener: AUDIOFOCUS_LOSS_TRANSIENT");
			break;
		case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:	// focus lost temporarily
			// lower stream volume
			Log.d(TAG, "onAudioFocusChangedListener: AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK");
			break;
		default:
			break;
		}
	}


	//--------------- Broadcast Receivers ---------------

	//==========================================================
	// headsetReceiver
	public static native void nativeHeadsetEvent(int state);

	private static class HeadsetReceiver extends BroadcastReceiver {

		public Activity act;

		@Override
		public void onReceive(Context context, final Intent intent) {

			act.runOnUiThread(new Runnable() {
				public void run() {
					Log.d( TAG, "!$$$$$$! headsetReceiver::onReceive" );
					if (intent.hasExtra("state"))
					{
						int state = intent.getIntExtra("state", 0);
						nativeHeadsetEvent( state );
					}
				}
			});
		}
	}

	public static HeadsetReceiver headsetReceiver = null;
	public static IntentFilter headsetFilter = null;

	public static void startHeadsetReceiver( Activity act ) {

		Log.d( TAG, "Registering headset receiver" );
		if ( headsetFilter == null ) {
			headsetFilter = new IntentFilter(Intent.ACTION_HEADSET_PLUG);
		}
		if ( headsetReceiver == null ) {
			headsetReceiver = new HeadsetReceiver();
		}
		headsetReceiver.act = act;

		act.registerReceiver(headsetReceiver, headsetFilter);

		// initialize with the current headset state
		int state = act.getIntent().getIntExtra("state", 0);
		Log.d( TAG, "startHeadsetReceiver: " + state );
		nativeHeadsetEvent( state );
	}

	public static void stopHeadsetReceiver( Activity act ) {
		Log.d( TAG, "Unregistering headset receiver" );
		act.unregisterReceiver(headsetReceiver);
	}

	//==========================================================
	// VolumeReceiver
	public static native void nativeVolumeEvent(int volume);

	private static class VolumeReceiver extends BroadcastReceiver {

		public Activity act;

		@Override
		public void onReceive(Context context, final Intent intent) {

			act.runOnUiThread(new Runnable() {
				public void run() {
					Log.d(TAG, "OnReceive VOLUME_CHANGED_ACTION" );
					int stream = ( Integer )intent.getExtras().get( "android.media.EXTRA_VOLUME_STREAM_TYPE" );
					int volume = ( Integer )intent.getExtras().get( "android.media.EXTRA_VOLUME_STREAM_VALUE" );
					if ( stream == AudioManager.STREAM_MUSIC )
					{
						Log.d(TAG, "calling nativeVolumeEvent()" );
						nativeVolumeEvent( volume );
					}
					else
					{
						Log.d(TAG, "skipping volume change from stream " + stream );
					}
				}
			});
		}
	}

	public static VolumeReceiver volumeReceiver = null;
	public static IntentFilter volumeFilter = null;

	public static void startVolumeReceiver( Activity act ) {

		Log.d( TAG, "Registering volume receiver" );
		if ( volumeFilter == null ) {
			volumeFilter = new IntentFilter();
			volumeFilter.addAction( "android.media.VOLUME_CHANGED_ACTION" );
		}
		if ( volumeReceiver == null ) {
			volumeReceiver = new VolumeReceiver();
		}
		volumeReceiver.act = act;

		act.registerReceiver(volumeReceiver, volumeFilter);

		AudioManager audio = (AudioManager)act.getSystemService(Context.AUDIO_SERVICE);
		int volume = audio.getStreamVolume(AudioManager.STREAM_MUSIC);
		Log.d( TAG, "startVolumeReceiver: " + volume );
		//nativeVolumeEvent( volume );
	}

	public static void stopVolumeReceiver( Activity act ) {
		Log.d( TAG, "Unregistering volume receiver" );
		act.unregisterReceiver(volumeReceiver);
	}

	public static void startReceivers( Activity act ) {
		startHeadsetReceiver( act );
		startVolumeReceiver( act );
	}

	public static void stopReceivers( Activity act ) {
		stopHeadsetReceiver( act );
		stopVolumeReceiver( act );
	}

	public static void finishOnUiThread( final Activity act ) {
    	act.runOnUiThread( new Runnable()
    	{
		 @Override
    		public void run()
    		{
			 	Log.d(TAG, "finishOnUiThread calling finish()" );
    			act.finish();
    			act.overridePendingTransition(0, 0);
            }
    	});
	}

	public static void finishAffinityOnUiThread( final Activity act ) {
    	act.runOnUiThread( new Runnable()
    	{
		 @Override
    		public void run()
    		{
			 	Log.d(TAG, "finishAffinityOnUiThread calling finish()" );
				act.finishAffinity();
    			act.overridePendingTransition(0, 0);
            }
    	});
	}

	public static boolean getBluetoothEnabled( final Activity act ) {
		return Settings.Global.getInt( act.getContentResolver(),
				Settings.Global.BLUETOOTH_ON, 0 ) != 0;
	}

	public static boolean isAirplaneModeEnabled( final Activity act ) {
		return Settings.Global.getInt( act.getContentResolver(),
				Settings.Global.AIRPLANE_MODE_ON, 0 ) != 0;
	}

	// returns true if time settings specifies 24 hour format
	public static boolean isTime24HourFormat( Activity act ) {
		ContentResolver cr = act.getContentResolver();
		String v = Settings.System.getString( cr, android.provider.Settings.System.TIME_12_24 );
		if ( v == null || v.isEmpty() || v.equals( "12" ) ) {
			return false;
		}
		return true;
	}

	public static boolean packageIsInstalled( Activity act, String packageName ) {
		PackageManager pm = act.getPackageManager();
		try {
			pm.getPackageInfo( packageName,PackageManager.GET_META_DATA );
		} catch ( NameNotFoundException e ) {
			Log.d( TAG, "Package " + packageName + " does NOT exist on device" );
			return false;
		}

		Log.d( TAG, "Package " + packageName + " exists on device" );
		return true;
	}

	public static boolean isActivityInPackage( Activity act, String packageName, String activityName ) {
		Log.d( TAG, "isActivityInPackage( '" + packageName + "', '" + activityName + "' )" );
		PackageManager pm = act.getPackageManager();
		try {
			PackageInfo pi = pm.getPackageInfo( packageName, PackageManager.GET_ACTIVITIES );
			if ( pi == null ) {
				Log.d( TAG, "Could not get package info for " + packageName + "!" );
				return false;
			}
			if ( pi.activities == null ) {
				Log.d( TAG, "Package " + packageName + "has no activities!" );
				return false;
			}

			for ( int i = 0; i < pi.activities.length; i++ ) {
				Log.d( TAG, "activity[" + i + "] = " + pi.activities[i] );
				if ( pi.activities[i] != null && pi.activities[i].name.equals( activityName ) ) {
					Log.d( TAG, "Found activity " + activityName + " in package " + packageName + "!" );
					return true;
				}
			}
		}
		catch ( NameNotFoundException s ) {
			Log.d( TAG, "isActivityInPackage: package " + packageName + " does NOT exist on device!" );
		}
		return false;
	}

	public static boolean isHybridApp( final Activity act ) {
		try {
		    ApplicationInfo appInfo = act.getPackageManager().getApplicationInfo(act.getPackageName(), PackageManager.GET_META_DATA);
		    Bundle bundle = appInfo.metaData;
		    String applicationMode = bundle.getString("com.samsung.android.vr.application.mode");
		    return (applicationMode.equals("dual"));
		} catch( NameNotFoundException e ) {
			e.printStackTrace();
		} catch( NullPointerException e ) {
		    Log.e(TAG, "Failed to load meta-data, NullPointer: " + e.getMessage());
		}

		return false;
	}

	public static boolean isWifiConnected( final Activity act ) {
		ConnectivityManager connManager = ( ConnectivityManager ) act.getSystemService( Context.CONNECTIVITY_SERVICE );
		NetworkInfo mWifi = connManager.getNetworkInfo( ConnectivityManager.TYPE_WIFI );
		return mWifi.isConnected();
	}

	public static String getExternalStorageDirectory() {
		return Environment.getExternalStorageDirectory().getAbsolutePath();
	}

	// Converts some thing like "/sdcard" to "/sdcard/", always ends with "/" to indicate folder path
	public static String toFolderPathFormat( String inStr ) {
		if( inStr == null ||
			inStr.length() == 0	)
		{
			return "/";
		}

		if( inStr.charAt( inStr.length() - 1 ) != '/' )
		{
			return inStr + "/";
		}

		return inStr;
	}

	/*** Internal Storage ***/
	public static String getInternalStorageRootDir() {
		return toFolderPathFormat( Environment.getDataDirectory().getPath() );
	}

	public static String getInternalStorageFilesDir( Activity act ) {
		if ( act != null )
		{
			return toFolderPathFormat( act.getFilesDir().getPath() );
		}
		else
		{
			Log.e( TAG, "Activity is null in getInternalStorageFilesDir method" );
		}
		return "";
	}

	public static String getInternalStorageCacheDir( Activity act ) {
		if ( act != null )
		{
			return toFolderPathFormat( act.getCacheDir().getPath() );
		}
		else
		{
			Log.e( TAG, "activity is null getInternalStorageCacheDir method" );
		}
		return "";
	}

	public static long getInternalCacheMemoryInBytes( Activity act )
	{
		if ( act != null )
		{
			String path = getInternalStorageCacheDir( act );
			StatFs stat = new StatFs( path );
			return stat.getAvailableBytes();
		}
		else
		{
			Log.e( TAG, "activity is null getInternalCacheMemoryInBytes method" );
		}
		return 0;
	}

	/*** External Storage ***/
	public static String getExternalStorageFilesDirAtIdx( Activity act, int idx ) {
		if ( act != null )
		{
			File[] filesDirs = act.getExternalFilesDirs(null);
			if( filesDirs != null && filesDirs.length > idx && filesDirs[idx] != null )
			{
				return toFolderPathFormat( filesDirs[idx].getPath() );
			}
		}
		else
		{
			Log.e( TAG, "activity is null getExternalStorageFilesDirAtIdx method" );
		}
		return "";
	}

	public static String getExternalStorageCacheDirAtIdx( Activity act, int idx ) {
		if ( act != null )
		{
			File[] cacheDirs = act.getExternalCacheDirs();
			if( cacheDirs != null && cacheDirs.length > idx && cacheDirs[idx] != null )
			{
				return toFolderPathFormat( cacheDirs[idx].getPath() );
			}
		}
		else
		{
			Log.e( TAG, "activity is null in getExternalStorageCacheDirAtIdx method with index " + idx );
		}
		return "";
	}

	// Primary External Storage
	public static final int PRIMARY_EXTERNAL_STORAGE_IDX = 0;
	public static String getPrimaryExternalStorageRootDir( Activity act ) {
		return toFolderPathFormat( Environment.getExternalStorageDirectory().getPath() );
	}

	public static String getPrimaryExternalStorageFilesDir( Activity act ) {
		return getExternalStorageFilesDirAtIdx( act, PRIMARY_EXTERNAL_STORAGE_IDX );
	}

	public static String getPrimaryExternalStorageCacheDir( Activity act ) {
		return getExternalStorageCacheDirAtIdx( act, PRIMARY_EXTERNAL_STORAGE_IDX );
	}

	// Secondary External Storage
	public static final int SECONDARY_EXTERNAL_STORAGE_IDX = 1;
	public static String getSecondaryExternalStorageRootDir() {
		return "/storage/extSdCard/";
	}

	public static String getSecondaryExternalStorageFilesDir( Activity act ) {
		return getExternalStorageFilesDirAtIdx( act, SECONDARY_EXTERNAL_STORAGE_IDX );
	}

	public static String getSecondaryExternalStorageCacheDir( Activity act ) {
		return getExternalStorageCacheDirAtIdx( act, SECONDARY_EXTERNAL_STORAGE_IDX );
	}

	// we need this string to track when setLocale has change the language, otherwise if
	// the language is changed with setLocale, we can't determine the current language of
	// the application.
	private static String currentLanguage = null;

	public static void setCurrentLanguage( String lang ) {
		currentLanguage = lang;
		Log.d( TAG, "Current language set to '" + lang + "'." );
	}

	public static String getCurrentLanguage() {
		// In the case of Unity, the activity onCreate does not set the current langage
		// so we need to assume it is defaulted if setLocale() has never been called
		if ( currentLanguage == null || currentLanguage.isEmpty() ) {
			currentLanguage = Locale.getDefault().getLanguage();
		}
		return currentLanguage;
	}
}
