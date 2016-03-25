#pragma once

#include <VString.h>


extern "C" {

// While in VrMode, we get headset plugged/unplugged updates from Android.
bool ovr_GetHeadsetPluggedState();

// While in VrMode, we get volume updates from Android.
int ovr_GetVolume();
double ovr_GetTimeSinceLastVolumeChange();

//-----------------------------------------------------------------
// Activity start/exit
//-----------------------------------------------------------------


void ovr_ExitActivity( ovrMobile * ovr, eExitType type );
void ovr_ReturnToHome( ovrMobile * ovr );

void ovr_SendIntent( ovrMobile * ovr, const char * actionName, const char * toPackageName,
		const char * toClassName, const char * command, const char * uri, eExitType exitType );
void ovr_SendLaunchIntent( ovrMobile * ovr, const char * toPackageName, const char * command,
		const char * uri, eExitType exitType );
bool ovr_StartSystemActivity( ovrMobile * ovr, const char * command, const char * jsonText );
// fills outBuffer with a JSON text object with the required versioning info, the passed command, and embedded extraJsonText.
bool ovr_CreateSystemActivityIntent( ovrMobile * ovr, const char * command, const char * extraJsonText,
		char * outBuffer, unsigned long long const outBufferSize, unsigned long long & outRequiredBufferSize );

//-----------------------------------------------------------------
// Activity start/exit
//-----------------------------------------------------------------

// This must match the value declared in ProximityReceiver.java / SystemActivityReceiver.java
#define SYSTEM_ACTIVITY_INTENT "com.oculus.system_activity"
#define	SYSTEM_ACTIVITY_EVENT_REORIENT "reorient"
#define SYSTEM_ACTIVITY_EVENT_RETURN_TO_LAUNCHER "returnToLauncher"
#define SYSTEM_ACTIVITY_EVENT_EXIT_TO_HOME "exitToHome"

// return values for ovr_nextPendingEvent
enum eVrApiEventStatus
{
	VRAPI_EVENT_ERROR_INTERNAL = -2,		// queue isn't created, etc.
	VRAPI_EVENT_ERROR_INVALID_BUFFER = -1,	// the buffer passed in was invalid
	VRAPI_EVENT_NOT_PENDING = 0,			// no event is waiting
	VRAPI_EVENT_PENDING,					// an event is waiting
	VRAPI_EVENT_CONSUMED,					// an event was pending but was consumed internally
	VRAPI_EVENT_BUFFER_OVERFLOW,			// an event is being returned, but it could not fit into the buffer
	VRAPI_EVENT_INVALID_JSON				// there was an error parsing the JSON data
};

eVrApiEventStatus ovr_nextPendingEvent( NervGear::VString& buffer, unsigned int const bufferSize );
}	// extern "C"


