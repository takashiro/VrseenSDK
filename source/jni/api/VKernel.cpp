/************************************************************************************

Filename    :   VrApi.cpp
Content     :   Primary C level interface necessary for VR, App builds on this
Created     :   July, 2014
Authors     :   John Carmack

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "VDevice.h"
#include "VFrameSmooth.h"

#include "VKernel.h"
#include "../App.h"

#include <unistd.h>						// gettid, usleep, etc
#include <jni.h>
#include <sstream>
#include <math.h>

#include <VLog.h>

#include "api/VGlOperation.h"
#include "android/JniUtils.h"
#include "android/VOsBuild.h"

#include "VString.h"			// for ReadFreq()
#include "VJson.h"			// needed for ovr_StartSystemActivity
#include "sensor/DeviceImpl.h"

#include "HmdSensors.h"
#include "Vsync.h"
#include "VSystemActivities.h"
#include "VThread.h"

NV_USING_NAMESPACE

// Platform UI command strings.
#define PUI_GLOBAL_MENU				"globalMenu"
#define PUI_GLOBAL_MENU_TUTORIAL	"globalMenuTutorial"
#define PUI_CONFIRM_QUIT			"confirmQuit"
#define PUI_THROTTLED1				"throttled1"	// Warn that Power Save Mode has been activated
#define PUI_THROTTLED2				"throttled2"	// Warn that Minimum Mode has been activated
#define PUI_HMT_UNMOUNT				"HMT_unmount"	// the HMT has been taken off the head
#define PUI_HMT_MOUNT				"HMT_mount"		// the HMT has been placed on the head
#define PUI_WARNING					"warning"		// the HMT has been placed on the head and a warning message shows

// version 0 is pre-json
// #define PLATFORM_UI_VERSION 1	// initial version
#define PLATFORM_UI_VERSION 2		// added "exitToHome" - apps built with current versions only respond to "returnToLauncher" if
// the Systems Activity that sent it is version 1 (meaning they'll never get an "exitToHome"
// from System Activities)

// FIXME:VRAPI move to ovrMobile
static HMDState * OvrHmdState = NULL;
float OvrHmdYaw;

static VKernel* instance = NULL;
// Valid for the thread that called ovr_EnterVrMode
static JNIEnv	*				Jni;
static  VFrameSmooth* frameSmooth = NULL;
static jobject  ActivityObject = NULL;


// This must be called by a function called directly from a java thread,
// preferably at JNI_OnLoad().  It will fail if called from a pthread created
// in native code, or from a NativeActivity due to the class-lookup issue:
//
// http://developer.android.com/training/articles/perf-jni.html#faq_FindClass
//
// This should not start any threads or consume any significant amount of
// resources, so hybrid apps aren't penalizing their normal mode of operation
// by supporting VR.
void		ovr_OnLoad( JavaVM * JavaVm_ );

// For a dedicated VR app this is called from JNI_OnLoad().
// A hybrid app, however, may want to defer calling it until the first headset
// plugin event to avoid starting the device manager.
void		ovr_Init();

// Shutdown without exiting the activity.
// A dedicated VR app will call ovr_ExitActivity instead, but a hybrid
// app may call this when leaving VR mode.
void		ovr_Shutdown();

// Returns a 3x3 minor of a 4x4 matrix.
inline float ovrMatrix4f_Minor( const ovrMatrix4f * m, int r0, int r1, int r2, int c0, int c1, int c2 )
{
    return	m->M[r0][c0] * ( m->M[r1][c1] * m->M[r2][c2] - m->M[r2][c1] * m->M[r1][c2] ) -
              m->M[r0][c1] * ( m->M[r1][c0] * m->M[r2][c2] - m->M[r2][c0] * m->M[r1][c2] ) +
              m->M[r0][c2] * ( m->M[r1][c0] * m->M[r2][c1] - m->M[r2][c0] * m->M[r1][c1] );
}


void ovr_InitSensors()
{
    OvrHmdState = new HMDState();
    if ( OvrHmdState != NULL )
    {
        OvrHmdState->initDevice();
    }

    if ( OvrHmdState == NULL )
    {
        FAIL( "failed to create HMD device" );
    }

    // Start the sensor running
    OvrHmdState->startSensor( ovrHmdCap_Orientation|ovrHmdCap_YawCorrection, 0 );
}

void ovr_ShutdownSensors()
{
    if ( OvrHmdState != NULL )
    {
        delete OvrHmdState;
        OvrHmdState = NULL;
    }
}

bool ovr_InitializeInternal()
{
    ovr_InitSensors();

    return true;
}

void ovr_Shutdown()
{
    ovr_ShutdownSensors();

#ifdef OVR_ENABLE_THREADS
    // Wait for all threads to finish; this must be done so that memory
    // allocator and all destructors finalize correctly.
    VThread::FinishAllThreads();
#endif
}

using namespace NervGear;

namespace NervGear {

    SensorState::operator const ovrSensorState& () const
    {
        OVR_COMPILER_ASSERT(sizeof(SensorState) == sizeof(ovrSensorState));
        return reinterpret_cast<const ovrSensorState&>(*this);
    }

} // namespace NervGear

ovrSensorState ovr_GetSensorStateInternal( double absTime )
{
    if ( OvrHmdState == NULL )
    {
        ovrSensorState state;
        memset( &state, 0, sizeof( state ) );
        state.Predicted.Pose.Orientation.w = 1.0f;
        state.Recorded.Pose.Orientation.w = 1.0f;
        return state;
    }
    return OvrHmdState->predictedSensorState( absTime );
}

void ovr_RecenterYawInternal()
{
    if ( OvrHmdState == NULL )
    {
        return;
    }
    OvrHmdState->recenterYaw();
}

// Does latency test processing and returns 'true' if specified rgb color should
// be used to clear the screen.
bool ovr_ProcessLatencyTest( unsigned char rgbColorOut[3] )
{
    if ( OvrHmdState == NULL )
    {
        return false;
    }
    return OvrHmdState->processLatencyTest( rgbColorOut );
}

// Returns non-null string once with latency test result, when it is available.
// Buffer is valid until next call.
const char * ovr_GetLatencyTestResult()
{
    if ( OvrHmdState == NULL )
    {
        return "";
    }
    return OvrHmdState->latencyTestResult();
}

int ovr_GetDeviceManagerThreadTid()
{
    if ( OvrHmdState == NULL )
    {
        return 0;
    }
    return static_cast<NervGear::DeviceManagerImpl *>( OvrHmdState->deviceManager() )->threadTid();
}

/*
 * This interacts with the VrLib java class to deal with Android platform issues.
 */

