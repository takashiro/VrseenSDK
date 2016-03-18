#pragma once

#include "vglobal.h"

#include "Types.h"
#include <stdarg.h> // for va_list args
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// Wide-char funcs
#include <wchar.h>
#include <wctype.h>

NV_NAMESPACE_BEGIN

inline int OVR_CDECL OVR_sprintf(char *dest, uint destsize, const char* format, ...)
{
	if ( destsize <= 0 || dest == NULL )
	{
		OVR_ASSERT( destsize > 0 );
		return -1;
	}
    va_list argList;
    va_start(argList,format);
    int ret;
#if defined(OVR_CC_MSVC)
    #if defined(OVR_MSVC_SAFESTRING)
        ret = _vsnprintf_s(dest, destsize, _TRUNCATE, format, argList);
    #else
		// FIXME: this is a security issue on Windows platforms that don't have _vsnprintf_s
        OVR_UNUSED(destsize);
        ret = _vsnprintf(dest, destsize - 1, format, argList); // -1 for space for the null character
        dest[destsize-1] = 0;	// may leave trash in the destination...
    #endif
#else
    ret = vsnprintf( dest, destsize, format, argList );
	// In the event of the output string being greater than the buffer size, vsnprintf should
	// return the size of the string before truncation. In that case we zero-terminate the
	// string to ensure that the result is the same as _vsnprintf_s would return for the
	// MSVC compiler. vsnprintf is supposed to zero-terminate in this case, but to be sure
	// we zero-terminate it ourselves.
	if ( ret >= (int)destsize )
	{
		dest[destsize - 1] = '\0';
	}
#endif
	// If an error occurs, vsnprintf should return -1, in which case we set zero byte to null character.
	OVR_ASSERT( ret >= 0 );	// ensure the format string is not malformed
	if ( ret < 0 )
	{
		dest[0] ='\0';
	}
    va_end(argList);
    return ret;
}

inline uint OVR_CDECL OVR_vsprintf(char *dest, uint destsize, const char * format, va_list argList)
{
    uint ret;
#if defined(OVR_CC_MSVC)
    #if defined(OVR_MSVC_SAFESTRING)
        dest[0] = '\0';
        int rv = vsnprintf_s(dest, destsize, _TRUNCATE, format, argList);
        if (rv == -1)
        {
            dest[destsize - 1] = '\0';
            ret = destsize - 1;
        }
        else
            ret = (UPInt)rv;
    #else
        OVR_UNUSED(destsize);
        int rv = _vsnprintf(dest, destsize - 1, format, argList);
        OVR_ASSERT(rv != -1);
        ret = (UPInt)rv;
        dest[destsize-1] = 0;
    #endif
#else
    OVR_UNUSED(destsize);
    ret = (uint)vsprintf(dest, format, argList);
    OVR_ASSERT(ret < destsize);
#endif
    return ret;
}
} // OVR


