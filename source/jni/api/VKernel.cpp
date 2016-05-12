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

NV_USING_NAMESPACE

// Valid for the thread that called ovr_EnterVrMode
static JNIEnv	*				Jni;
static  VFrameSmooth* frameSmooth = NULL;
static jobject  ActivityObject = NULL;

/*
 * This interacts with the VrLib java class to deal with Android platform issues.
 */

// This is public for any user.
JavaVM	* VrLibJavaVM;

// This needs to be looked up by a thread called directly from java,
// not a native pthread.
static jclass	VrLibClass = NULL;

static jmethodID getPowerLevelStateID = NULL;
static jmethodID setActivityWindowFullscreenID = NULL;
static jmethodID notifyMountHandledID = NULL;

static int BuildVersionSDK = 19;		// default supported version for vrlib is KitKat 19

static int windowSurfaceWidth = 2560;	// default to Note4 resolution
static int windowSurfaceHeight = 1440;	// default to Note4 resolution

VLockless<bool> HeadsetPluggedState;
VLockless<bool> PowerLevelStateThrottled;
VLockless<bool> PowerLevelStateMinimum;

extern "C"
{
// The JNIEXPORT macro prevents the functions from ever being stripped out of the library.

void Java_com_vrseen_nervgear_VrLib_nativeVsync( JNIEnv *jni, jclass clazz, jlong frameTimeNanos );
void Java_com_vrseen_nervgear_VrLib_nativeVolumeEvent(JNIEnv *jni, jclass clazz, jint volume);

JNIEXPORT void Java_com_vrseen_nervgear_VrLib_nativeHeadsetEvent(JNIEnv *jni, jclass clazz, jint state)
{
    vInfo("nativeHeadsetEvent(" << state << ")");
    HeadsetPluggedState.setState( ( state == 1 ) );
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

    VrLibClass = JniUtils::GetGlobalClassReference( jni, "com/vrseen/nervgear/VrLib" );

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
                    { VrLibClass, 				{ "nativeVolumeEvent", "(I)V",(void*)Java_com_vrseen_nervgear_VrLib_nativeVolumeEvent } },
                    { VrLibClass, 				{ "nativeHeadsetEvent", "(I)V",(void*)Java_com_vrseen_nervgear_VrLib_nativeHeadsetEvent } },
                    { VrLibClass, 				{ "nativeVsync", "(J)V",(void*)Java_com_vrseen_nervgear_VrLib_nativeVsync } },
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

    // Set the fixed cpu and gpu clock levels
    const jmethodID setSystemPerformanceId = JniUtils::GetStaticMethodID(VrJni, vrActivityClass, "setSystemPerformanceStatic", "(Landroid/app/Activity;II)[I");
    jintArray jintClocks = (jintArray) VrJni->CallStaticObjectMethod(vrActivityClass, setSystemPerformanceId, ActivityObject, cpuLevel, gpuLevel);

    vAssert(VrJni->GetArrayLength(jintClocks) == 4);		//  {CPU CLOCK, GPU CLOCK, POWERSAVE CPU CLOCK, POWERSAVE GPU CLOCK}
}

class VKernelModule : public VModule
{
public:
    void onPause() override
    {
        VKernel::instance()->exit();
    }

    void onResume() override
    {
        VKernel::instance()->run();
    }
};
NV_ADD_MODULE(VKernelModule)

VKernel *VKernel::instance()
{
    static VKernel kernel;
    return &kernel;
}

VKernel::VKernel()
    : isRunning(false)
{
    asyncSmooth = true;
    msaa = 0;

    m_smoothOptions =0;
    const VR4Matrixf tanAngleMatrix = VR4Matrixf::TanAngleMatrixFromFov( 90.0f );
    memset( &m_texId, 0, sizeof( m_texId ) );
    memset( &m_pose, 0, sizeof( m_pose ) );
    memset( &m_planarTexId, 0, sizeof( m_planarTexId ) );
    memset( &m_texMatrix, 0, sizeof( m_texMatrix ) );
    memset( &m_externalVelocity, 0, sizeof( m_externalVelocity ) );
   for ( int eye = 0; eye < 2; eye++ )
   {
       for ( int i = 0; i < 3; i++ )
       {
           m_texMatrix[eye][i] = tanAngleMatrix;
           m_pose[eye][i].w = 1.0f;
       }
   }

   m_externalVelocity.M[0][0] = 1.0f;
   m_externalVelocity.M[1][1] = 1.0f;
   m_externalVelocity.M[2][2] = 1.0f;
   m_externalVelocity.M[3][3] = 1.0f;
   m_minimumVsyncs = 1;
   m_preScheduleSeconds = 0.014f;
   m_smoothProgram = VK_DEFAULT;
   m_programParms[0] =0;
   m_programParms[1] =0;
   m_programParms[2] =0;
   m_programParms[3] =0;
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

    // Update the dimensions in pixels directly from the window
    device->widthbyPixels = windowSurfaceWidth;
    device->heightbyPixels = windowSurfaceHeight;

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


void VKernel::run()
{
    if(isRunning) return;

    vInfo("---------- VKernel run ----------");
#if defined( OVR_BUILD_DEBUG )
    char const * buildConfig = "DEBUG";
#else
    char const * buildConfig = "RELEASE";
#endif
    ActivityObject = vApp->javaObject();
    // This returns the existing jni if the caller has already created
    // one, or creates a new one.
    const jint rtn = VrLibJavaVM->AttachCurrentThread( &Jni, 0 );
    if ( rtn != JNI_OK )
    {
        vFatal("AttachCurrentThread returned" << rtn);
    }

    // log the application name, version, activity, build, model, etc.
    jmethodID logApplicationNameMethodId = JniUtils::GetStaticMethodID( Jni, VrLibClass, "logApplicationName", "(Landroid/app/Activity;)V" );
    Jni->CallStaticVoidMethod( VrLibClass, logApplicationNameMethodId, ActivityObject );

    jmethodID logApplicationVersionId = JniUtils::GetStaticMethodID( Jni, VrLibClass, "logApplicationVersion", "(Landroid/app/Activity;)V" );
    Jni->CallStaticVoidMethod( VrLibClass, logApplicationVersionId, ActivityObject );

    jmethodID logApplicationVrType = JniUtils::GetStaticMethodID( Jni, VrLibClass, "logApplicationVrType", "(Landroid/app/Activity;)V" );
    Jni->CallStaticVoidMethod( VrLibClass, logApplicationVrType, ActivityObject );

    VString currentClassName = JniUtils::GetCurrentActivityName(Jni, ActivityObject);
    vInfo("ACTIVITY =" << currentClassName);

    vInfo("BUILD =" << VOsBuild::getString(VOsBuild::Display) << buildConfig);
    vInfo("MODEL =" << VOsBuild::getString(VOsBuild::Model));

    isRunning = true;

    // Let GlUtils look up extensions

    VEglDriver::logExtensions();

    // Look up the window surface size (NOTE: This must happen before Direct Render
    // Mode is initiated and the pbuffer surface is bound).
    {
        EGLDisplay display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
        EGLSurface surface = eglGetCurrentSurface( EGL_DRAW );
        eglQuerySurface( display, surface, EGL_WIDTH, &windowSurfaceWidth );
        eglQuerySurface( display, surface, EGL_HEIGHT, &windowSurfaceHeight );
        vInfo("Window Surface Size: [" << windowSurfaceWidth << windowSurfaceHeight << "]");
    }

    // Based on sensor ID and platform, determine the HMD
    UpdateHmdInfo();

    // Start up our vsync callbacks.
    const jmethodID startVsyncId = JniUtils::GetStaticMethodID( Jni, VrLibClass,
                                                                "startVsync", "(Landroid/app/Activity;)V" );
    Jni->CallStaticVoidMethod( VrLibClass, startVsyncId, ActivityObject );

    // Register our receivers
    const jmethodID startReceiversId = JniUtils::GetStaticMethodID( Jni, VrLibClass,
                                                                    "startReceivers", "(Landroid/app/Activity;)V" );
    Jni->CallStaticVoidMethod( VrLibClass, startReceiversId, ActivityObject );

    getPowerLevelStateID = JniUtils::GetStaticMethodID( Jni, VrLibClass, "getPowerLevelState", "(Landroid/app/Activity;)I" );
    setActivityWindowFullscreenID = JniUtils::GetStaticMethodID( Jni, VrLibClass, "setActivityWindowFullscreen", "(Landroid/app/Activity;)V" );
    notifyMountHandledID = JniUtils::GetStaticMethodID( Jni, VrLibClass, "notifyMountHandled", "(Landroid/app/Activity;)V" );

    // get external storage directory
    const jmethodID getExternalStorageDirectoryMethodId = JniUtils::GetStaticMethodID( Jni, VrLibClass, "getExternalStorageDirectory", "()Ljava/lang/String;" );
    jstring externalStorageDirectoryString = (jstring)Jni->CallStaticObjectMethod( VrLibClass, getExternalStorageDirectoryMethodId );
    Jni->DeleteLocalRef(externalStorageDirectoryString);

    //TODO: CPU Level and GPU Level should be read from a config file
    SetVrSystemPerformance(Jni, VrLibClass, 2, 2);

    if ( Jni->ExceptionOccurred() )
    {
        Jni->ExceptionClear();
        vInfo("Cleared JNI exception");
    }

    frameSmooth = new VFrameSmooth(asyncSmooth, vApp->vrParms().wantSingleBuffer);

    jmethodID setSchedFifoId = JniUtils::GetStaticMethodID(Jni, VrLibClass, "setSchedFifoStatic", "(Landroid/app/Activity;II)I");
    Jni->CallStaticIntMethod(VrLibClass, setSchedFifoId, ActivityObject, gettid(), 1);
    Jni->CallStaticIntMethod(VrLibClass, setSchedFifoId, ActivityObject, frameSmooth->threadId(), 3);

    if ( setActivityWindowFullscreenID != NULL)
    {
        Jni->CallStaticVoidMethod( VrLibClass, setActivityWindowFullscreenID, ActivityObject );
    }
}

void VKernel::exit()
{
    vInfo("---------- VKernel Exit ----------");

    if (!isRunning)
    {
        vWarn("Skipping ovr_LeaveVrMode: ovr already Destroyed");
        return;
    }

    // log the application name, version, activity, build, model, etc.
    jmethodID logApplicationNameMethodId = JniUtils::GetStaticMethodID( Jni, VrLibClass, "logApplicationName", "(Landroid/app/Activity;)V" );
    Jni->CallStaticVoidMethod( VrLibClass, logApplicationNameMethodId, ActivityObject );

    jmethodID logApplicationVersionId = JniUtils::GetStaticMethodID( Jni, VrLibClass, "logApplicationVersion", "(Landroid/app/Activity;)V" );
    Jni->CallStaticVoidMethod( VrLibClass, logApplicationVersionId, ActivityObject );

    VString currentClassName = JniUtils::GetCurrentActivityName(Jni, ActivityObject);
    vInfo("ACTIVITY =" << currentClassName);

    delete frameSmooth;
    frameSmooth = NULL;
    isRunning  = false;

    getPowerLevelStateID = NULL;

    // Stop our vsync callbacks.
    const jmethodID stopVsyncId = JniUtils::GetStaticMethodID( Jni, VrLibClass,
                                                               "stopVsync", "(Landroid/app/Activity;)V" );
    Jni->CallStaticVoidMethod( VrLibClass, stopVsyncId, ActivityObject );

    // Unregister our receivers
    const jmethodID stopReceiversId = JniUtils::GetStaticMethodID( Jni, VrLibClass,
                                                                   "stopReceivers", "(Landroid/app/Activity;)V" );
    Jni->CallStaticVoidMethod( VrLibClass, stopReceiversId, ActivityObject );
}

void VKernel::destroy(eExitType exitType)
{
    if ( exitType == EXIT_TYPE_FINISH )
    {
        exit();

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
        exit();

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
        exit();
        vInfo("Calling exitType EXIT_TYPE_EXIT");
    }
}



void VKernel::setSmoothEyeTexture(uint texID, ushort eye, ushort layer)
{
    m_texId[eye][layer] = texID;
}

void VKernel::setTexMatrix(const VR4Matrixf &mtexMatrix, ushort eye,ushort layer)
{
    m_texMatrix[eye][layer] =  mtexMatrix;
}

void VKernel::setSmoothPose(const VRotationState &pose, ushort eye, ushort layer)
{
    m_pose[eye][layer] = pose;
}

void VKernel::setpTex(uint *mpTexId, ushort eye, ushort layer)
{
    m_planarTexId[eye][layer][0] = mpTexId[0];
    m_planarTexId[eye][layer][1] = mpTexId[1];
    m_planarTexId[eye][layer][2] = mpTexId[2];
}

void VKernel::setSmoothOption(int option)
{
    m_smoothOptions = option;
}

void VKernel::setMinimumVsncs( int vsnc)
{
    m_minimumVsyncs = vsnc;
}

void VKernel::setExternalVelocity(const VR4Matrixf &extV)
{
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            m_externalVelocity.M[i][j] = extV.M[i][j];
        }
    }
}

