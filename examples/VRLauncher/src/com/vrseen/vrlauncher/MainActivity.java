package com.vrseen.vrlauncher;

import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Parcel;
import android.util.Log;
import com.vrseen.VrActivity;

import java.util.List;

public class MainActivity extends VrActivity {

	VRLauncher launcher = null;

	static {
		System.loadLibrary("vrlauncher");
	}

	@Override
	protected void onCreate(Bundle savedInstanceState)
	{
		super.onCreate(savedInstanceState);
		launcher = new VRLauncher(this);
		String background = Environment.getExternalStorageDirectory().getAbsolutePath() + "/VRSeen/SDK/360Photos/1.jpg";
		launcher.setBackground(background);
	}

	private void startApp(String packageName, String data) {
		PackageInfo packageinfo = null;
		try {
			packageinfo = getPackageManager().getPackageInfo(packageName, 0);
		} catch (PackageManager.NameNotFoundException e) {
			e.printStackTrace();
		}
		if (packageinfo == null) {
			return;
		}

		Intent resolveIntent = new Intent(Intent.ACTION_MAIN, null);
		resolveIntent.addCategory(Intent.CATEGORY_LAUNCHER);
		resolveIntent.setPackage(packageinfo.packageName);

		List<ResolveInfo> resolveinfoList = getPackageManager().queryIntentActivities(resolveIntent, 0);
		ResolveInfo resolveinfo = resolveinfoList.iterator().next();
		if (resolveinfo != null) {
			Intent intent = new Intent(Intent.ACTION_MAIN);
			intent.addCategory(Intent.CATEGORY_LAUNCHER);
			ComponentName cn = new ComponentName(resolveinfo.activityInfo.packageName, resolveinfo.activityInfo.name);
			intent.setComponent(cn);
			intent.setData(Uri.parse(data));
			startActivity(intent);
		}
	}
}
