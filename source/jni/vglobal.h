#pragma once

#ifdef NV_NAMESPACE
# define NV_NAMESPACE_BEGIN namespace NV_NAMESPACE {
# define NV_NAMESPACE_END }
# define NV_USING_NAMESPACE using namespace NV_NAMESPACE;
#else
# define NV_NAMESPACE_BEGIN
# define NV_NAMESPACE_END
# define NV_USING_NAMESPACE
#endif

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;
typedef long long longlong;
typedef unsigned long long ulonglong;

#ifndef NV_FLOAT_AS_REAL
typedef double vreal;
#else
typedef float vreal;
#endif

//@to-do: Check target platform and compiler
typedef signed char vint8;
typedef uchar vuint8;
typedef short vint16;
typedef ushort vuint16;
typedef int vint32;
typedef uint vuint32;
typedef longlong vint64;
typedef ulonglong vuint64;

#define forever for(;;)

#define NV_DECLARE_PRIVATE struct Private; friend struct Private; Private *d;
#define NV_DISABLE_COPY(name) void operator=(const name &) = delete; name(const name &source) = delete;


//Operating System Macros
#if (defined(__APPLE__) && (defined(__GNUC__) || defined(__xlC__) || defined(__xlc__))) || defined(__MACOS__)
#  if (defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__) || defined(__IPHONE_OS_VERSION_MIN_REQUIRED))
#    define NV_OS_IPHONE
#  else
#    define NV_OS_MAC
#  endif
#elif (defined(WIN64) || defined(_WIN64) || defined(__WIN64__))
#  define NV_OS_WIN
#elif (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#  define NV_OS_WIN
#elif defined(__linux__) || defined(__linux)
#  define NV_OS_LINUX
#  if defined(ANDROID)
#    define NV_OS_ANDROID
#  endif
#endif

//Surpress unused variable warnings
#define __NV_UNUSED1(a) (void)(a)
#define __NV_UNUSED2(a,b) (void)(a),(void)(b)
#define __NV_UNUSED3(a,b,c) (void)(a),(void)(b),(void)(c)
#define __NV_UNUSED4(a,b,c,d) (void)(a),(void)(b),(void)(c),(void)(d)
#define __NV_UNUSED5(a,b,c,d,e) (void)(a),(void)(b),(void)(c),(void)(d),(void)(e)

#define __VA_NUM_ARGS_IMPL(_1,_2,_3,_4,_5, N,...) N
#define VA_NUM_ARGS(...) __VA_NUM_ARGS_IMPL(__VA_ARGS__, 5, 4, 3, 2, 1)

#define __NV_ALL_UNUSED_IMPL_(nargs) __NV_UNUSED ## nargs
#define __NV_ALL_UNUSED_IMPL(nargs) __NV_ALL_UNUSED_IMPL_(nargs)
#define NV_UNUSED(...) __NV_ALL_UNUSED_IMPL( VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__ )

//Version Macros
#define NV_MAJOR_VERSION 0
#define NV_MINOR_VERSION 0
#define NV_BUILD_VERSION 0
#define NV_PATCH_VERSION 0

#define NV_VERSION_CHECK(major, minor, build, patch) ((major<<24)|(minor<<16)|(build<<8)|patch)
#define NV_VERSION NV_VERSION_CHECK(NV_MAJOR_VERSION, NV_MINOR_VERSION, NV_BUILD_VERSION, NV_PATCH_VERSION)