// This is public for any user.
JavaVM	* VrLibJavaVM;

static pid_t	OnLoadTid;

// This needs to be looked up by a thread called directly from java,
// not a native pthread.
static jclass	VrLibClass = NULL;
static jclass	ProximityReceiverClass = NULL;
static jclass	DockReceiverClass = NULL;

static jmethodID getPowerLevelStateID = NULL;
static jmethodID setActivityWindowFullscreenID = NULL;
static jmethodID notifyMountHandledID = NULL;


static bool hmtIsMounted = false;

// Register the HMT receivers once, and do
// not unregister in Pause(). We may miss
// important mount or dock events while
// the receiver is unregistered.
static bool registerHMTReceivers = false;

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

template< typename T, T _initValue_ >
class LocklessVar
{
public:
    LocklessVar() : Value( _initValue_ ) { }
    LocklessVar( T const v ) : Value( v ) { }

    T	Value;
};

class LocklessDouble
{
public:
    LocklessDouble( const double v ) : Value( v ) { };
    LocklessDouble() : Value( -1 ) { };

    double Value;
};

typedef LocklessVar< int, -1> 						volume_t;
NervGear::VLockless< volume_t >					CurrentVolume;
NervGear::VLockless< LocklessDouble >				TimeOfLastVolumeChange;
NervGear::VLockless< bool >						HeadsetPluggedState;
NervGear::VLockless< bool >						PowerLevelStateThrottled;
NervGear::VLockless< bool >						PowerLevelStateMinimum;
NervGear::VLockless< HMTMountState_t >				HMTMountState;
NervGear::VLockless< HMTDockState_t >				HMTDockState;	// edge triggered events, not the actual state
static NervGear::VLockless< bool >					DockState;