void VKernel::setPreScheduleSeconds(float pres)
{
    m_preScheduleSeconds = pres;
}

void VKernel::setSmoothProgram(ushort program)
{
    m_smoothProgram = program;
}

void VKernel::setProgramParms(float proParms[4])
{
    m_programParms[0] = proParms[0];
    m_programParms[1] = proParms[1];
    m_programParms[2] = proParms[2];
    m_programParms[3] = proParms[3];
}

void VKernel::syncSmoothParms()
{
    if (frameSmooth == nullptr) {
        return;
    }
    frameSmooth->setSmoothProgram(m_smoothProgram);
    frameSmooth->setMinimumVsncs(m_minimumVsyncs);
    frameSmooth->setProgramParms(m_programParms);
    frameSmooth->setExternalVelocity(m_externalVelocity);
    frameSmooth->setPreScheduleSeconds(m_preScheduleSeconds);

    for (ushort eye = 0; eye < 2; eye++) {
        for (ushort i = 0; i < 3; i++) {
            frameSmooth->setSmoothEyeTexture(m_texId[eye][i],eye,i) ;
            frameSmooth->setTexMatrix(m_texMatrix[eye][i],eye,i) ;
            frameSmooth->setpTex(m_planarTexId[eye][i],eye,i) ;
            frameSmooth->setSmoothPose(m_pose[eye][i], eye, i);
        }
    }
    frameSmooth->setSmoothOption(m_smoothOptions);
}

