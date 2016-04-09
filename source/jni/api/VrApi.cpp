/************************************************************************************

Filename    :   VrApi.cpp
Content     :   Primary C level interface necessary for VR, App builds on this
Created     :   July, 2014
Authors     :   John Carmack

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "VrApi.h"

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
#include "MemBuffer.h"		// needed for MemBufferT
#include "sensor/DeviceImpl.h"

#include "VDevice.h"
#include "HmdSensors.h"
#include "VFrameSmooth.h"
#include "VrApi_local.h"
#include "Vsync.h"
#include "SystemActivities.h"

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

void ovr_InitSensors()
{
#if 0
	if ( OvrHmdState != NULL )
	{
		return;
	}

	OvrHmdState = new HMDState();
	if ( !OvrHmdState->InitDevice() )
	{
		vFatal("failed to create HMD device");
	}

	OvrHmdState->StartSensor( ovrHmdCap_Orientation|ovrHmdCap_YawCorrection, 0 );
	OvrHmdState->SetYaw( OvrHmdYaw );
#else

    OvrHmdState = new HMDState();
    if ( OvrHmdState != NULL )
	{
        OvrHmdState->initDevice();
    }

	if ( OvrHmdState == NULL )
	{
		vFatal("failed to create HMD device");
	}

	// Start the sensor running
    OvrHmdState->startSensor( ovrHmdCap_Orientation|ovrHmdCap_YawCorrection, 0 );
#endif
}

void ovr_ShutdownSensors()
{
#if 0
	if ( OvrHmdState == NULL )
	{
		return;
	}

	OvrHmdYaw = OvrHmdState->GetYaw();

	delete OvrHmdState;
	OvrHmdState = NULL;
#else

	if ( OvrHmdState != NULL )
	{
		delete OvrHmdState;
		OvrHmdState = NULL;
	}
#endif
}

bool ovr_InitializeInternal()
{
    // We must set up the system for the plugin to work
    if ( !NervGear::System::IsInitialized() )
	{
    	NervGear::System::Init( NervGear::Log::ConfigureDefaultLog( NervGear::LogMask_All ) );
	}

	ovr_InitSensors();

    return true;
}

void ovr_Shutdown()
{
	ovr_ShutdownSensors();

    // We should clean up the system to be complete
    NervGear::System::Destroy();
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
static jclass	ConsoleReceiverClass = NULL;

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
NervGear::LocklessUpdater< volume_t >					CurrentVolume;
NervGear::LocklessUpdater< LocklessDouble >				TimeOfLastVolumeChange;
NervGear::LocklessUpdater< bool >						HeadsetPluggedState;
NervGear::LocklessUpdater< bool >						PowerLevelStateThrottled;
NervGear::LocklessUpdater< bool >						PowerLevelStateMinimum;
NervGear::LocklessUpdater< HMTMountState_t >				HMTMountState;
NervGear::LocklessUpdater< HMTDockState_t >				HMTDockState;	// edge triggered events, not the actual state
static NervGear::LocklessUpdater< bool >					DockState;

extern "C"
{
// The JNIEXPORT macro prevents the functions from ever being stripped out of the library.

void Java_com_vrseen_nervgear_VrLib_nativeVsync( JNIEnv *jni, jclass clazz, jlong frameTimeNanos );

JNIEXPORT jint JNI_OnLoad( JavaVM * vm, void * reserved )
{
	vInfo("JNI_OnLoad");

	// Lookup our classnames
	ovr_OnLoad( vm );

	// Start up the Oculus device manager
	ovr_Init();

	return JNI_VERSION_1_6;
}

JNIEXPORT void Java_com_vrseen_nervgear_VrLib_nativeVolumeEvent(JNIEnv *jni, jclass clazz, jint volume)
{
    vInfo("nativeVolumeEvent(" << volume << ")");

    CurrentVolume.setState( volume );
    double now = ovr_GetTimeInSeconds();

    TimeOfLastVolumeChange.setState( now );
}

JNIEXPORT void Java_com_vrseen_nervgear_VrLib_nativeHeadsetEvent(JNIEnv *jni, jclass clazz, jint state)
{
    vInfo("nativeHeadsetEvent(" << state << ")");
    HeadsetPluggedState.setState( ( state == 1 ) );
}

JNIEXPORT void Java_com_vrseen_nervgear_ProximityReceiver_nativeMountHandled(JNIEnv *jni, jclass clazz)
{
	vInfo("Java_com_vrseen_nervgear_VrLib_nativeMountEventHandled");

	// If we're received this, the foreground app has already received
	// and processed the mount event.
	if ( HMTMountState.state().MountState == HMT_MOUNT_MOUNTED )
	{
		vInfo("RESETTING MOUNT");
		HMTMountState.setState( HMTMountState_t( HMT_MOUNT_NONE ) );
	}
}

JNIEXPORT void Java_com_vrseen_nervgear_ProximityReceiver_nativeProximitySensor(JNIEnv *jni, jclass clazz, jint state)
{
	vInfo("nativeProximitySensor(" << state << ")");
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
	vInfo("nativeDockEvent =" << ( state == 0 ) ? "UNDOCKED" : "DOCKED");

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
			vInfo("CLEARING UNDOCKED!!!!");
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
	OnLoadTid = gettid();

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
	ProximityReceiverClass = JniUtils::GetGlobalClassReference( jni, "com/vrseen/nervgear/ProximityReceiver" );
	DockReceiverClass = JniUtils::GetGlobalClassReference( jni, "com/vrseen/nervgear/DockReceiver" );
	ConsoleReceiverClass = JniUtils::GetGlobalClassReference( jni, "com/vrseen/nervgear/ConsoleReceiver" );

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
			vFatal("RegisterNatives failed on" << gMethods[i].Jnim.name1);
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

	// initialize Oculus code
	ovr_InitializeInternal();

	JNIEnv * jni;
	const jint rtn = VrLibJavaVM->AttachCurrentThread( &jni, 0 );
	if ( rtn != JNI_OK )
	{
		vFatal("AttachCurrentThread returned" << rtn);
	}

	// After ovr_Initialize(), because it uses String
    VOsBuild::Init(jni);

	NervGear::SystemActivities_InitEventQueues();
}

void ovr_ExitActivity( ovrMobile * ovr, eExitType exitType )
{
	if ( exitType == EXIT_TYPE_FINISH )
	{
		vInfo("ovr_ExitActivity( EXIT_TYPE_FINISH ) - act.finish()");

		ovr_LeaveVrMode( ovr );

		vAssert( ovr != NULL );

		//	const char * name = "finish";
		const char * name = "finishOnUiThread";
		const jmethodID mid = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass,
														   name, "(Landroid/app/Activity;)V" );

		if ( ovr->Jni->ExceptionOccurred() )
		{
			ovr->Jni->ExceptionClear();
			vInfo("Cleared JNI exception");
		}
		vInfo("Calling activity.finishOnUiThread()");
		ovr->Jni->CallStaticVoidMethod( VrLibClass, mid, *static_cast< jobject* >( &ovr->Parms.ActivityObject ) );
		vInfo("Returned from activity.finishOnUiThread()");
	}
	else if ( exitType == EXIT_TYPE_FINISH_AFFINITY )
	{
		vInfo("ovr_ExitActivity( EXIT_TYPE_FINISH_AFFINITY ) - act.finishAffinity()");

		vAssert( ovr != NULL );

		ovr_LeaveVrMode( ovr );

		const char * name = "finishAffinityOnUiThread";
		const jmethodID mid = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass,
														   name, "(Landroid/app/Activity;)V" );

		if ( ovr->Jni->ExceptionOccurred() )
		{
			ovr->Jni->ExceptionClear();
			vInfo("Cleared JNI exception");
		}
		vInfo("Calling activity.finishAffinityOnUiThread()");
		ovr->Jni->CallStaticVoidMethod( VrLibClass, mid, *static_cast< jobject* >( &ovr->Parms.ActivityObject ) );
		vInfo("Returned from activity.finishAffinityOnUiThread()");
	}
	else if ( exitType == EXIT_TYPE_EXIT )
	{
		vInfo("ovr_ExitActivity( EXIT_TYPE_EXIT ) - exit(0)");

		// This should only ever be called from the Java thread.
		// ovr_LeaveVrMode() should have been called already from the VrThread.
		vAssert( ovr == NULL || ovr->Destroyed );

		if ( OnLoadTid != gettid() )
		{
			vFatal("ovr_ExitActivity( EXIT_TYPE_EXIT ): Called with tid" << gettid() << "instead of" << OnLoadTid);
		}

		NervGear::SystemActivities_ShutdownEventQueues();
		ovr_Shutdown();
		exit( 0 );
	}
}

// This sends an explicit intent to the package/classname with the command and URI in the intent.
void ovr_SendIntent( ovrMobile * ovr, const char * actionName, const char * toPackageName,
		const char * toClassName, const char * command, const char * uri, eExitType exitType )
{
	LOG( "ovr_SendIntent( '%s' '%s/%s' '%s' '%s' )", actionName, toPackageName, toClassName,
			( command != NULL ) ? command : "<NULL>",
			( uri != NULL ) ? uri : "<NULL>" );

	// Eliminate any frames of lost head tracking, push black images to the screen
	const ovrTimeWarpParms warpSwapBlackParms = InitTimeWarpParms( WARP_INIT_BLACK );
	ovr_WarpSwap( ovr, &warpSwapBlackParms );

	// We need to leave before sending the intent, or the leave VR mode from the platform
	// activity can end up executing after we've already entered VR mode for the main activity.
	// This was showing up as the vsync callback getting turned off after it had already
	// been turned on.
	ovr_LeaveVrMode( ovr );

	JavaString actionString( ovr->Jni, actionName );
	JavaString packageString( ovr->Jni, toPackageName );
	JavaString className( ovr->Jni, toClassName );
	JavaString commandString( ovr->Jni, command == NULL ? PUI_GLOBAL_MENU : command );
	JavaString uriString( ovr->Jni, uri == NULL ? "" : uri );

	jmethodID sendIntentFromNativeId = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass,
			"sendIntentFromNative", "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V" );
	if ( sendIntentFromNativeId != NULL )
	{
		ovr->Jni->CallStaticVoidMethod( VrLibClass, sendIntentFromNativeId, ovr->Parms.ActivityObject,
				actionString.toJString(), packageString.toJString(), className.toJString(),
				commandString.toJString(), uriString.toJString() );
	}

	if ( exitType != EXIT_TYPE_NONE )
	{
		ovr_ExitActivity( ovr, exitType );
	}
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

// This will query Android for the launch intent of the specified package, then append the
// command and URI data to the intent.
void ovr_SendLaunchIntent( ovrMobile * ovr, const char * toPackageName, const char * command,
		const char * uri, eExitType exitType )
{
	LOG( "ovr_SendLaunchIntent( '%s' '%s' '%s' )", toPackageName,
			( command != NULL ) ? command : "<NULL>",
			( uri != NULL ) ? uri : "<NULL>" );

	// Eliminate any frames of lost head tracking, push black images to the screen
	const ovrTimeWarpParms warpSwapBlackParms = InitTimeWarpParms( WARP_INIT_BLACK );
	ovr_WarpSwap( ovr, &warpSwapBlackParms );

	// We need to leave before sending the intent, or the leave VR mode from the platform
	// activity can end up executing after we've already entered VR mode for the main activity.
	// This was showing up as the vsync callback getting turned off after it had already
	// been turned on.
	ovr_LeaveVrMode( ovr );

	JavaString packageString( ovr->Jni, toPackageName );
	JavaString commandString( ovr->Jni, command == NULL ? PUI_GLOBAL_MENU : command );
	JavaString uriString( ovr->Jni, uri == NULL ? "" : uri );

	jmethodID sendLaunchIntentId = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass,
			"sendLaunchIntent", "(Landroid/app/Activity;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V" );
	if ( sendLaunchIntentId != NULL )
	{
		ovr->Jni->CallStaticVoidMethod( VrLibClass, sendLaunchIntentId, ovr->Parms.ActivityObject,
				packageString.toJString(), commandString.toJString(), uriString.toJString() );
	}

	if ( exitType != EXIT_TYPE_NONE )
	{
		ovr_ExitActivity( ovr, exitType );
	}
}

bool ovr_StartSystemActivity_JSON( ovrMobile * ovr, const char * jsonText )
{
	return true;
}

// creates a command for sending to System Activities with optional embedded extra JSON text.
bool ovr_CreateSystemActivityIntent( ovrMobile * ovr, const char * command, const char * extraJsonText,
		char * outBuffer, unsigned long long const outBufferSize, unsigned long long & outRequiredBufferSize )
{
	outRequiredBufferSize = 0;
	if ( outBuffer == NULL || outBufferSize < 1 )
	{
		return false;
	}
	outBuffer[0] = '\0';

    VJsonObject jsonObj;

    jsonObj.insert("Command", command );
    jsonObj.insert("OVRVersion", ovr_GetVersionString() );
    jsonObj.insert( "PlatformUIVersion", PLATFORM_UI_VERSION );

    std::stringstream s;
    s << VJson(std::move(jsonObj));
    std::string str;
    s.str(str);

    outRequiredBufferSize = str.length() + 1;
    if ( extraJsonText != NULL && extraJsonText[0] != '\0' )
        outRequiredBufferSize += strlen(extraJsonText);

    if ( outBufferSize < outRequiredBufferSize )
        return false;

    // combine the JSON objects
    strcpy(outBuffer, str.c_str());
    if ( extraJsonText != NULL && extraJsonText[0] != '\0' ) {
        int offset = str.length() - 1;
        strcpy(outBuffer + offset, extraJsonText);
    }

	return true;
}

// creates a JSON object with command and version info in it and combines that with a pre-serialized json object
bool ovr_StartSystemActivity( ovrMobile * ovr, const char * command, const char * extraJsonText )
{
	unsigned long long const INTENT_COMMAND_SIZE = 1024;
	NervGear::MemBufferT< char > intentBuffer( INTENT_COMMAND_SIZE );
	unsigned long long requiredSize = 0;
	if ( !ovr_CreateSystemActivityIntent( ovr, command, extraJsonText, intentBuffer, INTENT_COMMAND_SIZE, requiredSize ) )
	{
		vAssert( requiredSize > INTENT_COMMAND_SIZE );	// if this isn't true, command creation failed for some other reason
		// reallocate a buffer of the required size
		intentBuffer.realloc( requiredSize );
		bool ok = ovr_CreateSystemActivityIntent( ovr, command, extraJsonText, intentBuffer, INTENT_COMMAND_SIZE, requiredSize );
		if ( !ok )
		{
			vAssert( ok );
			return false;
		}
	}

	return ovr_StartSystemActivity_JSON( ovr, intentBuffer );
}

static void UpdateHmdInfo( ovrMobile * ovr )
{
	short hmdVendorId = 0;
	short hmdProductId = 0;
	unsigned int hmdVersion = 0;

	// There is no sensor when entering VR mode before the device is docked.
	// Note that this may result in the wrong HMD info if GetDeviceHmdInfo()
	// needs the vendor/product id to identify the HMD.
	if ( OvrHmdState != NULL )
	{
		const NervGear::SensorInfo si = OvrHmdState->sensorInfo();
		hmdVendorId = si.VendorId;
		hmdProductId = si.ProductId;
		hmdVersion = si.Version;	// JDC: needed to disambiguate Samsung headsets
	}

	vInfo("VendorId =" << hmdVendorId);
	vInfo("ProductId =" << hmdProductId);
	vInfo("Version =" << hmdVersion);

    ovr->device = VDevice::instance();

    // Only use the Android info if we haven't explicitly set the screenWidth / height,
    // because they are reported wrong on the note.
    if(!ovr->device->widthbyMeters)
    {
        jmethodID getDisplayWidth = ovr->Jni->GetStaticMethodID( VrLibClass, "getDisplayWidth", "(Landroid/app/Activity;)F" );
        if ( !getDisplayWidth )
        {
            vFatal("couldn't get getDisplayWidth");
        }
        ovr->device->widthbyMeters = ovr->Jni->CallStaticFloatMethod(VrLibClass, getDisplayWidth, ovr->Parms.ActivityObject );

        jmethodID getDisplayHeight = ovr->Jni->GetStaticMethodID( VrLibClass, "getDisplayHeight", "(Landroid/app/Activity;)F" );
        if ( !getDisplayHeight )
        {
            vFatal("couldn't get getDisplayHeight");
        }
         ovr->device->heightbyMeters = ovr->Jni->CallStaticFloatMethod( VrLibClass, getDisplayHeight, ovr->Parms.ActivityObject );
    }

	// Update the dimensions in pixels directly from the window
	ovr->device->widthbyPixels = windowSurfaceWidth;
	ovr->device->heightbyPixels = windowSurfaceHeight;

	vInfo("hmdInfo.lensSeparation =" << ovr->device->lensDistance);
	vInfo("hmdInfo.widthMeters =" << ovr->device->widthbyMeters);
	vInfo("hmdInfo.heightMeters =" << ovr->device->heightbyMeters);
	vInfo("hmdInfo.widthPixels =" << ovr->device->widthbyPixels);
	vInfo("hmdInfo.heightPixels =" << ovr->device->heightbyPixels);
	vInfo("hmdInfo.eyeTextureResolution[0] =" << ovr->device->eyeDisplayResolution[0]);
	vInfo("hmdInfo.eyeTextureResolution[1] =" << ovr->device->eyeDisplayResolution[1]);
	vInfo("hmdInfo.eyeTextureFov[0] =" << ovr->device->eyeDisplayFov[0]);
	vInfo("hmdInfo.eyeTextureFov[1] =" << ovr->device->eyeDisplayFov[1]);
}

int ovr_GetSystemBrightness( ovrMobile * ovr )
{
	int cur = 255;
	jmethodID getSysBrightnessMethodId = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass, "getSystemBrightness", "(Landroid/app/Activity;)I" );
    if (getSysBrightnessMethodId != NULL && VOsBuild::getString(VOsBuild::Model).icompare("SM-G906S") != 0) {
		cur = ovr->Jni->CallStaticIntMethod( VrLibClass, getSysBrightnessMethodId, ovr->Parms.ActivityObject );
		vInfo("System brightness =" << cur);
	}
	return cur;
}

void ovr_SetSystemBrightness(  ovrMobile * ovr, int const v )
{
	int v2 = v < 0 ? 0 : v;
	v2 = v2 > 255 ? 255 : v2;
	jmethodID setSysBrightnessMethodId = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass, "setSystemBrightness", "(Landroid/app/Activity;I)V" );
    if (setSysBrightnessMethodId != NULL && VOsBuild::getString(VOsBuild::Model).icompare("SM-G906S") != 0) {
		ovr->Jni->CallStaticVoidMethod( VrLibClass, setSysBrightnessMethodId, ovr->Parms.ActivityObject, v2 );
		vInfo("Set brightness to" << v2);
		ovr_GetSystemBrightness( ovr );
	}
}

bool ovr_GetComfortModeEnabled( ovrMobile * ovr )
{
	jmethodID getComfortViewModeMethodId = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass, "getComfortViewModeEnabled", "(Landroid/app/Activity;)Z" );
	bool r = true;
    if ( getComfortViewModeMethodId != NULL && VOsBuild::getString(VOsBuild::Model).icompare("SM-G906S") != 0)
	{
		r = ovr->Jni->CallStaticBooleanMethod( VrLibClass, getComfortViewModeMethodId, ovr->Parms.ActivityObject );
		vInfo("System comfort mode =" << r ? "true" : "false");
	}
	return r;
}

void ovr_SetComfortModeEnabled( ovrMobile * ovr, bool const enabled )
{
	jmethodID enableComfortViewModeMethodId = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass, "enableComfortViewMode", "(Landroid/app/Activity;Z)V" );
    if ( enableComfortViewModeMethodId != NULL && VOsBuild::getString(VOsBuild::Model).icompare("SM-G906S") != 0)
	{
		ovr->Jni->CallStaticVoidMethod( VrLibClass, enableComfortViewModeMethodId, ovr->Parms.ActivityObject, enabled );
		vInfo("Set comfort mode to" << enabled ? "true" : "false");
		ovr_GetComfortModeEnabled( ovr );
	}
}

void ovr_SetDoNotDisturbMode( ovrMobile * ovr, bool const enabled )
{
	jmethodID setDoNotDisturbModeMethodId = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass, "setDoNotDisturbMode", "(Landroid/app/Activity;Z)V" );
    if ( setDoNotDisturbModeMethodId != NULL && VOsBuild::getString(VOsBuild::Model).icompare("SM-G906S") != 0)
	{
		ovr->Jni->CallStaticVoidMethod( VrLibClass, setDoNotDisturbModeMethodId, ovr->Parms.ActivityObject, enabled );
		vInfo("System DND mode =" << enabled ? "true" : "false");
	}
}

bool ovr_GetDoNotDisturbMode( ovrMobile * ovr )
{
	bool r = false;
	jmethodID getDoNotDisturbModeMethodId = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass, "getDoNotDisturbMode", "(Landroid/app/Activity;)Z" );
    if ( getDoNotDisturbModeMethodId != NULL && VOsBuild::getString(VOsBuild::Model).icompare("SM-G906S") != 0)
	{
		r = ovr->Jni->CallStaticBooleanMethod( VrLibClass, getDoNotDisturbModeMethodId, ovr->Parms.ActivityObject );
		vInfo("Set DND mode to" << r ? "true" : "false");
	}
	return r;
}

// This can be called before ovr_EnterVrMode() so hybrid apps can tell
// when they need to go to vr mode.
void ovr_RegisterHmtReceivers( JNIEnv * Jni, jobject ActivityObject )
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

	const jmethodID startConsoleReceiverId = JniUtils::GetStaticMethodID( Jni, ConsoleReceiverClass,
			"startReceiver", "(Landroid/app/Activity;)V" );
	Jni->CallStaticVoidMethod( ConsoleReceiverClass, startConsoleReceiverId, ActivityObject );

	registerHMTReceivers = true;
}

// Starts up TimeWarp, vsync tracking, sensor reading, clock locking, and sets video options.
// Should be called when the app is both resumed and has a window surface.
// The application must have their preferred OpenGL ES context current so the correct
// version and config ccan be configured for the background TimeWarp thread.
// On return, the context will be current on an invisible pbuffer, because TimeWarp
// will own the window.
ovrMobile * ovr_EnterVrMode( ovrModeParms parms, ovrHmdInfo * returnedHmdInfo )
{
	vInfo("---------- ovr_EnterVrMode ----------");
#if defined( OVR_BUILD_DEBUG )
	char const * buildConfig = "DEBUG";
#else
	char const * buildConfig = "RELEASE";
#endif

	// This returns the existing jni if the caller has already created
	// one, or creates a new one.
	JNIEnv	* Jni = NULL;
	const jint rtn = VrLibJavaVM->AttachCurrentThread( &Jni, 0 );
	if ( rtn != JNI_OK )
	{
		vFatal("AttachCurrentThread returned" << rtn);
	}

	// log the application name, version, activity, build, model, etc.
	jmethodID logApplicationNameMethodId = JniUtils::GetStaticMethodID( Jni, VrLibClass, "logApplicationName", "(Landroid/app/Activity;)V" );
	Jni->CallStaticVoidMethod( VrLibClass, logApplicationNameMethodId, parms.ActivityObject );

	jmethodID logApplicationVersionId = JniUtils::GetStaticMethodID( Jni, VrLibClass, "logApplicationVersion", "(Landroid/app/Activity;)V" );
	Jni->CallStaticVoidMethod( VrLibClass, logApplicationVersionId, parms.ActivityObject );

	jmethodID logApplicationVrType = JniUtils::GetStaticMethodID( Jni, VrLibClass, "logApplicationVrType", "(Landroid/app/Activity;)V" );
	Jni->CallStaticVoidMethod( VrLibClass, logApplicationVrType, parms.ActivityObject );

    VString currentClassName = JniUtils::GetCurrentActivityName(Jni, parms.ActivityObject);
    vInfo("ACTIVITY =" << currentClassName);

    vInfo("BUILD =" << VOsBuild::getString(VOsBuild::Display) << buildConfig);
    vInfo("MODEL =" << VOsBuild::getString(VOsBuild::Model));
	vInfo("OVR_VERSION =" << ovr_GetVersionString());
	vInfo("ovrModeParms.AsynchronousTimeWarp =" << parms.AsynchronousTimeWarp);
	vInfo("ovrModeParms.GameThreadTid =" << parms.GameThreadTid);

	ovrMobile * ovr = new ovrMobile;
	ovr->Jni = Jni;
	ovr->EnterTid = gettid();
	ovr->Parms = parms;
	ovr->Destroyed = false;

	ovrSensorState state = ovr_GetSensorStateInternal( ovr_GetTimeInSeconds() );
	if ( state.Status & ovrStatus_OrientationTracked )
	{
		vInfo("HMD sensor attached.");
	}
	else
	{
		vWarn("Operating without a sensor.");
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
		vInfo("Window Surface Size: [" << windowSurfaceWidthwindowSurfaceHeight << "]");
	}

	// Based on sensor ID and platform, determine the HMD
	UpdateHmdInfo( ovr );

	// Start up our vsync callbacks.
	const jmethodID startVsyncId = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass,
    		"startVsync", "(Landroid/app/Activity;)V" );
	ovr->Jni->CallStaticVoidMethod( VrLibClass, startVsyncId, ovr->Parms.ActivityObject );

	// Register our HMT receivers if they have not already been registered.
	ovr_RegisterHmtReceivers( ovr->Jni, ovr->Parms.ActivityObject );

	// Register our receivers
	const jmethodID startReceiversId = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass,
    		"startReceivers", "(Landroid/app/Activity;)V" );
	ovr->Jni->CallStaticVoidMethod( VrLibClass, startReceiversId, ovr->Parms.ActivityObject );

	getPowerLevelStateID = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass, "getPowerLevelState", "(Landroid/app/Activity;)I" );
	setActivityWindowFullscreenID = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass, "setActivityWindowFullscreen", "(Landroid/app/Activity;)V" );
	notifyMountHandledID = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass, "notifyMountHandled", "(Landroid/app/Activity;)V" );

	// get external storage directory
	const jmethodID getExternalStorageDirectoryMethodId = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass, "getExternalStorageDirectory", "()Ljava/lang/String;" );
	jstring externalStorageDirectoryString = (jstring)ovr->Jni->CallStaticObjectMethod( VrLibClass, getExternalStorageDirectoryMethodId );
    VString externalStorageDirectory = JniUtils::Convert(ovr->Jni, externalStorageDirectoryString);
    ovr->Jni->DeleteLocalRef(externalStorageDirectoryString);

	if ( ovr->Jni->ExceptionOccurred() )
	{
		ovr->Jni->ExceptionClear();
		vInfo("Cleared JNI exception");
	}

	ovr->Warp = new VFrameSmooth( ovr->Parms.AsynchronousTimeWarp,ovr->device);

	// reset brightness, DND and comfort modes because VRSVC, while maintaining the value, does not actually enforce these settings.
	int brightness = ovr_GetSystemBrightness( ovr );
	ovr_SetSystemBrightness( ovr, brightness );

	bool dndMode = ovr_GetDoNotDisturbMode( ovr );
	ovr_SetDoNotDisturbMode( ovr, dndMode );

	bool comfortMode = ovr_GetComfortModeEnabled( ovr );
	ovr_SetComfortModeEnabled( ovr, comfortMode );

	ovrHmdInfo info = {};
	info.SuggestedEyeResolution[0] = ovr->device->eyeDisplayResolution[0];
	info.SuggestedEyeResolution[1] = ovr->device->eyeDisplayResolution[1];
	info.SuggestedEyeFov[0] = ovr->device->eyeDisplayFov[0];
	info.SuggestedEyeFov[1] = ovr->device->eyeDisplayFov[1];

	*returnedHmdInfo = info;

	/*

	PERFORMANCE WARNING

	Upon return from the platform activity, an additional full-screen
	HWC layer with the fully-qualified activity name would be active in
	the SurfaceFlinger compositor list, consuming nearly a GB/s of bandwidth.

	"adb shell dumpsys SurfaceFlinger":

	HWC			| b84364f0 | 00000002 | 00000000 | 00 | 00100 | 00000001 | [    0.0,    0.0, 1440.0, 2560.0] | [    0,    0, 1440, 2560] SurfaceView
	HWC			| b848e020 | 00000002 | 00000000 | 00 | 00105 | 00000001 | [    0.0,    0.0, 1440.0, 2560.0] | [    0,    0, 1440, 2560] com.oculusvr.vrcene/com.oculusvr.vrscene.MainActivity
	FB TARGET	| b843d6f0 | 00000000 | 00000000 | 00 | 00105 | 00000001 | [    0.0,    0.0, 1440.0, 2560.0] | [    0,    0, 1440, 2560] HWC_FRAMEBUFFER_TARGET

	The additional layer is tied to the decor view of the activity
	and is normally not visible. The decor view becomes visible because
	the SurfaceView window is no longer fullscreen after returning
	from the platform activity.

	By always setting the SurfaceView window as full screen, the decor view
	will not be rendered and won't waste any precious bandwidth.

	*/
	// TODO_PLATFORMUI - once platformactivity is in it's own apk, remove setActivityWindowFullscreen functionality.
	if ( setActivityWindowFullscreenID != NULL && !ovr->Parms.SkipWindowFullscreenReset )
	{
		ovr->Jni->CallStaticVoidMethod( VrLibClass, setActivityWindowFullscreenID, ovr->Parms.ActivityObject );
	}

	return ovr;
}

