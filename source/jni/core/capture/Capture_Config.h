#pragma once

#include "vglobal.h"

#if defined(_WIN32) || defined(_WIN64)
    #define OVR_CAPTURE_WINDOWS
#elif defined(__APPLE__)
    #define OVR_CAPTURE_DARWIN
    #define OVR_CAPTURE_POSIX
    #define OVR_CAPTURE_HAS_MACH_ABSOLUTE_TIME
  //#define OVR_CAPTURE_HAS_GETTIMEOFDAY
#elif defined(__ANDROID__)
    #define OVR_CAPTURE_ANDROID
    #define OVR_CAPTURE_POSIX
    #define OVR_CAPTURE_HAS_CLOCK_GETTIME
  //#define OVR_CAPTURE_HAS_GETTIMEOFDAY
    #define OVR_CAPTURE_HAS_GLES3
    #define OVR_CAPTURE_HAS_ATRACE
#elif defined(__linux__)
    #define OVR_CAPTURE_LINUX
    #define OVR_CAPTURE_POSIX
    #define OVR_CAPTURE_HAS_CLOCK_GETTIME
  //#define OVR_CAPTURE_HAS_GETTIMEOFDAY
#else
    #error Unknown Platform!
#endif

// We use zlib for crc32() to generate Label name hashes
// the path without zlib though is nearly as good at generating an event bit distribution
//#define OVR_CAPTURE_HAS_ZLIB

// Runtime Assert support...
#if defined(OVR_CAPTURE_POSIX)
    #include <assert.h>
    #define OVR_CAPTURE_ASSERT(exp) assert(exp)
#endif

// Compile Time Assert support...
#if defined(__clang__)
    #if __has_feature(cxx_static_assert) || __has_extension(cxx_static_assert)
        #define OVR_CAPTURE_STATIC_ASSERT(exp) static_assert(exp)
    #endif
#endif
#if !defined(OVR_CAPTURE_STATIC_ASSERT)
    NV_NAMESPACE_BEGIN
    namespace Capture
    {
        template<bool exp> struct StaticCondition;
        template<>         struct StaticCondition<true>{ static const int value=1; };
    }
    NV_NAMESPACE_END
    #define OVR_CAPTURE_STATIC_ASSERT__(exp,id) enum { OVRCAPTURESTATICASSERT##id = NervGear::Capture::StaticCondition<exp>::value };
    #define OVR_CAPTURE_STATIC_ASSERT_(exp,id)  OVR_CAPTURE_STATIC_ASSERT__(exp, id)
    #define OVR_CAPTURE_STATIC_ASSERT(exp)      OVR_CAPTURE_STATIC_ASSERT_(exp, __COUNTER__)
#endif