extern "C"
{
// The JNIEXPORT macro prevents the functions from ever being stripped out of the library.

void Java_com_vrseen_nervgear_VrLib_nativeVsync( JNIEnv *jni, jclass clazz, jlong frameTimeNanos );

JNIEXPORT jint JNI_OnLoad( JavaVM * vm, void * reserved )
{
LOG( "JNI_OnLoad" );

// Lookup our classnames
ovr_OnLoad( vm );

// Start up the Oculus device manager
ovr_Init();

return JNI_VERSION_1_6;
}

JNIEXPORT void Java_com_vrseen_nervgear_VrLib_nativeVolumeEvent(JNIEnv *jni, jclass clazz, jint volume)
{
    LOG( "nativeVolumeEvent(%i)", volume );

    CurrentVolume.setState( volume );
    double now = ovr_GetTimeInSeconds();

    TimeOfLastVolumeChange.setState( now );
}

JNIEXPORT void Java_com_vrseen_nervgear_VrLib_nativeHeadsetEvent(JNIEnv *jni, jclass clazz, jint state)
{
    LOG( "nativeHeadsetEvent(%i)", state );
    HeadsetPluggedState.setState( ( state == 1 ) );
}

JNIEXPORT void Java_com_vrseen_nervgear_ProximityReceiver_nativeMountHandled(JNIEnv *jni, jclass clazz)
{
    LOG( "Java_com_vrseen_nervgear_VrLib_nativeMountEventHandled" );

    // If we're received this, the foreground app has already received
    // and processed the mount event.
    if ( HMTMountState.state().MountState == HMT_MOUNT_MOUNTED )
    {
        LOG( "RESETTING MOUNT" );
        HMTMountState.setState( HMTMountState_t( HMT_MOUNT_NONE ) );
    }
}

JNIEXPORT void Java_com_vrseen_nervgear_ProximityReceiver_nativeProximitySensor(JNIEnv *jni, jclass clazz, jint state)
{
    LOG( "nativeProximitySensor(%i)", state );
    if ( state == 0 )
    {
        HMTMountState.setState( HMTMountState_t( HMT_MOUNT_UNMOUNTED ) );
    }
    else
    {
        HMTMountState.setState( HMTMountState_t( HMT_MOUNT_MOUNTED ) );
    }
}

JNIEXPORT void Java_com_vrseen_nervgear_DockReceiver_nativeDockEvent(JNIEnv *jni, jclass clazz, jint state)
{
    LOG( "nativeDockEvent = %s", ( state == 0 ) ? "UNDOCKED" : "DOCKED" );

    DockState.setState( state != 0 );

    if ( state == 0 )
    {
        // On undock, we need to do the following 2 things:
        // (1) Provide a means for apps to save their state.
        // (2) Call finish() to kill app.

        // NOTE: act.finish() triggers OnPause() -> OnStop() -> OnDestroy() to
        // be called on the activity. Apps may place save data and state in
        // OnPause() (or OnApplicationPause() for Unity)

        HMTDockState.setState( HMTDockState_t( HMT_DOCK_UNDOCKED ) );
    }
    else
    {
        HMTDockState_t dockState = HMTDockState.state();
        if ( dockState.DockState == HMT_DOCK_UNDOCKED )
        {
            LOG( "CLEARING UNDOCKED!!!!" );
        }
        HMTDockState.setState( HMTDockState_t( HMT_DOCK_DOCKED ) );
    }
}

} // extern "C"