void ovr_notifyMountHandled( ovrMobile * ovr )
{
	if ( ovr == NULL )
	{
		return;
	}

	if ( notifyMountHandledID != NULL )
	{
		ovr->Jni->CallStaticVoidMethod( VrLibClass, notifyMountHandledID, ovr->Parms.ActivityObject );
	}
}

// Should be called before the window is destroyed.
void ovr_LeaveVrMode( ovrMobile * ovr )
{
	vInfo("---------- ovr_LeaveVrMode ----------");

	if ( ovr == NULL )
	{
		vWarn("NULL ovr");
		return;
	}

	if ( ovr->Destroyed )
	{
		vWarn("Skipping ovr_LeaveVrMode: ovr already Destroyed");
		return;
	}

	if ( gettid() != ovr->EnterTid )
	{
		vFatal("ovr_LeaveVrMode: Called with tid" << gettid() << "instead of" << ovr->EnterTid);
	}

	// log the application name, version, activity, build, model, etc.
	jmethodID logApplicationNameMethodId = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass, "logApplicationName", "(Landroid/app/Activity;)V" );
	ovr->Jni->CallStaticVoidMethod( VrLibClass, logApplicationNameMethodId, ovr->Parms.ActivityObject );

	jmethodID logApplicationVersionId = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass, "logApplicationVersion", "(Landroid/app/Activity;)V" );
	ovr->Jni->CallStaticVoidMethod( VrLibClass, logApplicationVersionId, ovr->Parms.ActivityObject );

    VString currentClassName = JniUtils::GetCurrentActivityName(ovr->Jni, ovr->Parms.ActivityObject);
    vInfo("ACTIVITY =" << currentClassName);

	delete ovr->Warp;
	ovr->Warp = 0;

	// This must be after pushing the textures to timewarp.
	ovr->Destroyed = true;

	getPowerLevelStateID = NULL;

	// Stop our vsync callbacks.
	const jmethodID stopVsyncId = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass,
			"stopVsync", "(Landroid/app/Activity;)V" );
	ovr->Jni->CallStaticVoidMethod( VrLibClass, stopVsyncId, ovr->Parms.ActivityObject );

	// Unregister our receivers
