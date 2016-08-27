#include "VDevice.h"
#include "VFrameSmooth.h"

#include "VKernel.h"
#include "App.h"

#include <unistd.h>						// gettid, usleep, etc
#include <jni.h>
#include <math.h>

#include "VLog.h"
#include "VEglDriver.h"
#include "VString.h"
#include "VLockless.h"
#include "VModule.h"

#include "android/JniUtils.h"
#include "android/VOsBuild.h"

NV_NAMESPACE_BEGIN

// Valid for the thread that called ovr_EnterVrMode
static JNIEnv	*				Jni;
static  VFrameSmooth* frameSmooth = NULL;
static jobject  ActivityObject = NULL;

/*
 * This interacts with the VrLib java class to deal with Android platform issues.
 */

// This is public for any user.
JavaVM *VrLibJavaVM = NULL;

// This needs to be looked up by a thread called directly from java,
// not a native pthread.
jclass VrLibClass = NULL;

static jmethodID getPowerLevelStateID = NULL;
static jmethodID notifyMountHandledID = NULL;

static int BuildVersionSDK = 19;		// default supported version for vrlib is KitKat 19

//static int windowSurfaceWidth = 2560;	// default to Note4 resolution
//static int windowSurfaceHeight = 1440;	// default to Note4 resolution

VLockless<bool> HeadsetPluggedState;
VLockless<bool> PowerLevelStateThrottled;
VLockless<bool> PowerLevelStateMinimum;

extern "C"
{
// The JNIEXPORT macro prevents the functions from ever being stripped out of the library.

void Java_com_vrseen_VrLib_nativeVsync( JNIEnv *jni, jclass clazz, jlong frameTimeNanos );
void Java_com_vrseen_VrLib_nativeVolumeEvent(JNIEnv *jni, jclass clazz, jint volume);

JNIEXPORT void Java_com_vrseen_VrLib_nativeHeadsetEvent(JNIEnv *, jclass, jint state)
{
    vInfo("nativeHeadsetEvent(" << state << ")");
    HeadsetPluggedState.setState( ( state == 1 ) );
}

JNIEXPORT void Java_com_vrseen_VrLib_nativeVolumeEvent(JNIEnv *, jclass, jint volume)
{
    NV_UNUSED(volume);
}

} // extern "C"

// This must be called by a function called directly from a java thread,
// preferably at JNI_OnLoad().  It will fail if called from a pthread created
// in native code, or from a NativeActivity due to the class-lookup issue:
//
// http://developer.android.com/training/articles/perf-jni.html#faq_FindClass
//
// This should not start any threads or consume any significant amount of
// resources, so hybrid apps aren't penalizing their normal mode of operation
// by supporting VR.
void ovr_OnLoad(JavaVM * JavaVm_, JNIEnv *jni)
{
    vInfo("ovr_OnLoad()");

    if ( JavaVm_ == NULL )
    {
        vFatal("JavaVm == NULL");
    }
    if ( VrLibJavaVM != NULL )
    {
        // Should we silently return instead?
        vFatal("ovr_OnLoad() called twice");
    }

    VrLibJavaVM = JavaVm_;

    VrLibClass = JniUtils::GetGlobalClassReference( jni, "com/vrseen/VrLib" );

    // Get the BuildVersion SDK
    jclass versionClass = jni->FindClass( "android/os/Build$VERSION" );
    if ( versionClass != 0 )
    {
        jfieldID sdkIntFieldID = jni->GetStaticFieldID( versionClass, "SDK_INT", "I" );
        if ( sdkIntFieldID != 0 )
        {
            BuildVersionSDK = jni->GetStaticIntField( versionClass, sdkIntFieldID );
            vInfo("BuildVersionSDK" << BuildVersionSDK);
        }
        jni->DeleteLocalRef( versionClass );
    }

    // Explicitly register our functions.
    // Without this, the automatic name lookup fails in some projects
    // where we are loaded as a secondary .so.
    struct
    {
        jclass			Clazz;
        JNINativeMethod	Jnim;
    } gMethods[] =
            {
                    { VrLibClass, 				{ "nativeVolumeEvent", "(I)V",(void*)Java_com_vrseen_VrLib_nativeVolumeEvent } },
                    { VrLibClass, 				{ "nativeHeadsetEvent", "(I)V",(void*)Java_com_vrseen_VrLib_nativeHeadsetEvent } },
                    { VrLibClass, 				{ "nativeVsync", "(J)V",(void*)Java_com_vrseen_VrLib_nativeVsync } },
            };
    const int count = sizeof( gMethods ) / sizeof( gMethods[0] );

    // Register one at a time, so we can issue a good error message.
    for ( int i = 0; i < count; i++ )
    {
        if ( JNI_OK != jni->RegisterNatives( gMethods[i].Clazz, &gMethods[i].Jnim, 1 ) )
        {
            vFatal("RegisterNatives failed on" << gMethods[i].Jnim.name);
        }
    }
}
NV_REGISTER_JNI_LOADER(ovr_OnLoad)