const char *ovr_GetVersionString()
{
    return NV_VERSION_STRING;
}

double ovr_GetTimeInSeconds()
{
    return NervGear::VTimer::Seconds();
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
    LOG( "ovr_OnLoad()" );

    if ( JavaVm_ == NULL )
    {
        FAIL( "JavaVm == NULL" );
    }
    if ( VrLibJavaVM != NULL )
    {
        // Should we silently return instead?
        FAIL( "ovr_OnLoad() called twice" );
    }

    VrLibJavaVM = JavaVm_;
    OnLoadTid = gettid();

    JNIEnv * jni;
    bool privateEnv = false;
    if ( JNI_OK != VrLibJavaVM->GetEnv( reinterpret_cast<void**>(&jni), JNI_VERSION_1_6 ) )
    {
        LOG( "Creating temporary JNIEnv" );
        // We will detach after we are done
        privateEnv = true;
        const jint rtn = VrLibJavaVM->AttachCurrentThread( &jni, 0 );
        if ( rtn != JNI_OK )
        {
            FAIL( "AttachCurrentThread returned %i", rtn );
        }
    }
    else
    {
        LOG( "Using caller's JNIEnv" );
    }

    VrLibClass = JniUtils::GetGlobalClassReference( jni, "com/vrseen/nervgear/VrLib" );
    ProximityReceiverClass = JniUtils::GetGlobalClassReference( jni, "com/vrseen/nervgear/ProximityReceiver" );
    DockReceiverClass = JniUtils::GetGlobalClassReference( jni, "com/vrseen/nervgear/DockReceiver" );

    // Get the BuildVersion SDK
    jclass versionClass = jni->FindClass( "android/os/Build$VERSION" );
    if ( versionClass != 0 )
    {
        jfieldID sdkIntFieldID = jni->GetStaticFieldID( versionClass, "SDK_INT", "I" );
        if ( sdkIntFieldID != 0 )
        {
            BuildVersionSDK = jni->GetStaticIntField( versionClass, sdkIntFieldID );
            LOG( "BuildVersionSDK %d", BuildVersionSDK );
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
                    { DockReceiverClass, 		{ "nativeDockEvent", "(I)V",(void*)Java_com_vrseen_nervgear_DockReceiver_nativeDockEvent } },
                    { ProximityReceiverClass, 	{ "nativeProximitySensor", "(I)V",(void*)Java_com_vrseen_nervgear_ProximityReceiver_nativeProximitySensor } },
                    { ProximityReceiverClass, 	{ "nativeMountHandled", "()V",(void*)Java_com_vrseen_nervgear_ProximityReceiver_nativeMountHandled } },
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
            FAIL( "RegisterNatives failed on %s", gMethods[i].Jnim.name, 1 );
        }
    }

    // Detach if the caller wasn't already attached
    if ( privateEnv )
    {
        LOG( "Freeing temporary JNIEnv" );
        VrLibJavaVM->DetachCurrentThread();
    }
}

// A dedicated VR app will usually call this immediately after ovr_OnLoad(),
// but a hybrid app may want to defer calling it until the first headset
// plugin event to avoid starting the device manager.
void ovr_Init()
{
    LOG( "ovr_Init" );

    // initialize Oculus code
    ovr_InitializeInternal();

    JNIEnv * jni;
    const jint rtn = VrLibJavaVM->AttachCurrentThread( &jni, 0 );
    if ( rtn != JNI_OK )
    {
        FAIL( "AttachCurrentThread returned %i", rtn );
    }

    // After ovr_Initialize(), because it uses String
    VOsBuild::Init(jni);

    NervGear::VSystemActivities::instance()->initEventQueues();
}