#if 1
	const jmethodID stopReceiversId = JniUtils::GetStaticMethodID( ovr->Jni, VrLibClass,
			"stopReceivers", "(Landroid/app/Activity;)V" );
	ovr->Jni->CallStaticVoidMethod( VrLibClass, stopReceiversId, ovr->Parms.ActivityObject );
#else
	const jmethodID stopCellularReceiverId = ovr_GetStaticMethodID( ovr->Jni, VrLibClass,
			"stopCellularReceiver", "(Landroid/app/Activity;)V" );
	ovr->Jni->CallStaticVoidMethod( VrLibClass, stopCellularReceiverId, ovr->Parms.ActivityObject );

	const jmethodID stopWifiReceiverId = ovr_GetStaticMethodID( ovr->Jni, VrLibClass,
			"stopWifiReceiver", "(Landroid/app/Activity;)V" );
	ovr->Jni->CallStaticVoidMethod( VrLibClass, stopWifiReceiverId, ovr->Parms.ActivityObject );

	const jmethodID stopVolumeReceiverId = ovr_GetStaticMethodID( ovr->Jni, VrLibClass,
			"stopVolumeReceiver", "(Landroid/app/Activity;)V" );
	ovr->Jni->CallStaticVoidMethod( VrLibClass, stopVolumeReceiverId, ovr->Parms.ActivityObject );

	const jmethodID stopBatteryReceiverId = ovr_GetStaticMethodID( ovr->Jni, VrLibClass,
			"stopBatteryReceiver", "(Landroid/app/Activity;)V" );
	ovr->Jni->CallStaticVoidMethod( VrLibClass, stopBatteryReceiverId, ovr->Parms.ActivityObject );

	const jmethodID stopHeadsetReceiverId = ovr_GetStaticMethodID( ovr->Jni, VrLibClass,
			"stopHeadsetReceiver", "(Landroid/app/Activity;)V" );
	ovr->Jni->CallStaticVoidMethod( VrLibClass, stopHeadsetReceiverId, ovr->Parms.ActivityObject );
