package nervgear;

import android.os.Bundle;
import android.util.Log;
import android.content.Intent;
import com.vrseen.nervgear.VrActivity;
import com.vrseen.nervgear.VrLib;

public class MainActivity extends VrActivity {
	public static final String TAG = "VrTemplate";

	/** Load jni .so on initialization */
	static {
		Log.d(TAG, "LoadLibrary");
		System.loadLibrary("vrapp");
	}

    public static native void nativeSetAppInterface( VrActivity act, String fromPackageNameString, String commandString, String uriString );

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

		Intent intent = getIntent();
		String commandString = VrLib.getCommandStringFromIntent( intent );
		String fromPackageNameString = VrLib.getPackageStringFromIntent( intent );
		String uriString = VrLib.getUriStringFromIntent( intent );

		nativeSetAppInterface( this, fromPackageNameString, commandString, uriString );
    }
}
