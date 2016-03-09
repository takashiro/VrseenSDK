/************************************************************************************

Filename    :   Console.cpp
Content     :   Allows adb to send commands to an application.
Created     :   11/21/2014
Authors     :   Jonathan Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "Console.h"
#include "Std.h"
#include "Android/JniUtils.h"
#include "Android/LogUtils.h"
#include "Array.h"
#include "VString.h"			// for ReadFreq()
#include "VLog.h"

NV_NAMESPACE_BEGIN

class OvrConsole
{
public:
	void RegisterConsoleFunction( const char * name, consoleFn_t function )
	{
		//LOG( "Registering console function '%s'", name );
		for ( int i = 0 ; i < ConsoleFunctions.sizeInt(); ++i )
		{
			if ( OVR_stricmp( ConsoleFunctions[i].GetName(), name ) == 0 )
			{
				LOG( "OvrConsole", "Console function '%s' is already registered!!", name );
				OVR_ASSERT( false );	// why are you registering the same function twice??
				return;
			}
		}
		LOG( "Registered console function '%s'", name );
		ConsoleFunctions.append( OvrConsoleFunction( name, function ) );
	}

	void UnRegisterConsoleFunctions()
	{
		ConsoleFunctions.clearAndRelease();
	}

	void ExecuteConsoleFunction( long appPtr, char const * commandStr ) const
	{
		DROIDLOG( "OvrConsole", "Received console command \"%s\"", commandStr );

		char cmdName[128];
		char const * parms = "";
		int cmdLen = (int)strlen( commandStr );
		char const * spacePtr = strstr( commandStr, " " );
		if ( spacePtr != NULL && spacePtr - commandStr < cmdLen )
		{
			parms = spacePtr + 1;
			OVR_strncpy( cmdName, sizeof( cmdName ), commandStr, spacePtr - commandStr );
		}
		else
		{
			OVR_strcpy( cmdName, sizeof( cmdName ), commandStr );
		}

		LOG( "ExecuteConsoleFunction( %s, %s )", cmdName, parms );
		for ( int i = 0 ; i < ConsoleFunctions.sizeInt(); ++i )
		{
			LOG( "Checking console function '%s'", ConsoleFunctions[i].GetName() );
			if ( OVR_stricmp( ConsoleFunctions[i].GetName(), cmdName ) == 0 )
			{
				LOG( "Executing console function '%s'", cmdName );
				ConsoleFunctions[i].Execute( reinterpret_cast< void* >( appPtr ), parms );
				return;
			}
		}

		DROIDLOG( "OvrConsole", "ERROR: unknown console command '%s'", cmdName );
	}

private:
	class OvrConsoleFunction
	{
	public:
		OvrConsoleFunction( const char * name, consoleFn_t function ) :
			Function( function )
		{
			NervGear::OVR_strcpy( Name, sizeof( Name ), name );
		}

		const char *	GetName() const { return Name; }
		void			Execute( void * appPtr, const char * cmd ) const { Function( appPtr, cmd ); }

	private:
		char			Name[64];		// not an NervGear::String because this can be freed after the OVR heap has been destroyed.
		consoleFn_t		Function;
	};

	Array< OvrConsoleFunction >	ConsoleFunctions;
};

OvrConsole * Console = NULL;

void InitConsole()
{
	Console = new OvrConsole;
}

void ShutdownConsole()
{
	delete Console;
	Console = NULL;
}

// add a pointer to a function that can be executed from the console
void RegisterConsoleFunction( const char * name, consoleFn_t function )
{
	Console->RegisterConsoleFunction( name, function );
}

void UnRegisterConsoleFunctions()
{
	Console->UnRegisterConsoleFunctions();
}

void DebugPrint( void * appPtr, const char * cmd ) {
	DROIDLOG( "OvrDebug", "%s", cmd );
}

NV_NAMESPACE_END

NV_USING_NAMESPACE

extern "C" {

JNIEXPORT void Java_me_takashiro_nervgear_ConsoleReceiver_nativeConsoleCommand( JNIEnv * jni, jclass clazz, jlong appPtr, jstring command )
{
    VString commandStr = JniUtils::Convert(jni, command);
    vInfo("nativeConsoleCommand:" << commandStr);
    if (NervGear::Console != NULL ) {
        VByteArray utf8 = commandStr.toUtf8();
        NervGear::Console->ExecuteConsoleFunction(appPtr, utf8.data());
    } else {
        vInfo("Tried to execute console function without a console!");
    }
}

}