void CreateSystemActivitiesCommand(const char * toPackageName, const char * command, const char * uri, NervGear::VString & out )
{
    // serialize the command to a JSON object with version inf
    VJsonObject obj;
    obj.insert("Command", command);
    obj.insert("OVRVersion", ovr_GetVersionString());
    obj.insert("PlatformUIVersion", PLATFORM_UI_VERSION);
    obj.insert("ToPackage", toPackageName);

    std::stringstream s;
    s << VJson(std::move(obj));

    char text[10240];
    s.getline(text, 10240);
    out = text;
}

// This can be called before ovr_EnterVrMode() so hybrid apps can tell
// when they need to go to vr mode.
void ovr_RegisterHmtReceivers()
{
    if ( registerHMTReceivers )
    {
        return;
    }
    const jmethodID startProximityReceiverId = JniUtils::GetStaticMethodID( Jni, ProximityReceiverClass,
                                                                            "startReceiver", "(Landroid/app/Activity;)V" );
    Jni->CallStaticVoidMethod( ProximityReceiverClass, startProximityReceiverId, ActivityObject );

    const jmethodID startDockReceiverId = JniUtils::GetStaticMethodID( Jni, DockReceiverClass,
                                                                       "startDockReceiver", "(Landroid/app/Activity;)V" );
    Jni->CallStaticVoidMethod( DockReceiverClass, startDockReceiverId, ActivityObject );

    registerHMTReceivers = true;
}

int ovr_GetVolume()
{
    return CurrentVolume.state().Value;
}

double ovr_GetTimeSinceLastVolumeChange()
{
    double value = TimeOfLastVolumeChange.state().Value;
    if ( value == -1 )
    {
        //LOG( "ovr_GetTimeSinceLastVolumeChange() : Not initialized.  Returning -1" );
        return -1;
    }
    return ovr_GetTimeInSeconds() - value;
}

eVrApiEventStatus ovr_nextPendingEvent( VString& buffer, unsigned int const bufferSize )
{
    eVrApiEventStatus status = NervGear::VSystemActivities::instance()->nextPendingMainEvent( buffer, bufferSize );
    if ( status < VRAPI_EVENT_PENDING )
    {
        return status;
    }

    // Parse to JSON here to determine if we should handle the event natively, or pass it along to the client app.
    VJson reader = VJson::Parse(buffer.toLatin1());
    if (reader.isObject())
    {
        VString command = reader.value( "Command" ).toString();
        if (command == SYSTEM_ACTIVITY_EVENT_REORIENT) {
            // for reorient, we recenter yaw natively, then pass the event along so that the client
            // application can also handle the event (for instance, to reposition menus)
            LOG( "Queuing internal reorient event." );
            ovr_RecenterYawInternal();
            // also queue as an internal event
            NervGear::VSystemActivities::instance()->addInternalEvent( buffer );
        } else if (command == SYSTEM_ACTIVITY_EVENT_RETURN_TO_LAUNCHER) {
            // In the case of the returnToLauncher event, we always handler it internally and pass
            // along an empty buffer so that any remaining events still get processed by the client.
            LOG( "Queuing internal returnToLauncher event." );
            // queue as an internal event
            NervGear::VSystemActivities::instance()->addInternalEvent( buffer );
            // treat as an empty event externally
            buffer = "";
            status = VRAPI_EVENT_CONSUMED;
        }
    }
    else
    {
        // a malformed event string was pushed! This implies an error in the native code somewhere.
        WARN( "Error parsing System Activities Event");
        return VRAPI_EVENT_INVALID_JSON;
    }
    return status;
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
    const ovrMatrix4f tanAngleMatrix = TanAngleMatrixFromFov( 90.0f );
    memset( &m_images, 0, sizeof( m_images ) );
    memset( &m_externalVelocity, 0, sizeof( m_externalVelocity ) );
   for ( int eye = 0; eye < 2; eye++ )
   {
       for ( int i = 0; i < 3; i++ )
       {
           m_images[eye][i].TexCoordsFromTanAngles = tanAngleMatrix;
           m_images[eye][i].Pose.Pose.Orientation.w = 1.0f;
       }
   }

   m_externalVelocity.M[0][0] = 1.0f;
   m_externalVelocity.M[1][1] = 1.0f;
   m_externalVelocity.M[2][2] = 1.0f;
   m_externalVelocity.M[3][3] = 1.0f;
   m_minimumVsyncs = 1;
   m_preScheduleSeconds = 0.014f;
   m_smoothProgram = WP_SIMPLE;
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
            FAIL( "couldn't get getDisplayWidth" );
        }
        instance->device->widthbyMeters = Jni->CallStaticFloatMethod(VrLibClass, getDisplayWidth, ActivityObject);

        jmethodID getDisplayHeight = Jni->GetStaticMethodID( VrLibClass, "getDisplayHeight", "(Landroid/app/Activity;)F" );
        if ( !getDisplayHeight )
        {
            FAIL( "couldn't get getDisplayHeight" );
        }
        instance->device->heightbyMeters = Jni->CallStaticFloatMethod( VrLibClass, getDisplayHeight, ActivityObject );
    }

    // Update the dimensions in pixels directly from the window
    instance->device->widthbyPixels = windowSurfaceWidth;
    instance->device->heightbyPixels = windowSurfaceHeight;

    LOG( "hmdInfo.lensSeparation = %f", instance->device->lensDistance );
    LOG( "hmdInfo.widthMeters = %f", instance->device->widthbyMeters );
    LOG( "hmdInfo.heightMeters = %f", instance->device->heightbyMeters );
    LOG( "hmdInfo.widthPixels = %i", instance->device->widthbyPixels );
    LOG( "hmdInfo.heightPixels = %i", instance->device->heightbyPixels );
    LOG( "hmdInfo.eyeTextureResolution[0] = %i", instance->device->eyeDisplayResolution[0] );
    LOG( "hmdInfo.eyeTextureResolution[1] = %i", instance->device->eyeDisplayResolution[1] );
    LOG( "hmdInfo.eyeTextureFov[0] = %f", instance->device->eyeDisplayFov[0] );
    LOG( "hmdInfo.eyeTextureFov[1] = %f", instance->device->eyeDisplayFov[1] );
}