#endif
}

void ovr_HandleHmdEvents( ovrMobile * ovr )
{
	if ( ovr == NULL )
	{
		return;
	}

	// check if the HMT has been undocked
	HMTDockState_t dockState = HMTDockState.state();
	if ( dockState.DockState == HMT_DOCK_UNDOCKED )
	{
		vInfo("ovr_HandleHmdEvents::Hmt was disconnected");

		// reset the sensor info
		if ( OvrHmdState != NULL )
		{
			OvrHmdState->resetSensor();
		}

		// reset the real dock state since we're handling the change
		HMTDockState.setState( HMTDockState_t( HMT_DOCK_NONE ) );

		// ovr_ExitActivity() with EXIT_TYPE_FINISH_AFFINITY will finish
		// all activities in the stack.
		ovr_ExitActivity( ovr, EXIT_TYPE_FINISH_AFFINITY );

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
				vInfo("ovr_HandleHmtEvents: HMT is already mounted");
			}
			else
			{
				vInfo("ovr_HandleHmdEvents: HMT was mounted");
				hmtIsMounted = true;

				// broadcast to background apps that mount has been handled
				ovr_notifyMountHandled( ovr );

				NervGear::VString reorientMessage;
                CreateSystemActivitiesCommand( "", SYSTEM_ACTIVITY_EVENT_REORIENT, "", reorientMessage );
                NervGear::SystemActivities_AddEvent( reorientMessage );
			}
		}
		else if ( mountState.MountState == HMT_MOUNT_UNMOUNTED )
		{
			vInfo("ovr_HandleHmdEvents: HMT was UNmounted");

			hmtIsMounted = false;
		}
	}
}

