#include "VDevice.h"
#include "VFrameSmooth.h"

#include "VKernel.h"
#include "App.h"

#include <unistd.h>						// gettid, usleep, etc
#include <jni.h>
#include <sstream>
#include <math.h>

#include "VLog.h"
#include "VEglDriver.h"
#include "android/JniUtils.h"
#include "android/VOsBuild.h"

#include "VString.h"			// for ReadFreq()
#include "VJson.h"			// needed for ovr_StartSystemActivity

#include "VLockless.h"
#include "VRotationSensor.h"
#include "VThread.h"

NV_USING_NAMESPACE

static VKernel* instance = NULL;
// Valid for the thread that called ovr_EnterVrMode
static JNIEnv	*				Jni;
static  VFrameSmooth* frameSmooth = NULL;
static jobject  ActivityObject = NULL;


void		ovr_OnLoad( JavaVM * JavaVm_ );
void		ovr_Init();

ovrSensorState ovr_GetSensorStateInternal( double absTime )
{
    ovrSensorState state;
    memset( &state, 0, sizeof( state ) );

    VRotationSensor *sensor = VRotationSensor::instance();

    VRotationSensor::State recordedState = sensor->state();
    state.Recorded.Orientation.w = recordedState.w;
    state.Recorded.Orientation.x = recordedState.x;
    state.Recorded.Orientation.y = recordedState.y;
    state.Recorded.Orientation.z = recordedState.z;
    state.Recorded.TimeBySeconds = recordedState.timestamp;

    VRotationSensor::State predictedState = sensor->predictState(absTime);
    state.Predicted.Orientation.w = predictedState.w;
    state.Predicted.Orientation.x = predictedState.x;
    state.Predicted.Orientation.y = predictedState.y;
    state.Predicted.Orientation.z = predictedState.z;
    state.Predicted.TimeBySeconds = absTime;

    return state;
}

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

enum eHMTDockState
{
    HMT_DOCK_NONE,			// nothing to do
    HMT_DOCK_DOCKED,		// the device is inserted into the HMT
    HMT_DOCK_UNDOCKED		// the device has been removed from the HMT
};

struct HMTDockState_t
{
    HMTDockState_t() :
            DockState( HMT_DOCK_NONE )
    {
    }

    explicit HMTDockState_t( eHMTDockState const dockState ) :
            DockState( dockState )
    {
    }

    eHMTDockState	DockState;
};

enum eHMTMountState
{
    HMT_MOUNT_NONE,			// nothing to do
    HMT_MOUNT_MOUNTED,		// the HMT has been placed on the head
    HMT_MOUNT_UNMOUNTED		// the HMT has been removed from the head
};

struct HMTMountState_t
{
    HMTMountState_t() :
            MountState( HMT_MOUNT_NONE )
    {
    }

    explicit HMTMountState_t( eHMTMountState const mountState ) :
            MountState( mountState )
    {
    }

    eHMTMountState	MountState;
};






NervGear::VLockless< bool >						HeadsetPluggedState;
NervGear::VLockless< bool >						PowerLevelStateThrottled;
NervGear::VLockless< bool >						PowerLevelStateMinimum;

extern "C"
{
// The JNIEXPORT macro prevents the functions from ever being stripped out of the library.

void Java_com_vrseen_nervgear_VrLib_nativeVsync( JNIEnv *jni, jclass clazz, jlong frameTimeNanos );
void Java_com_vrseen_nervgear_VrLib_nativeVolumeEvent(JNIEnv *jni, jclass clazz, jint volume);

JNIEXPORT jint JNI_OnLoad( JavaVM * vm, void * reserved )
{
vInfo("JNI_OnLoad");

// Lookup our classnames
ovr_OnLoad( vm );

// Start up the Oculus device manager
ovr_Init();

return JNI_VERSION_1_6;
}







JNIEXPORT void Java_com_vrseen_nervgear_VrLib_nativeHeadsetEvent(JNIEnv *jni, jclass clazz, jint state)
{
    vInfo("nativeHeadsetEvent(" << state << ")");
    HeadsetPluggedState.setState( ( state == 1 ) );
}

} // extern "C"