enum {
    LEVEL_GPU_MIN = 0,
    LEVEL_GPU_MAX,
    LEVEL_CPU_MIN,
    LEVEL_CPU_MAX
};

static void SetVrSystemPerformance(JNIEnv * VrJni, jclass vrActivityClass, int cpuLevel, int gpuLevel)
{
    // Clear any previous exceptions.
    // NOTE: This can be removed once security exception handling is moved to
    // Java IF.
    if (VrJni->ExceptionOccurred()) {
        VrJni->ExceptionClear();
        vWarn("SetVrSystemPerformance: Enter: JNI Exception occurred");
    }

    vInfo("SetVrSystemPerformance:" << cpuLevel << gpuLevel);

    // Get the available clock levels for the device.
    const jmethodID getAvailableClockLevelsId = JniUtils::GetStaticMethodID(VrJni, vrActivityClass, "getAvailableFreqLevels", "(Landroid/app/Activity;)[I");
    if(getAvailableClockLevelsId)
    {
        jintArray jintLevels = (jintArray) VrJni->CallStaticObjectMethod(vrActivityClass, getAvailableClockLevelsId, ActivityObject);

        // Move security exception detection to the java IF.
        // Catch Permission denied
        if ( VrJni->ExceptionOccurred() ) {
            VrJni->ExceptionClear();
            vInfo("SetVrSystemPerformance: JNI Exception occurred, returning");
            return;
        }

        vAssert(VrJni->GetArrayLength(jintLevels) == 4);		// {GPU MIN, GPU MAX, CPU MIN, CPU MAX}

        jint * levels = VrJni->GetIntArrayElements(jintLevels, NULL);
        if (levels != NULL) {
            // Verify levels are within appropriate range for the device
            if (cpuLevel < levels[LEVEL_CPU_MIN]) {
                cpuLevel = levels[LEVEL_CPU_MIN];
            } else if (cpuLevel > levels[LEVEL_CPU_MAX]) {
                cpuLevel = levels[LEVEL_CPU_MAX];
            }
            if (gpuLevel < levels[LEVEL_GPU_MIN]) {
                gpuLevel = levels[LEVEL_GPU_MIN];
            } else if (gpuLevel > levels[LEVEL_GPU_MAX]) {
                gpuLevel = levels[LEVEL_GPU_MAX];
            }

            VrJni->ReleaseIntArrayElements(jintLevels, levels, 0);
        }
        VrJni->DeleteLocalRef(jintLevels);
    }
    // Set the fixed cpu and gpu clock levels
    const jmethodID setSystemPerformanceId = JniUtils::GetStaticMethodID(VrJni, vrActivityClass, "setSystemPerformanceStatic", "(Landroid/app/Activity;II)[I");
    if(setSystemPerformanceId)
    {
        jintArray jintClocks = (jintArray) VrJni->CallStaticObjectMethod(vrActivityClass, setSystemPerformanceId, ActivityObject, cpuLevel, gpuLevel);

        vAssert(VrJni->GetArrayLength(jintClocks) == 4);		//  {CPU CLOCK, GPU CLOCK, POWERSAVE CPU CLOCK, POWERSAVE GPU CLOCK}
    }

}

