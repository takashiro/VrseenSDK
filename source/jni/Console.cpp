/************************************************************************************

Filename    :   Console.cpp
Content     :   Allows adb to send commands to an application.
Created     :   11/21/2014
Authors     :   Jonathan Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "Console.h"
#include "Android/JniUtils.h"
#include "Android/LogUtils.h"
#include "VArray.h"
#include "VArray.h"
#include "VString.h"			// for ReadFreq()
#include "VLog.h"
//#include "Std.h"

NV_NAMESPACE_BEGIN

class OvrConsole
{
public:
    void RegisterConsoleFunction( const VString name, consoleFn_t function )
	{
        LOG( "Registering console function '%s'", name.toCString() );
		for ( int i = 0 ; i < ConsoleFunctions.length(); ++i )
		{
            if ( ConsoleFunctions[i].GetName().compare(name) == 0 )
			{
                LOG( "OvrConsole", "Console function '%s' is already registered!!", name.toCString() );
				return;
			}
		}
        LOG( "Registered console function '%s'", name.toCString() );
		ConsoleFunctions.append( OvrConsoleFunction( name, function ) );
	}

	void UnRegisterConsoleFunctions()
	{
		ConsoleFunctions.clear();
	}

    void ExecuteConsoleFunction( long appPtr, const VString commandStr ) const
	{
        DROIDLOG( "OvrConsole", "Received console command \"%s\"", commandStr.toCString() );

        VString cmdName;
        int parms = 0;
        int cmdLen = reinterpret_cast<int>(commandStr.length());
        int spacePos = static_cast<int>(commandStr.find(' '));
        if ( (size_t)spacePos != std::string::npos && spacePos < cmdLen )
		{
            parms = spacePos + 1;
            cmdName = commandStr.substr(0, spacePos);
		}
		else
		{
            cmdName = commandStr;
		}

        LOG( "ExecuteConsoleFunction( %s, %s )", cmdName.toCString(), commandStr.substr(parms).c_str() );
		for ( int i = 0 ; i < ConsoleFunctions.length(); ++i )
		{
            LOG( "Checking console function '%s'", ConsoleFunctions[i].GetName().toCString() );
            if (ConsoleFunctions[i].GetName().compare(cmdName) == 0)
			{
                LOG( "Executing console function '%s'", cmdName.toCString() );
                ConsoleFunctions[i].Execute( reinterpret_cast< void* >( appPtr ),(char*)(commandStr.substr(parms).c_str()) );
				return;
			}
		}

        DROIDLOG( "OvrConsole", "ERROR: unknown console command '%s'", cmdName.toCString() );
	}

private:
	class OvrConsoleFunction
	{
	public:
        OvrConsoleFunction( const VString name, consoleFn_t function ) :
			Function( function )
		{
            Name = name;
		}

        const VString&	GetName() const { return Name; }
		void			Execute( void * appPtr, const char * cmd ) const { Function( appPtr, cmd ); }

	private:
        VString Name;
		consoleFn_t		Function;
	};

	VArray< OvrConsoleFunction >	ConsoleFunctions;
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

JNIEXPORT void Java_com_vrseen_nervgear_ConsoleReceiver_nativeConsoleCommand( JNIEnv * jni, jclass clazz, jlong appPtr, jstring command )
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
