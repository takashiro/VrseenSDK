
/*
 * VConsole.h
 *
 *  Created on: 2016年3月21日
 *      Author: gaojialing
 */

#include "vglobal.h"
#include "VArray.h"

NV_NAMESPACE_BEGIN
class VConsole{
public:
    typedef void (*vconsoleFn_t)( void * appPtr, const char * cmd );
private:
    VConsole();
    vconsoleFn_t Function;

    char    Name[64];
public:
    static VConsole *pInstance;
    static typename NervGear::VConsole * Instantialize();
    static void DestoryVConsole();
    const char *    GetName() const;
    VConsole(const char * name, vconsoleFn_t function);
    static VArray< VConsole >    VConsoleFunctions;
    static void RegisterConsole( const char * name, vconsoleFn_t function );
    static void UnRegisterConsole();
    void Execute( void * appPtr, const char * cmd ) const;
    void ExecuteConsole( long appPtr, char const * commandStr ) const;
    static void DebugPrint( void * appPtr, const char * cmd );
};




NV_NAMESPACE_END