static void ReleaseVrSystemPerformance( JNIEnv * VrJni, jclass vrActivityClass, jobject activityObject )
{
    // Clear any previous exceptions.
    // NOTE: This can be removed once security exception handling is moved to Java IF.
    if ( VrJni->ExceptionOccurred() )
    {
        VrJni->ExceptionClear();
        vWarn( "ReleaseVrSystemPerformance: Enter: JNI Exception occurred" );
    }

    vInfo( "ReleaseVrSystemPerformance" );

    // Release the fixed cpu and gpu clock levels
    const jmethodID releaseSystemPerformanceId = JniUtils::GetStaticMethodID( VrJni, vrActivityClass,
                                                                        "releaseSystemPerformanceStatic", "(Landroid/app/Activity;)V" );
    VrJni->CallStaticVoidMethod( vrActivityClass, releaseSystemPerformanceId, activityObject );
    if ( VrJni->ExceptionOccurred() )
    {
        VrJni->ExceptionClear();
        vWarn( "ReleaseVrSystemPerformance: Enter: JNI Exception occurred" );
    }
}

static void SetSchedFifo( JNIEnv* VrJni, jclass vrActivityClass, jobject activityObject, int tid, int priority )
{
    const jmethodID setSchedFifoMethodId = JniUtils::GetStaticMethodID( VrJni, VrLibClass, "setSchedFifoStatic", "(Landroid/app/Activity;II)I" );
    const int r = VrJni->CallStaticIntMethod( vrActivityClass, setSchedFifoMethodId, activityObject, tid, priority );
    if ( r >= 0 )
    {
        vInfo( "SetSchedFifo tid = "<< tid );
        vInfo( "SetSchedFifo priority = "<< priority );
    }
    else
    {
        if(r == -1)
        {
            vWarn( "SetSchedFifo VRManager failed "<< tid );
        }else if(r == -2)
        {
            vWarn( "SetSchedFifo API not found "<< tid );
        }else if(r == -3)
        {
            vWarn("SetSchedFifo security exception");
        }

    }

    if ( priority != 0 )
    {
        switch ( r )
        {
            case -1: vWarn( "VRManager failed to set thread priority." ); break;
            case -2: vWarn( "Thread priority API does not exist. Update your device binary." ); break;
            case -3: vWarn( "Thread priority security exception. Make sure the APK is signed." ); break;
        }
    }

    if ( VrJni->ExceptionOccurred() )
    {
        VrJni->ExceptionClear();
        vWarn( "ReleaseVrSystemPerformance: Enter: JNI Exception occurred" );
    }
}

VKernel *VKernel::instance()
{
    static VKernel kernel;
    return &kernel;
}

VKernel::VKernel()
    : isRunning(false)
{
    msaa = 0;
}

void UpdateHmdInfo()
{
    VDevice *device = VDevice::instance();

    // Only use the Android info if we haven't explicitly set the screenWidth / height,
    // because they are reported wrong on the note.
    if(!device->widthbyMeters)
    {
        jmethodID getDisplayWidth = Jni->GetStaticMethodID( VrLibClass, "getDisplayWidth", "(Landroid/app/Activity;)F" );
        if ( !getDisplayWidth )
        {
            vFatal("couldn't get getDisplayWidth");
        }
        device->widthbyMeters = Jni->CallStaticFloatMethod(VrLibClass, getDisplayWidth, ActivityObject);

        jmethodID getDisplayHeight = Jni->GetStaticMethodID( VrLibClass, "getDisplayHeight", "(Landroid/app/Activity;)F" );
        if ( !getDisplayHeight )
        {
            vFatal("couldn't get getDisplayHeight");
        }
        device->heightbyMeters = Jni->CallStaticFloatMethod( VrLibClass, getDisplayHeight, ActivityObject );
    }

    vInfo("hmdInfo.lensSeparation =" << device->lensDistance);
    vInfo("hmdInfo.widthMeters =" << device->widthbyMeters);
    vInfo("hmdInfo.heightMeters =" << device->heightbyMeters);
    vInfo("hmdInfo.widthPixels =" << device->widthbyPixels);
    vInfo("hmdInfo.heightPixels =" << device->heightbyPixels);
    vInfo("hmdInfo.eyeTextureResolution[0] =" << device->eyeDisplayResolution[0]);
    vInfo("hmdInfo.eyeTextureResolution[1] =" << device->eyeDisplayResolution[1]);
    vInfo("hmdInfo.eyeTextureFov[0] =" << device->eyeDisplayFov[0]);
    vInfo("hmdInfo.eyeTextureFov[1] =" << device->eyeDisplayFov[1]);
}