void VKernel::run()
{
    if(isRunning) return;

    LOG( "---------- VKernel run ----------" );
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
        FAIL( "AttachCurrentThread returned %i", rtn );
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
    LOG( "OVR_VERSION = %s", ovr_GetVersionString() );

    isRunning = true;

    ovrSensorState state = ovr_GetSensorStateInternal( ovr_GetTimeInSeconds() );
    if ( state.Status & ovrStatus_OrientationTracked )
    {
        LOG( "HMD sensor attached.");
    }
    else
    {
        WARN( "Operating without a sensor.");
    }

    // Let GlUtils look up extensions
    VGlOperation glOperation;
    glOperation.logExtensions();

    // Look up the window surface size (NOTE: This must happen before Direct Render
    // Mode is initiated and the pbuffer surface is bound).
    {
        EGLDisplay display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
        EGLSurface surface = eglGetCurrentSurface( EGL_DRAW );
        eglQuerySurface( display, surface, EGL_WIDTH, &windowSurfaceWidth );
        eglQuerySurface( display, surface, EGL_HEIGHT, &windowSurfaceHeight );
        LOG( "Window Surface Size: [%dx%d]", windowSurfaceWidth, windowSurfaceHeight );
    }

    // Based on sensor ID and platform, determine the HMD
    UpdateHmdInfo();

    // Start up our vsync callbacks.
    const jmethodID startVsyncId = JniUtils::GetStaticMethodID( Jni, VrLibClass,
                                                                "startVsync", "(Landroid/app/Activity;)V" );
    Jni->CallStaticVoidMethod( VrLibClass, startVsyncId, ActivityObject );

    // Register our HMT receivers if they have not already been registered.
    ovr_RegisterHmtReceivers();

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
    VString externalStorageDirectory = JniUtils::Convert(Jni, externalStorageDirectoryString);
    Jni->DeleteLocalRef(externalStorageDirectoryString);

    if ( Jni->ExceptionOccurred() )
    {
        Jni->ExceptionClear();
        LOG( "Cleared JNI exception" );
    }

    frameSmooth = new VFrameSmooth(asyncSmooth,device);

    if ( setActivityWindowFullscreenID != NULL)
    {
        Jni->CallStaticVoidMethod( VrLibClass, setActivityWindowFullscreenID, ActivityObject );
    }

}