const char *ovr_GetVersionString()
{
    return NV_VERSION_STRING;
}



// This must be called by a function called directly from a java thread,
// preferably at JNI_OnLoad().  It will fail if called from a pthread created
// in native code, or from a NativeActivity due to the class-lookup issue:
//
// http://developer.android.com/training/articles/perf-jni.html#faq_FindClass
//
// This should not start any threads or consume any significant amount of
// resources, so hybrid apps aren't penalizing their normal mode of operation
// by supporting VR.
void ovr_OnLoad( JavaVM * JavaVm_ )
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

    JNIEnv * jni;
    bool privateEnv = false;
    if ( JNI_OK != VrLibJavaVM->GetEnv( reinterpret_cast<void**>(&jni), JNI_VERSION_1_6 ) )
    {
        vInfo("Creating temporary JNIEnv");
        // We will detach after we are done
        privateEnv = true;
        const jint rtn = VrLibJavaVM->AttachCurrentThread( &jni, 0 );
        if ( rtn != JNI_OK )
        {
            vFatal("AttachCurrentThread returned" << rtn);
        }
    }
    else
    {
        vInfo("Using caller's JNIEnv");
    }

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

    // Detach if the caller wasn't already attached
    if ( privateEnv )
    {
        vInfo("Freeing temporary JNIEnv");
        VrLibJavaVM->DetachCurrentThread();
    }
}

// A dedicated VR app will usually call this immediately after ovr_OnLoad(),
// but a hybrid app may want to defer calling it until the first headset
// plugin event to avoid starting the device manager.
void ovr_Init()
{
    vInfo("ovr_Init");

    JNIEnv * jni;
    const jint rtn = VrLibJavaVM->AttachCurrentThread( &jni, 0 );
    if ( rtn != JNI_OK )
    {
        vFatal("AttachCurrentThread returned" << rtn);
    }

    // After ovr_Initialize(), because it uses String
    VOsBuild::Init(jni);
}

VKernel* VKernel::GetInstance()
{
    if(instance==NULL)
    {
        instance = new VKernel();
    }
    return  instance;
}

VKernel::VKernel()
    : isRunning(false)
{
    asyncSmooth = true;
    msaa = 0;
    device = VDevice::instance();

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
           m_pose[eye][i].Orientation.w = 1.0f;
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
    // Only use the Android info if we haven't explicitly set the screenWidth / height,
    // because they are reported wrong on the note.
    if(!instance->device->widthbyMeters)
    {
        jmethodID getDisplayWidth = Jni->GetStaticMethodID( VrLibClass, "getDisplayWidth", "(Landroid/app/Activity;)F" );
        if ( !getDisplayWidth )
        {
            vFatal("couldn't get getDisplayWidth");
        }
        instance->device->widthbyMeters = Jni->CallStaticFloatMethod(VrLibClass, getDisplayWidth, ActivityObject);

        jmethodID getDisplayHeight = Jni->GetStaticMethodID( VrLibClass, "getDisplayHeight", "(Landroid/app/Activity;)F" );
        if ( !getDisplayHeight )
        {
            vFatal("couldn't get getDisplayHeight");
        }
        instance->device->heightbyMeters = Jni->CallStaticFloatMethod( VrLibClass, getDisplayHeight, ActivityObject );
    }

    // Update the dimensions in pixels directly from the window
    instance->device->widthbyPixels = windowSurfaceWidth;
    instance->device->heightbyPixels = windowSurfaceHeight;

    vInfo("hmdInfo.lensSeparation =" << instance->device->lensDistance);
    vInfo("hmdInfo.widthMeters =" << instance->device->widthbyMeters);
    vInfo("hmdInfo.heightMeters =" << instance->device->heightbyMeters);
    vInfo("hmdInfo.widthPixels =" << instance->device->widthbyPixels);
    vInfo("hmdInfo.heightPixels =" << instance->device->heightbyPixels);
    vInfo("hmdInfo.eyeTextureResolution[0] =" << instance->device->eyeDisplayResolution[0]);
    vInfo("hmdInfo.eyeTextureResolution[1] =" << instance->device->eyeDisplayResolution[1]);
    vInfo("hmdInfo.eyeTextureFov[0] =" << instance->device->eyeDisplayFov[0]);
    vInfo("hmdInfo.eyeTextureFov[1] =" << instance->device->eyeDisplayFov[1]);
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
    vInfo("OVR_VERSION =" << ovr_GetVersionString());

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

    if ( Jni->ExceptionOccurred() )
    {
        Jni->ExceptionClear();
        vInfo("Cleared JNI exception");
    }

    frameSmooth = new VFrameSmooth(asyncSmooth,device);

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
        delete  instance;
        instance = NULL;
    }
}



