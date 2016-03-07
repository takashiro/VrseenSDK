#pragma once

#include "vglobal.h"
#include "VString.h"

#include <sstream>

NV_NAMESPACE_BEGIN

class VLog
{
public:
    enum Priority {
        Unknown,
        Default,

        Verbose,
        Debug,
        Info,
        Warn,
        Error,
        Fatal,

        Silent,
    };

    VLog(Priority priority);
    ~VLog();

    VLog &operator << (short num);
    VLog &operator << (ushort num);
    VLog &operator << (int num);
    VLog &operator << (uint num);
    VLog &operator << (float num);
    VLog &operator << (double num);

    VLog &operator << (const char *str);
    VLog &operator << (const VString &str);
    VLog &operator << (const VByteArray &str);

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VLog)
};

#if 1
#  define vVerbose(args) { VLog(VLog::Verbose) << args ; }
#  define vDebug(args) { VLog(VLog::Debug) << args ; }
#  define vInfo(args) { VLog(VLog::Info) << args ; }
#  define vWarn(args) { VLog(VLog::Warn) << args ; }
#  define vError(args) { VLog(VLog::Error) << args ; }
#  define vFatal(args) { VLog(VLog::Verbose) << args; __builtin_trap() ; }
#else
#  define vVerbose(args)
#  define vDebug(args)
#  define vInfo(args)
#  define vWarn(args)
#  define vError(args)
#  define vFatal(args)
#endif

NV_NAMESPACE_END