void VKernel::exit()
{
    LOG( "---------- VKernel Exit ----------" );

    if (!isRunning)
    {
        WARN( "Skipping ovr_LeaveVrMode: ovr already Destroyed" );
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
            LOG("Cleared JNI exception");
        }
        LOG( "Calling activity.finishOnUiThread()" );
        Jni->CallStaticVoidMethod( VrLibClass, mid, *static_cast< jobject* >( &ActivityObject ) );
        LOG( "Returned from activity.finishOnUiThread()" );
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
            LOG("Cleared JNI exception");
        }
        LOG( "Calling activity.finishAffinityOnUiThread()" );
        Jni->CallStaticVoidMethod( VrLibClass, mid, *static_cast< jobject* >( &ActivityObject ) );
        LOG( "Returned from activity.finishAffinityOnUiThread()" );
    }
    else if ( exitType == EXIT_TYPE_EXIT )
    {
        LOG( "Calling exitType EXIT_TYPE_EXIT" );
        NervGear::VSystemActivities::instance()->shutdownEventQueues();
        ovr_Shutdown();
        delete  instance;
        instance = NULL;
    }
}



void VKernel::setSmoothEyeTexture(unsigned int texID,ushort eye,ushort layer)
{

   m_images[eye][layer].TexId =  texID;

}
void VKernel::setSmoothOption(int option)
{
  m_smoothOptions = option;

}
void VKernel::setMinimumVsncs( int vsnc)
{

  m_minimumVsyncs = vsnc;
}
void VKernel::setExternalVelocity(ovrMatrix4f extV)

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
void VKernel::setSmoothProgram(ovrTimeWarpProgram program)
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
         frameSmooth->setSmoothEyeTexture(eye,i,m_images[eye][i]) ;

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

   // frameSmooth->doSmooth(parms);

}



static  void ovr_HandleHmdEvents()
{
    // check if the HMT has been undocked
    HMTDockState_t dockState = HMTDockState.state();
    if ( dockState.DockState == HMT_DOCK_UNDOCKED )
    {
        LOG( "ovr_HandleHmdEvents::Hmt was disconnected" );

        // reset the sensor info
        if ( OvrHmdState != NULL )
        {
            OvrHmdState->resetSensor();
        }

        // reset the real dock state since we're handling the change
        HMTDockState.setState( HMTDockState_t( HMT_DOCK_NONE ) );

        instance->destroy(EXIT_TYPE_FINISH_AFFINITY );

        return;
    }

    // check if the HMT has been mounted or unmounted
    HMTMountState_t mountState = HMTMountState.state();
    if ( mountState.MountState != HMT_MOUNT_NONE )
    {
        // reset the real mount state since we're handling the change
        HMTMountState.setState( HMTMountState_t( HMT_MOUNT_NONE ) );

        if ( mountState.MountState == HMT_MOUNT_MOUNTED )
        {
            if ( hmtIsMounted )
            {
                LOG( "ovr_HandleHmtEvents: HMT is already mounted" );
            }
            else
            {
                LOG( "ovr_HandleHmdEvents: HMT was mounted" );
                hmtIsMounted = true;

                // broadcast to background apps that mount has been handled
                if ( notifyMountHandledID != NULL )
                {
                    Jni->CallStaticVoidMethod( VrLibClass, notifyMountHandledID, ActivityObject);
                }

                NervGear::VString reorientMessage;
                CreateSystemActivitiesCommand( "", SYSTEM_ACTIVITY_EVENT_REORIENT, "", reorientMessage );
                NervGear::VSystemActivities::instance()->addEvent( reorientMessage );
            }
        }
        else if ( mountState.MountState == HMT_MOUNT_UNMOUNTED )
        {
            LOG( "ovr_HandleHmdEvents: HMT was UNmounted" );

            hmtIsMounted = false;
        }
    }
}