static void vrEnableVRMode(JNIEnv * VrJni,jclass vrActivityClass, int mode )
{
    vInfo("Vrmode is set to " << mode);
    if(!(VOsBuild::getString(VOsBuild::Model) == "ZTE A2017"))
        return;

    // Clear any previous exceptions.
    // NOTE: This can be removed once security exception handling is moved to Java IF.
    if ( VrJni->ExceptionOccurred() )
    {
        VrJni->ExceptionClear();
        vInfo( "vrEnableVRMode: Enter: JNI Exception" );
    }

    vInfo( "********vrEnableVRMode***********" );
    const jmethodID vrEnableVRModeStaticId =
    JniUtils::GetStaticMethodID( VrJni, vrActivityClass, "vrEnableVRModeStatic", "(Landroid/app/Activity;I)I" );

    VrJni->CallStaticIntMethod( vrActivityClass, vrEnableVRModeStaticId, vApp->javaObject(), mode );

    if ( VrJni->ExceptionOccurred() )
    {
        VrJni->ExceptionClear();
        vInfo( "vrEnableVRMode: Enter: JNI Exception" );
    }
}

void VKernel::enterVrMode()
{
    if(isRunning) return;

    vInfo("---------- VKernel run ----------");
    ActivityObject = vApp->javaObject();
    // This returns the existing jni if the caller has already created
    // one, or creates a new one.
    const jint rtn = VrLibJavaVM->AttachCurrentThread( &Jni, 0 );
    if ( rtn != JNI_OK )
    {
        vFatal("AttachCurrentThread returned" << rtn);
    }

    vInfo("MODEL =" << VOsBuild::getString(VOsBuild::Model));

    isRunning = true;

    // Based on sensor ID and platform, determine the HMD
    UpdateHmdInfo();

    // Start up our vsync callbacks.
    const jmethodID startVsyncId = JniUtils::GetStaticMethodID( Jni, VrLibClass, "startVsync", "(Landroid/app/Activity;)V" );
    Jni->CallStaticVoidMethod( VrLibClass, startVsyncId, ActivityObject );

    // Register our receivers
    const jmethodID startReceiversId = JniUtils::GetStaticMethodID( Jni, VrLibClass, "startReceivers", "(Landroid/app/Activity;)V" );
    Jni->CallStaticVoidMethod( VrLibClass, startReceiversId, ActivityObject );

    getPowerLevelStateID = JniUtils::GetStaticMethodID( Jni, VrLibClass, "getPowerLevelState", "(Landroid/app/Activity;)I" );
    notifyMountHandledID = JniUtils::GetStaticMethodID( Jni, VrLibClass, "notifyMountHandled", "(Landroid/app/Activity;)V" );

    //TODO: CPU Level and GPU Level should be read from a config file
    SetVrSystemPerformance(Jni, VrLibClass, 2, 2);

    vrEnableVRMode( Jni, VrLibClass, 1 );

    if ( Jni->ExceptionOccurred() )
    {
        Jni->ExceptionClear();
        vInfo("Cleared JNI exception");
    }

    if(frameSmooth == NULL)
    {
        VFrameSmooth::HardwareRefreshDirection direction = (VOsBuild::getString(VOsBuild::Model) == "vivo Xplay5S") ?
                                                           VFrameSmooth::HWRD_BT : VFrameSmooth::HWRD_TB;

        frameSmooth = new VFrameSmooth(vApp->vrParms().wantSingleBuffer, direction);
    }

    // The calling thread, which is presumably the eye buffer drawing thread,
    // will be at the lowest realtime priority.
    SetSchedFifo( Jni, VrLibClass, ActivityObject, gettid(), SCHED_FIFO_PRIORITY_VRTHREAD );
//    if ( ovr->Parms.GameThreadTid )
//    {
//        SetSchedFifo( Jni, VrLibClass, ActivityObject, ovr->Parms.GameThreadTid, SCHED_FIFO_PRIORITY_VRTHREAD );
//    }
//
//    // The device manager deals with sensor updates, which will happen at 500 hz, and should
//    // preempt the main thread, but not timewarp or audio mixing.
//    SetSchedFifo( Jni, VrLibClass, ActivityObject, ovr_GetDeviceManagerThreadTid(), SCHED_FIFO_PRIORITY_DEVICEMNGR );

    // The timewarp thread, if present, will be very high.  It only uses a fraction of
    // a millisecond of CPU, so it can be even higher than audio threads.
    if ( frameSmooth->threadId() )
    {
        SetSchedFifo( Jni, VrLibClass, ActivityObject, frameSmooth->threadId(), SCHED_FIFO_PRIORITY_TIMEWARP );
    }
}