void VKernel::doSmooth()
{
    if(frameSmooth == nullptr || !isRunning) {
        return;
    }

    syncSmoothParms();

    //ovrTimeWarpParms   parms = InitSmoothParms();
    //parms.WarpOptions = SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER | SWAP_OPTION_FLUSH | SWAP_OPTION_DEFAULT_IMAGES;

    //frameSmooth->doSmooth();
}

void VKernel::InitTimeWarpParms()
{
    m_smoothOptions = 0;
    const VR4Matrixf tanAngleMatrix = VR4Matrixf::TanAngleMatrixFromFov( 90.0f );
    memset(&m_texId, 0, sizeof(m_texId));
    memset(&m_pose, 0, sizeof(m_pose));
    memset(&m_planarTexId, 0, sizeof(m_planarTexId));
    memset(&m_texMatrix, 0, sizeof(m_texMatrix));
    memset(&m_externalVelocity, 0, sizeof(m_externalVelocity ) );
    for (int eye = 0; eye < 2; eye++) {
        for(int i = 0; i < 3; i++) {
            m_texMatrix[eye][i] = tanAngleMatrix;
            m_pose[eye][i].w = 1.0f;
        }
    }

    m_externalVelocity.M[0][0] = 1.0f;
    m_externalVelocity.M[1][1] = 1.0f;
    m_externalVelocity.M[2][2] = 1.0f;
    m_externalVelocity.M[3][3] = 1.0f;
    m_minimumVsyncs = 1;
    m_preScheduleSeconds = 0.014f;
    m_smoothProgram = VK_DEFAULT;
    m_programParms[0] = 0;
    m_programParms[1] = 0;
    m_programParms[2] = 0;
    m_programParms[3] = 0;
}

int VKernel::getBuildVersion()
{
    return BuildVersionSDK;
}
