package com.vrseen;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.util.Log;
import android.view.Window;
import android.view.WindowManager;

/**
 * Created by FX on 2016/7/8 16:06.
 * 描述:
 */


public class ScreenBrightnessTool
{

    public static final int ACTIVITY_BRIGHTNESS_AUTOMATIC = -1;

    public static final int SCREEN_BRIGHTNESS_MODE_AUTOMATIC = Settings.System.SCREEN_BRIGHTNESS_MODE_AUTOMATIC;

    public static final int SCREEN_BRIGHTNESS_MODE_MANUAL = Settings.System.SCREEN_BRIGHTNESS_MODE_MANUAL;

    public static final int SCREEN_BRIGHTNESS_DEFAULT = 75;

    public static final int MAX_BRIGHTNESS = 100;

    public static final int MIN_BRIGHTNESS = 0;

    public static final int mMaxBrighrness = 255;
    private static final int mMinBrighrness = 1;

    private static final String _TAG = ScreenBrightnessTool.class.getName();

    // 当前系统调节模式
    private  boolean sysAutomaticMode;
    // 当前系统亮度值
    private int sysBrightness;

    private Context context;

    private static ScreenBrightnessTool _instance = null;

    private ScreenBrightnessTool()
    {
    }

    private static boolean init(Activity activity)
    {
        if(_instance == null)
            _instance = new ScreenBrightnessTool();

        return _instance.Builder(activity.getApplicationContext());
    }

    private  boolean Builder(Context context)
    {
        try
        {
            this.context = context;
            // 获取当前系统调节模式
            this.sysAutomaticMode = Settings.System.getInt(
                    context.getContentResolver(),
                    Settings.System.SCREEN_BRIGHTNESS_MODE) == SCREEN_BRIGHTNESS_MODE_AUTOMATIC;
            if(!sysAutomaticMode) {
                // 获取当前系统亮度值
                //Android中并未提供处于“自动亮度”模式下的亮度值接口。
                // 下面获取系统亮度值接口实际上都是指“手动亮度”模式下的亮度值。
                sysBrightness = Settings.System.getInt(context.getContentResolver(),
                        Settings.System.SCREEN_BRIGHTNESS);
            }else{
                sysBrightness = 50;
            }
        }
        catch (SettingNotFoundException e)
        {
            return false;
        }

        return  true;
    }

    public boolean getSystemAutomaticMode()
    {
        return sysAutomaticMode;
    }

    public int getSystemBrightness()
    {
        return sysBrightness;
    }

    public void setMode(int mode)
    {
        if (mode != SCREEN_BRIGHTNESS_MODE_AUTOMATIC
                && mode != SCREEN_BRIGHTNESS_MODE_MANUAL)
            return;

        //sysAutomaticMode = mode == SCREEN_BRIGHTNESS_MODE_AUTOMATIC;
        Settings.System.putInt(context.getContentResolver(),
                Settings.System.SCREEN_BRIGHTNESS_MODE, mode);
    }

    public void setBrightness(int brightness)
    {
        int mid = mMaxBrighrness - mMinBrighrness;
        int bri = (int) (mMinBrighrness + mid * ((float) brightness)
                / MAX_BRIGHTNESS);


        Log.e("setBrightness",bri+"");
        ContentResolver resolver = context.getContentResolver();
        Settings.System
                .putInt(resolver, Settings.System.SCREEN_BRIGHTNESS, bri);
    }

    public static void brightnessPreview(Activity activity, float brightness)
    {
        Window window = activity.getWindow();
        WindowManager.LayoutParams lp = window.getAttributes();
        lp.screenBrightness = brightness;
        window.setAttributes(lp);
    }

    public static void brightnessPreviewFromPercent(Activity activity,
                                                    float percent)
    {
        float brightness = percent + (1.0f - percent)
                * (((float) mMinBrighrness) / mMaxBrighrness);
        brightnessPreview(activity, brightness);
    }

    public static void enterVrMode(Activity activity)
    {
        if( !init(activity))
        {
            Log.e(_TAG," ScreenBrightnessTool init failed!");
            return;
        }

        _instance.setMode(SCREEN_BRIGHTNESS_MODE_MANUAL);
    }

    public static void exitVrMode()
    {
        if( null == _instance)
        {
            Log.e(_TAG,"ScreenBrightnessTool is null!");
            return;
        }
        _instance.setMode(_instance.sysAutomaticMode ? SCREEN_BRIGHTNESS_MODE_AUTOMATIC : SCREEN_BRIGHTNESS_MODE_MANUAL);
        _instance.setBrightness(_instance.sysBrightness);

    }

    public static void destory()
    {
        _instance = null;
    }
}