void VKernel::resume()
{
    vInfo("Resume in VKernel!");
    if(frameSmooth) {
        frameSmooth->resume();
        enterVrMode();
    }
}

void VKernel::setSurfaceRevolution(EGLSurface surface)
{
    if(frameSmooth) {
        frameSmooth->setupSurface(surface);
    }
}

void VKernel::pause()
{
    vInfo("Pause in VKernel!");
    if(frameSmooth)
    {
        leaveVrMode();
        frameSmooth->pause();
    }
}

void VKernel::leaveVrMode()
{
    vInfo("---------- VKernel Exit ----------");

    if (!isRunning)
    {
        vWarn("Skipping vkernel_LeaveVrMode: vkernel already Destroyed");
        return;
    }

    // Remove SCHED_FIFO from the remaining threads.
    SetSchedFifo( Jni, VrLibClass, ActivityObject, gettid(), SCHED_FIFO_PRIORITY_NONE );
    if (frameSmooth->threadId() )
    {
        SetSchedFifo( Jni, VrLibClass, ActivityObject, frameSmooth->threadId(), SCHED_FIFO_PRIORITY_NONE );
    }
    //SetSchedFifo( ovr, ovr_GetDeviceManagerThreadTid(), SCHED_FIFO_PRIORITY_NONE );

    //resume common system setting
    ReleaseVrSystemPerformance(Jni, VrLibClass, ActivityObject);

    vrEnableVRMode(Jni, VrLibClass, 0);

    getPowerLevelStateID = NULL;

    // Stop our vsync callbacks.
    const jmethodID stopVsyncId = JniUtils::GetStaticMethodID( Jni, VrLibClass, "stopVsync", "(Landroid/app/Activity;)V" );
    Jni->CallStaticVoidMethod( VrLibClass, stopVsyncId, ActivityObject );

    // Unregister our receivers
    const jmethodID stopReceiversId = JniUtils::GetStaticMethodID( Jni, VrLibClass, "stopReceivers", "(Landroid/app/Activity;)V" );
    Jni->CallStaticVoidMethod( VrLibClass, stopReceiversId, ActivityObject );

    isRunning = false;
}

//zx_note 2016.8.9  @叶月林修改此处代码
void VKernel::destroy(eExitType exitType)
{
    leaveVrMode();
    if ( exitType == EXIT_TYPE_FINISH )
    {
        //	const char * name = "finish";
        const char * name = "finishOnUiThread";
        const jmethodID mid = JniUtils::GetStaticMethodID( Jni, VrLibClass,
                                                           name, "(Landroid/app/Activity;)V" );

        if ( Jni->ExceptionOccurred() )
        {
            Jni->ExceptionClear();
            vInfo("Cleared JNI exception");
        }
        vInfo("Calling activity.finishOnUiThread()");
        Jni->CallStaticVoidMethod( VrLibClass, mid, *static_cast< jobject* >( &ActivityObject ) );
        vInfo("Returned from activity.finishOnUiThread()");
    }
    else if ( exitType == EXIT_TYPE_FINISH_AFFINITY )
    {
        const char * name = "finishAffinityOnUiThread";
        const jmethodID mid = JniUtils::GetStaticMethodID( Jni, VrLibClass,
                                                           name, "(Landroid/app/Activity;)V" );

        if ( Jni->ExceptionOccurred() )
        {
            Jni->ExceptionClear();
            vInfo("Cleared JNI exception");
        }
        vInfo("Calling activity.finishAffinityOnUiThread()");
        Jni->CallStaticVoidMethod( VrLibClass, mid, *static_cast< jobject* >( &ActivityObject ) );
        vInfo("Returned from activity.finishAffinityOnUiThread()");
    }
    else if ( exitType == EXIT_TYPE_EXIT )
    {
        const jmethodID destoryScreenBrightnessToolId = JniUtils::GetStaticMethodID( Jni, VrLibClass, "destoryScreenBrightnessTool", "()V" );
        Jni->CallStaticVoidMethod( VrLibClass, destoryScreenBrightnessToolId );

        delete frameSmooth;
        frameSmooth = NULL;

        vInfo("Calling exitType EXIT_TYPE_EXIT");
    }
}

