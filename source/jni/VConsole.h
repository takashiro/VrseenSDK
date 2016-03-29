/*
 * VConsole.h
 *
 *  Created on: 2016年3月21日
 *      Author: gaojialing
 */

#include "vglobal.h"

#include "VString.h"
#include "VArray.h"

NV_NAMESPACE_BEGIN

class VConsole
{
public:
    typedef void (*Function)(void * appPtr, const char *cmd);

    const VString &name() const;

    static typename NervGear::VConsole * Instantialize();
    static void DestoryVConsole();
    VConsole(const char * name, Function function);
    static void RegisterConsole( const char * name, Function function );
    static void UnRegisterConsole();
    void Execute( void * appPtr, const char * cmd ) const;
    void ExecuteConsole( long appPtr, char const * commandStr ) const;
    static void DebugPrint( void * appPtr, const char * cmd );

private:
    VConsole();

    Function m_function;
    VString m_name;

    static VConsole *pInstance;
    static VArray<VConsole> VConsoleFunctions;
};

NV_NAMESPACE_END
