/*
 * VConsole.cpp
 *
 *  Created on: 2016年3月21日
 *      Author: gaojialing
 */
#include <VConsole.h>
#include <iostream>
#include "Android/JniUtils.h"
#include "Android/LogUtils.h"
#include <VLog.h>
#include <VArray.h>
#include <string.h>
#include <assert.h>
using namespace std;
namespace NervGear {
/*VConsole::vconsoleFn_t VConsole::Function;
VArray< VConsole >    VConsole::VConsoleFunctions;
char    VConsole::Name[64];*/
VConsole * VConsole = nullptr;
typename NervGear::VConsole *VConsole::pInstance = nullptr;
VArray<typename NervGear::VConsole>    VConsole::VConsoleFunctions;
VConsole::VConsole(){

}
const char *    VConsole::GetName() const
{
    return Name;
}
void    VConsole::Execute( void * appPtr, const char * cmd ) const
{
    Function( appPtr, cmd );
}
VConsole::VConsole( const char * name, vconsoleFn_t function ) :
    Function( function )
{
    strcpy( Name, name );
}
typename NervGear::VConsole* VConsole::Instantialize() {
    if (VConsole::pInstance == NULL) {
        VConsole::pInstance = new VConsole();
    }
    return VConsole::pInstance;
}
void VConsole::DestoryVConsole() {
    if (VConsole::pInstance != NULL) {
        delete VConsole::pInstance;
        VConsole::pInstance = NULL;
    }
}
void VConsole::RegisterConsole( const char * name, VConsole::vconsoleFn_t function )
{
for ( int i = 0 ; i < VConsoleFunctions.length(); ++i )
        {
            if ( strcmp( VConsoleFunctions[i].GetName(), name ) == 0 )
            {
                LOG( "OvrConsole", "Console function '%s' is already registered!!", name );
                assert( false );    // why are you registering the same function twice??
                return;
            }
        }
        LOG( "Registered console function '%s'", name );
        VConsoleFunctions.append( VConsole( name, function ) );

}
void VConsole::UnRegisterConsole()
{
    VConsoleFunctions.clear();
}
void VConsole::ExecuteConsole( long appPtr, char const * commandStr ) const
    {
        DROIDLOG( "OvrConsole", "Received console command \"%s\"", commandStr );

        char cmdName[128];
        char const * parms = "";
        int cmdLen = (int)strlen( commandStr );
        char const * spacePtr = strstr( commandStr, " " );
        if ( spacePtr != NULL && spacePtr - commandStr < cmdLen )
        {
            parms = spacePtr + 1;
            strncpy( cmdName, commandStr, sizeof( cmdName ));
        }
        else
        {
            strcpy( cmdName, commandStr );
        }

        LOG( "ExecuteConsoleFunction( %s, %s )", cmdName, parms );
        for ( int i = 0 ; i < VConsoleFunctions.length(); ++i )
        {
            LOG( "Checking console function '%s'", VConsoleFunctions[i].GetName() );
            if ( strcmp( VConsoleFunctions[i].GetName(), cmdName ) == 0 )
            {
                LOG( "Executing console function '%s'", cmdName );
                VConsoleFunctions[i].Execute( reinterpret_cast< void* >( appPtr ), parms );
                return;
            }
        }

        DROIDLOG( "OvrConsole", "ERROR: unknown console command '%s'", cmdName );
}
void VConsole::DebugPrint( void * appPtr, const char * cmd ) {
    DROIDLOG( "OvrDebug", "%s", cmd );
}
NV_USING_NAMESPACE

extern "C" {

JNIEXPORT void Java_com_vrseen_nervgear_ConsoleReceiver_nativeConsoleCommand( JNIEnv * jni, jclass clazz, jlong appPtr, jstring command )
{
    VString commandStr = JniUtils::Convert(jni, command);
    vInfo("nativeConsoleCommand:" << commandStr);
    if (NervGear::VConsole != nullptr ) {
        VByteArray utf8 = commandStr.toUtf8();
        NervGear::VConsole->ExecuteConsole(appPtr, utf8.data());
    } else {
        vInfo("Tried to execute console function without a console!");
    }
}

}
}