VTimeWarpParms  VKernel::InitTimeWarpParms( const VWarpInit init, const unsigned int texId)
{
    const VMatrix4f tanAngleMatrix = VMatrix4f::TanAngleMatrixFromFov( 90.0f );

    VTimeWarpParms parms;
    memset( &parms, 0, sizeof( parms ) );

    for ( int eye = 0; eye < MAX_WARP_EYES; eye++ )
    {
        for ( int i = 0; i < MAX_WARP_IMAGES; i++ )
        {
            parms.Images[eye][i].TexCoordsFromTanAngles = tanAngleMatrix;
            parms.Images[eye][i].Pose.w = 1.0f;
        }
    }
    parms.ExternalVelocity.cell[0][0] = 1.0f;
    parms.ExternalVelocity.cell[1][1] = 1.0f;
    parms.ExternalVelocity.cell[2][2] = 1.0f;
    parms.ExternalVelocity.cell[3][3] = 1.0f;
    parms.MinimumVsyncs = 1;
    parms.PreScheduleSeconds = 0.014f;
    parms.WarpProgram = WP_SIMPLE;

    switch ( init )
    {
        case WARP_INIT_DEFAULT:
        {
            break;
        }
        case WARP_INIT_BLACK:
        {
            parms.WarpOptions = SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER | SWAP_OPTION_FLUSH | SWAP_OPTION_DEFAULT_IMAGES;
            parms.WarpProgram = WP_SIMPLE;
            for ( int eye = 0; eye < MAX_WARP_EYES; eye++ )
            {
                parms.Images[eye][0].TexId = 0;		// default replaced with a black texture
            }
            break;
        }
        case WARP_INIT_LOADING_ICON:
        {
            parms.WarpOptions = SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER | SWAP_OPTION_FLUSH | SWAP_OPTION_DEFAULT_IMAGES;
            parms.WarpProgram = WP_LOADING_ICON;
            parms.ProgramParms[0] = 1.0f;		// rotation in radians per second
            parms.ProgramParms[1] = 16.0f;		// icon size factor smaller than fullscreen
            for ( int eye = 0; eye < MAX_WARP_EYES; eye++ )
            {
                parms.Images[eye][0].TexId = 0;		// default replaced with a black texture
                parms.Images[eye][1].TexId = texId;	// loading icon texture
            }
            break;
        }
        case WARP_INIT_MESSAGE:
        {
            parms.WarpOptions = SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER | SWAP_OPTION_FLUSH | SWAP_OPTION_DEFAULT_IMAGES;
            parms.WarpProgram = WP_LOADING_ICON;
            parms.ProgramParms[0] = 0.0f;		// rotation in radians per second
            parms.ProgramParms[1] = 2.0f;		// message size factor smaller than fullscreen
            for ( int eye = 0; eye < MAX_WARP_EYES; eye++ )
            {
                parms.Images[eye][0].TexId = 0;		// default replaced with a black texture
                parms.Images[eye][1].TexId = texId;	// message texture
            }
            break;
        }
    }
    return parms;
}

int VKernel::getBuildVersion()
{
    return BuildVersionSDK;
}

void VKernel::doSmooth(const VTimeWarpParms * parms )
{
    if(frameSmooth==NULL||!isRunning) return;
    frameSmooth->doSmooth(*parms);
}

NV_NAMESPACE_END