void VKernel::setSmoothEyeTexture(unsigned int texID,ushort eye,ushort layer)
{

   m_texId[eye][layer] =  texID;

}


void VKernel::setTexMatrix(VR4Matrixf	mtexMatrix,ushort eye,ushort layer)
{
  m_texMatrix[eye][layer] =  mtexMatrix;
}
void VKernel::setSmoothPose(VKpose	mpose,ushort eye,ushort layer)
{
   m_pose[eye][layer] =  mpose;
}
void VKernel::setpTex(unsigned int	*mpTexId,ushort eye,ushort layer)
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
void VKernel::setExternalVelocity(VR4Matrixf extV)

{
    for(int i=0;i<4;i++)
        for( int j=0;j<4;j++)
    {
 m_externalVelocity.M[i][j] = extV.M[i][j];
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
void VKernel::setProgramParms( float * proParms)
{
   m_programParms[0] = proParms[0];
    m_programParms[1] = proParms[1];
     m_programParms[2] = proParms[2];
      m_programParms[3] = proParms[3];

}

void VKernel::syncSmoothParms()
{
 if (frameSmooth==NULL)
     return;
 frameSmooth->setSmoothProgram(m_smoothProgram);
 frameSmooth->setMinimumVsncs(m_minimumVsyncs);
 frameSmooth->setProgramParms(m_programParms);
 frameSmooth->setExternalVelocity(m_externalVelocity);
 frameSmooth->setPreScheduleSeconds(m_preScheduleSeconds);
 for ( ushort eye = 0; eye < 2; eye++ )
 {
     for ( ushort i = 0; i < 3; i++ )
     {
         frameSmooth->setSmoothEyeTexture(m_texId[eye][i],eye,i) ;
         frameSmooth->setTexMatrix(m_texMatrix[eye][i],eye,i) ;
         frameSmooth->setpTex(m_planarTexId[eye][i],eye,i) ;
         frameSmooth->setSmoothPose(m_pose[eye][i],eye,i);

     }
 }

 frameSmooth->setSmoothOption(m_smoothOptions);

}

void VKernel::doSmooth()
{
    if(frameSmooth==NULL||!isRunning) return;
    syncSmoothParms();

    //ovrTimeWarpParms   parms = InitSmoothParms();
    //parms.WarpOptions = SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER | SWAP_OPTION_FLUSH | SWAP_OPTION_DEFAULT_IMAGES;

   // frameSmooth->doSmooth();

}

ovrSensorState VKernel::ovr_GetPredictedSensorState(double absTime )
{
    return ovr_GetSensorStateInternal( absTime );
}

/*void VKernel::doSmooth(const ovrTimeWarpParms * parms )
{
    if(frameSmooth==NULL||!isRunning) return;

    //setSmoothParms(*parms);
    syncSmoothParms();
   // frameSmooth->doSmooth();
}*/
 void VKernel::InitTimeWarpParms( )
{
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
            m_pose[eye][i].Orientation.w = 1.0f;
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
/*
void VKernel::setSmoothParms(const ovrTimeWarpParms &  parms)
{
    //ovrTimeWarpParms parms;
   // memset( &parms, 0, sizeof( parms ) );
    for(int i=0;i<2;i++)
         for (int j=0;j<3;j++)
         {
    m_images[i][j].PlanarTexId[0]  =      parms.Images[i][j].PlanarTexId[0] ;
    m_images[i][j].PlanarTexId[1]  =      parms.Images[i][j].PlanarTexId[1] ;
    m_images[i][j].PlanarTexId[2] =       parms.Images[i][j].PlanarTexId[2] ;
                    for(int m=0;m<4;m++)
                        for(int n=0;n<4;n++)
                        {
                    m_images[i][j].TexCoordsFromTanAngles.M[m][n] =parms.Images[i][j].TexCoordsFromTanAngles.M[m][n];
                        }
                      m_images[i][j].TexId=parms.Images[i][j].TexId ;
                     m_images[i][j].Pose.AngularAcceleration.x = parms.Images[i][j].Pose.AngularAcceleration.x ;
                      m_images[i][j].Pose.AngularAcceleration.y = parms.Images[i][j].Pose.AngularAcceleration.y;
                     m_images[i][j].Pose.AngularAcceleration.z = parms.Images[i][j].Pose.AngularAcceleration.z ;
            m_images[i][j].Pose.AngularVelocity.x = parms.Images[i][j].Pose.AngularVelocity.x ;
                m_images[i][j].Pose.AngularVelocity.y = parms.Images[i][j].Pose.AngularVelocity.y;
                 m_images[i][j].Pose.AngularVelocity.z = parms.Images[i][j].Pose.AngularVelocity.z;
             m_images[i][j].Pose.LinearAcceleration.x = parms.Images[i][j].Pose.LinearAcceleration.x;
             m_images[i][j].Pose.LinearAcceleration.y = parms.Images[i][j].Pose.LinearAcceleration.y;
             m_images[i][j].Pose.LinearAcceleration.z = parms.Images[i][j].Pose.LinearAcceleration.z;
              m_images[i][j].Pose.LinearVelocity.x =parms.Images[i][j].Pose.LinearVelocity.x;
               m_images[i][j].Pose.LinearVelocity.y =parms.Images[i][j].Pose.LinearVelocity.y;
                m_images[i][j].Pose.LinearVelocity.z = parms.Images[i][j].Pose.LinearVelocity.z;
              m_images[i][j].Pose.Pose.Orientation.w =  parms.Images[i][j].Pose.Pose.Orientation.w;
               m_images[i][j].Pose.Pose.Orientation.x = parms.Images[i][j].Pose.Pose.Orientation.x;
               m_images[i][j].Pose.Pose.Orientation.y = parms.Images[i][j].Pose.Pose.Orientation.y;
               m_images[i][j].Pose.Pose.Orientation.z =parms.Images[i][j].Pose.Pose.Orientation.z;


               m_images[i][j].Pose.Pose.Position.x =parms.Images[i][j].Pose.Pose.Position.x;
               m_images[i][j].Pose.Pose.Position.y =parms.Images[i][j].Pose.Pose.Position.y;
               m_images[i][j].Pose.Pose.Position.z =parms.Images[i][j].Pose.Pose.Position.z;


                m_images[i][j].Pose.TimeInSeconds =parms.Images[i][j].Pose.TimeInSeconds;




         }

    m_smoothOptions =parms.WarpOptions;
    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++)
    {
        m_externalVelocity.M[i][j] = parms.ExternalVelocity.M[i][j];

    }

   m_minimumVsyncs = parms.MinimumVsyncs ;
   m_preScheduleSeconds =parms.PreScheduleSeconds;
   m_smoothProgram = parms.WarpProgram;
    for(int i=0;i<4;i++)
    {
        m_programParms[i] = parms.ProgramParms[i];

    }

}

*/
 /*
ovrTimeWarpParms  VKernel::getSmoothParms()
{
    ovrTimeWarpParms parms;
    memset( &parms, 0, sizeof( parms ) );
    for(int i=0;i<2;i++)
         for (int j=0;j<3;j++)
         {
           parms.Images[i][j].PlanarTexId[0] = m_images[i][j].PlanarTexId[0];
                    parms.Images[i][j].PlanarTexId[1] = m_images[i][j].PlanarTexId[1];
                    parms.Images[i][j].PlanarTexId[2] = m_images[i][j].PlanarTexId[2];
                    for(int m=0;m<4;m++)
                        for(int n=0;n<4;n++)
                        {
                    parms.Images[i][j].TexCoordsFromTanAngles.M[m][n] = m_images[i][j].TexCoordsFromTanAngles.M[m][n];
                        }
                     parms.Images[i][j].TexId = m_images[i][j].TexId;
                     parms.Images[i][j].Pose.AngularAcceleration.x =m_images[i][j].Pose.AngularAcceleration.x;
                     parms.Images[i][j].Pose.AngularAcceleration.y =m_images[i][j].Pose.AngularAcceleration.y;
                     parms.Images[i][j].Pose.AngularAcceleration.z =m_images[i][j].Pose.AngularAcceleration.z;
             parms.Images[i][j].Pose.AngularVelocity.x =m_images[i][j].Pose.AngularVelocity.x;
               parms.Images[i][j].Pose.AngularVelocity.y =m_images[i][j].Pose.AngularVelocity.y;
                 parms.Images[i][j].Pose.AngularVelocity.z =m_images[i][j].Pose.AngularVelocity.z;
             parms.Images[i][j].Pose.LinearAcceleration.x =m_images[i][j].Pose.LinearAcceleration.x;
             parms.Images[i][j].Pose.LinearAcceleration.y =m_images[i][j].Pose.LinearAcceleration.y;
             parms.Images[i][j].Pose.LinearAcceleration.z =m_images[i][j].Pose.LinearAcceleration.z;
              parms.Images[i][j].Pose.LinearVelocity.x =m_images[i][j].Pose.LinearVelocity.x;
               parms.Images[i][j].Pose.LinearVelocity.y =m_images[i][j].Pose.LinearVelocity.y;
                parms.Images[i][j].Pose.LinearVelocity.z =m_images[i][j].Pose.LinearVelocity.z;
               parms.Images[i][j].Pose.Pose.Orientation.w =m_images[i][j].Pose.Pose.Orientation.w;
               parms.Images[i][j].Pose.Pose.Orientation.x =m_images[i][j].Pose.Pose.Orientation.x;
               parms.Images[i][j].Pose.Pose.Orientation.y =m_images[i][j].Pose.Pose.Orientation.y;
               parms.Images[i][j].Pose.Pose.Orientation.z =m_images[i][j].Pose.Pose.Orientation.z;


               parms.Images[i][j].Pose.Pose.Position.x =m_images[i][j].Pose.Pose.Position.x;
               parms.Images[i][j].Pose.Pose.Position.y =m_images[i][j].Pose.Pose.Position.y;
               parms.Images[i][j].Pose.Pose.Position.z =m_images[i][j].Pose.Pose.Position.z;


                parms.Images[i][j].Pose.TimeInSeconds =m_images[i][j].Pose.TimeInSeconds;




         }

    parms.WarpOptions = m_smoothOptions;
    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++)
    {
        parms.ExternalVelocity.M[i][j]= m_externalVelocity.M[i][j];

    }

   parms.MinimumVsyncs =		m_minimumVsyncs;
   parms.PreScheduleSeconds =   m_preScheduleSeconds;
   parms.WarpProgram =			m_smoothProgram;
    for(int i=0;i<4;i++)
    {
        parms.ProgramParms[i]= m_programParms[i];

    }

 return parms;
}
*/