void ovr_HandleDeviceStateChanges( ovrMobile * ovr )
{
	if ( ovr == NULL )
	{
		return;
	}

	// Test for Hmd Events such as mount/unmount, dock/undock
	ovr_HandleHmdEvents( ovr );

	// check for pending events that must be handled natively
	size_t const MAX_EVENT_SIZE = 4096;
//	char eventBuffer[MAX_EVENT_SIZE];
    VString eventBuffer;

	for ( eVrApiEventStatus status = NervGear::SystemActivities_nextPendingInternalEvent( eventBuffer, MAX_EVENT_SIZE );
		status >= VRAPI_EVENT_PENDING;
		status = NervGear::SystemActivities_nextPendingInternalEvent( eventBuffer, MAX_EVENT_SIZE ) )
	{
		if ( status != VRAPI_EVENT_PENDING )
		{
			vWarn("Error" << status << "handing internal System Activities Event");
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
				vInfo("ovr_HandleDeviceStateChanges: Acting on System Activity reorient event.");
				ovr_RecenterYawInternal();
			}
            else if (command == SYSTEM_ACTIVITY_EVENT_RETURN_TO_LAUNCHER && platformUIVersion < 2 )
			{
				// In the case of the returnToLauncher event, we always handler it internally and pass
				// along an empty buffer so that any remaining events still get processed by the client.
				vInfo("ovr_HandleDeviceStateChanges: Acting on System Activity returnToLauncher event.");
				// PlatformActivity and Home should NEVER get one of these!
				ovr_ExitActivity(ovr, EXIT_TYPE_FINISH_AFFINITY);
            }
		}
		else
		{
			// a malformed event string was pushed! This implies an error in the native code somewhere.
            vWarn("Error parsing System Activities Event");
		}
	}
}

