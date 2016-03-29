/************************************************************************************

Filename    :   ConsoleReceiver.java
Content     :   A broadcast receiver that allows adb to send commands to an application.
Created     :   11/21/2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/
package com.vrseen.nervgear;

import android.app.Activity;
import android.util.Log;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;

//==============================================================
// ConsoleReceiver
//
// To send a "console" command to the app:
// adb shell am -broadcast oculus.console -es cmd <command text>
// where <command text> is replaced with a quoted string:
// adb shell am -broadcast oculus.console -es cmd "print QA Event!"
//==============================================================
public class ConsoleReceiver extends BroadcastReceiver {
	public static final String TAG = "OvrConsole";

	public static String CONSOLE_INTENT = "oculus.console";
	public static String CONSOLE_STRING_EXTRA = "cmd";
	
	public static native void nativeConsoleCommand(String command);

	static ConsoleReceiver receiver = new ConsoleReceiver();
	static boolean registeredReceiver = false;
	static Activity activity = null;

	public static void startReceiver( Activity act )
	{
		activity = act;
		if ( !registeredReceiver )
		{
			Log.d( TAG, "!!#######!! Registering console receiver" );

			IntentFilter filter = new IntentFilter();
			filter.addAction( CONSOLE_INTENT );
			act.registerReceiver( receiver, filter );
			registeredReceiver = true;
		}
		else
		{
			Log.d( TAG, "!!!!!!!!!!! Already registered console receiver!" );
		}
	}
	public static void stopReceiver( Activity act )
	{
		if ( registeredReceiver )
		{
			Log.d( TAG, "Unregistering console receiver" );
			act.unregisterReceiver( receiver );
			registeredReceiver = false;
		}
	}

 	@Override
	public void onReceive(Context context, final Intent intent) {
		Log.d( TAG, "!@#!@ConsoleReceiver action:" + intent );
		if (intent.getAction().equals( CONSOLE_INTENT ))
		{
			nativeConsoleCommand(intent.getStringExtra( CONSOLE_STRING_EXTRA ) );
		}
	}

}