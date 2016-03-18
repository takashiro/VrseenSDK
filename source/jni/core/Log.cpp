/************************************************************************************

Filename    :   OVR_Log.cpp
Content     :   Logging support
Created     :   September 19, 2012
Notes       :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#include "Log.h"
#include "Std.h"
#include <stdarg.h>
#include <stdio.h>

#if defined(OVR_OS_WIN32)
#include <windows.h>
#elif defined(OVR_OS_ANDROID)
#include <android/log.h>
#endif

namespace NervGear {

// Global Log pointer.
Log* volatile OVR_GlobalLog = 0;

//-----------------------------------------------------------------------------------
// ***** Log Implementation

Log::~Log()
{
    // Clear out global log
    if (this == OVR_GlobalLog)
    {
        // TBD: perhaps we should ASSERT if this happens before system shutdown?
        OVR_GlobalLog = 0;
    }
}

void Log::LogMessageVarg(LogMessageType messageType, const char* fmt, va_list argList)
{
    if ((messageType & LoggingMask) == 0)
        return;
#ifndef OVR_BUILD_DEBUG
    if (IsDebugMessage(messageType))
        return;
#endif

    VString buffer;
    FormatLog(buffer, messageType, fmt, argList);
    DefaultLogOutput(messageType, buffer.toCString());
}

void NervGear::Log::LogMessage(LogMessageType messageType, const char* pfmt, ...)
{
    va_list argList;
    va_start(argList, pfmt);
    LogMessageVarg(messageType, pfmt, argList);
    va_end(argList);
}


void Log::FormatLog(VString& buffer, LogMessageType messageType,
                    const char* fmt, va_list argList)
{
    bool addNewline = true;

    switch(messageType)
    {
    case Log_Error:         buffer = "Error: ";     break;
    case Log_Debug:         buffer = "Debug: ";     break;
    case Log_Assert:        buffer = "Assert: ";    break;
    case Log_Text:       buffer = ""; addNewline = false; break;
    case Log_DebugText:  buffer = ""; addNewline = false; break;
    default:
        buffer = "";
        addNewline = false;
        break;
    }

    buffer.sprintf(fmt, argList);
    if (addNewline) {
        buffer += "\n";
    }
}


void Log::DefaultLogOutput(LogMessageType messageType, const char* formattedText)
{

#if defined(OVR_OS_WIN32)
    // Under Win32, output regular messages to console if it exists; debug window otherwise.
    static DWORD dummyMode;
    static bool  hasConsole = (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE) &&
                              (GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &dummyMode));

    if (!hasConsole || IsDebugMessage(messageType))
    {
        ::OutputDebugStringA(formattedText);
    }
    else
    {
         fputs(formattedText, stdout);
    }

#elif defined(OVR_OS_ANDROID)

    int logPriority = ANDROID_LOG_INFO;
    switch(messageType)
    {
    case Log_DebugText:
    case Log_Debug:     logPriority = ANDROID_LOG_DEBUG; break;
    case Log_Assert:
    case Log_Error:     logPriority = ANDROID_LOG_ERROR; break;
    default:
        logPriority = ANDROID_LOG_INFO;
    }

	__android_log_write(logPriority, "OVR", formattedText);

#else
    fputs(formattedText, stdout);

#endif

    // Just in case.
    OVR_UNUSED2(formattedText, messageType);
}


//static
void Log::SetGlobalLog(Log *log)
{
    OVR_GlobalLog = log;
}
//static
Log* Log::GetGlobalLog()
{
// No global log by default?
//    if (!OVR_GlobalLog)
//        OVR_GlobalLog = GetDefaultLog();
    return OVR_GlobalLog;
}

//static
Log* Log::GetDefaultLog()
{
    // Create default log pointer statically so that it can be used
    // even during startup.
    static Log defaultLog;
    return &defaultLog;
}


//-----------------------------------------------------------------------------------
// ***** Global Logging functions

#define OVR_LOG_FUNCTION_IMPL(Name)  \
    void Log##Name(const char* fmt, ...) \
    {                                                                    \
        if (OVR_GlobalLog)                                               \
        {                                                                \
            va_list argList; va_start(argList, fmt);                     \
            OVR_GlobalLog->LogMessageVarg(Log_##Name, fmt, argList);  \
            va_end(argList);                                             \
        }                                                                \
    }

OVR_LOG_FUNCTION_IMPL(Text)
OVR_LOG_FUNCTION_IMPL(Error)

#ifdef OVR_BUILD_DEBUG
OVR_LOG_FUNCTION_IMPL(DebugText)
OVR_LOG_FUNCTION_IMPL(Debug)
OVR_LOG_FUNCTION_IMPL(Assert)
#endif

} // OVR