void VKernel::ovr_HandleDeviceStateChanges()
{
    // Test for Hmd Events such as mount/unmount, dock/undock
    ovr_HandleHmdEvents();

    // check for pending events that must be handled natively
    size_t const MAX_EVENT_SIZE = 4096;
//	char eventBuffer[MAX_EVENT_SIZE];
    VString eventBuffer;

    for ( eVrApiEventStatus status = NervGear::VSystemActivities::instance()->nextPendingInternalEvent( eventBuffer, MAX_EVENT_SIZE );
          status >= VRAPI_EVENT_PENDING;
          status = NervGear::VSystemActivities::instance()->nextPendingInternalEvent( eventBuffer, MAX_EVENT_SIZE ) )
    {
        if ( status != VRAPI_EVENT_PENDING )
        {
            WARN( "Error %i handing internal System Activities Event", status );
            continue;
        }

        VJson reader = VJson::Parse(eventBuffer.toLatin1());
        if ( reader.isObject() )
        {
            VString command = reader.value("Command").toString();
            int32_t platformUIVersion = reader.value("PlatformUIVersion").toInt();
            if (command == SYSTEM_ACTIVITY_EVENT_REORIENT)
            {
                // for reorient, we recenter yaw natively, then pass the event along so that the client
                // application can also handle the event (for instance, to reposition menus)
                LOG( "ovr_HandleDeviceStateChanges: Acting on System Activity reorient event." );
                ovr_RecenterYawInternal();
            }
            else if (command == SYSTEM_ACTIVITY_EVENT_RETURN_TO_LAUNCHER && platformUIVersion < 2 )
            {
                // In the case of the returnToLauncher event, we always handler it internally and pass
                // along an empty buffer so that any remaining events still get processed by the client.
                LOG( "ovr_HandleDeviceStateChanges: Acting on System Activity returnToLauncher event." );
                // PlatformActivity and Home should NEVER get one of these!
                instance->destroy(EXIT_TYPE_FINISH_AFFINITY);
            }
        }
        else
        {
            // a malformed event string was pushed! This implies an error in the native code somewhere.
            WARN( "Error parsing System Activities Event");
        }
    }
}

ovrSensorState VKernel::ovr_GetPredictedSensorState(double absTime )
{
    return ovr_GetSensorStateInternal( absTime );
}

void VKernel::ovr_RecenterYaw()
{
    ovr_RecenterYawInternal();
}

void VKernel::doSmooth(const ovrTimeWarpParms * parms )
{
    if(frameSmooth==NULL||!isRunning) return;

    setSmoothParms(*parms);
    syncSmoothParms();
    frameSmooth->doSmooth();
}
 ovrTimeWarpParms VKernel::InitTimeWarpParms( const ovrWarpInit init, const unsigned int texId )
{
    const ovrMatrix4f tanAngleMatrix = TanAngleMatrixFromFov( 90.0f );

    ovrTimeWarpParms parms;
    memset( &parms, 0, sizeof( parms ) );

    for ( int eye = 0; eye < MAX_WARP_EYES; eye++ )
    {
        for ( int i = 0; i < MAX_WARP_IMAGES; i++ )
        {
            parms.Images[eye][i].TexCoordsFromTanAngles = tanAngleMatrix;
            parms.Images[eye][i].Pose.Pose.Orientation.w = 1.0f;
        }
    }
    parms.ExternalVelocity.M[0][0] = 1.0f;
    parms.ExternalVelocity.M[1][1] = 1.0f;
    parms.ExternalVelocity.M[2][2] = 1.0f;
    parms.ExternalVelocity.M[3][3] = 1.0f;
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