double ovr_GetPredictedDisplayTime( ovrMobile * ovr, int minimumVsyncs, int pipelineDepth )
{
	if ( ovr == NULL )
	{
		return ovr_GetTimeInSeconds();
	}
	if ( ovr->Destroyed )
	{
		vInfo("ovr_GetPredictedDisplayTime: Returning due to Destroyed");
		return ovr_GetTimeInSeconds();
	}
	// Handle power throttling the same way ovr_WarpSwap() does.
	const int throttledMinimumVsyncs = minimumVsyncs;
	const double vsyncBase = floor( NervGear::GetFractionalVsync() );
	const double predictedFrames = (double)throttledMinimumVsyncs * ( (double)pipelineDepth + 0.5 );
	const double predictedVsync = vsyncBase + predictedFrames;
	const double predictedTime = NervGear::FramePointTimeInSeconds( predictedVsync );
	//vInfo("synthesis: vsyncBase =" << vsyncBase << ", predicted frames =" << predictedFrames << ", predicted V-sync =" << predictedVsync << ", predicted time =" << predictedTime);
	return predictedTime;
}

ovrSensorState ovr_GetPredictedSensorState( ovrMobile * ovr, double absTime )
{
	return ovr_GetSensorStateInternal( absTime );
}

void ovr_RecenterYaw( ovrMobile * ovr )
{
	ovr_RecenterYawInternal();
}

