#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

#ifdef NV_OS_WIN
#include "windows.h"
// A simple helper class to disable/enable system error mode, if necessary
// Disabling happens conditionally only if a drive name is involved
class SysErrorModeDisabler
{
    //BOOL    Disabled;
    //UINT    OldMode;
    bool    Disabled;
    uint    OldMode;

public:
    SysErrorModeDisabler(const char* pfileName)
    {
        if (pfileName && (pfileName[0]!=0) && pfileName[1]==':')
        {
            Disabled = 1;
            OldMode = ::SetErrorMode(SEM_FAILCRITICALERRORS);
        }
        else
            Disabled = 0;
    }

    ~SysErrorModeDisabler()
    {
        if (Disabled) ::SetErrorMode(OldMode);
    }
};
#else
class SysErrorModeDisabler
{
public:
    SysErrorModeDisabler(const char* pfileName) { }
};
#endif // NV_OS_WIN

NV_NAMESPACE_END
