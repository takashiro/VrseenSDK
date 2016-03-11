#pragma once

#include "vglobal.h"
#include "VString.h"

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

    VLog(const char *file, uint line, Priority priority);
    ~VLog();

    VLog &operator << (char ch);
    VLog &operator << (char16_t ch);
    VLog &operator << (char32_t ch);
    VLog &operator << (short num);
    VLog &operator << (ushort num);
    VLog &operator << (int num);
    VLog &operator << (uint num);
    VLog &operator << (float num);
    VLog &operator << (double num);
    VLog &operator << (long long num);
    VLog &operator << (ulonglong num);

    VLog &operator << (const char *str);
    VLog &operator << (const VString &str);
    VLog &operator << (const VByteArray &str);

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VLog)
};

#if 1
#  define vVerbose(args) { VLog(__FILE__, __LINE__, VLog::Verbose) << args ; }
#  define vDebug(args) { VLog(__FILE__, __LINE__, VLog::Debug) << args ; }
#  define vInfo(args) { VLog(__FILE__, __LINE__, VLog::Info) << args ; }
#  define vWarn(args) { VLog(__FILE__, __LINE__, VLog::Warn) << args ; }
#  define vError(args) { VLog(__FILE__, __LINE__, VLog::Error) << args ; }
#  define vFatal(args) { VLog(__FILE__, __LINE__, VLog::Verbose) << args; __builtin_trap() ; }
#  define vAssert(expr) { if (!(expr)) { vFatal(#expr); } }
#else
#  define vVerbose(args)
#  define vDebug(args)
#  define vInfo(args)
#  define vWarn(args)
#  define vError(args)
#  define vFatal(args)
#  define vAssert(expr)
#endif

NV_NAMESPACE_END