void ovr_WarpSwap( ovrMobile * ovr, const ovrTimeWarpParms * parms )
{
	if ( ovr == NULL )
	{
		return;
	}

	if ( ovr->Warp == NULL )
	{
		return;
	}
	// If we are changing to a new activity, leave the screen black
	if ( ovr->Destroyed )
	{
		vInfo("ovr_WarpSwap: Returning due to Destroyed");
		return;
	}

	if ( gettid() != ovr->EnterTid )
	{
		vFatal("ovr_WarpSwap: Called with tid" << gettid() << "instead of" << ovr->EnterTid);
	}

	// Push the new data
	ovr->Warp->doSmooth( *parms );
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
		//vInfo("ovr_GetTimeSinceLastVolumeChange() : Not initialized.  Returning -1");
		return -1;
	}
	return ovr_GetTimeInSeconds() - value;
}

eVrApiEventStatus ovr_nextPendingEvent( VString& buffer, unsigned int const bufferSize )
{
	eVrApiEventStatus status = NervGear::SystemActivities_nextPendingMainEvent( buffer, bufferSize );
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
			vInfo("Queuing internal reorient event.");
			ovr_RecenterYawInternal();
			// also queue as an internal event
			NervGear::SystemActivities_AddInternalEvent( buffer );
        } else if (command == SYSTEM_ACTIVITY_EVENT_RETURN_TO_LAUNCHER) {
			// In the case of the returnToLauncher event, we always handler it internally and pass
			// along an empty buffer so that any remaining events still get processed by the client.
			vInfo("Queuing internal returnToLauncher event.");
			// queue as an internal event
			NervGear::SystemActivities_AddInternalEvent( buffer );
			// treat as an empty event externally
            buffer = "";
			status = VRAPI_EVENT_CONSUMED;
        }
	}
	else
	{
		// a malformed event string was pushed! This implies an error in the native code somewhere.
        vWarn("Error parsing System Activities Event");
		return VRAPI_EVENT_INVALID_JSON;
	}
	return status;
}
