#pragma once

#include "vglobal.h"

#include "Capture_Config.h"

// Using POSIX file handles improves perf by about 2x over stdio's fopen/fread/fclose...
#define OVR_CAPTURE_USE_POSIX_FILES 1

#if OVR_CAPTURE_USE_POSIX_FILES
    #include <fcntl.h>  // for posix IO
    #include <stdlib.h> // for atoi
#else
    #include <stdio.h>  // for std IO
#endif


#pragma once

NV_NAMESPACE_BEGIN
namespace Capture
{

#if OVR_CAPTURE_USE_POSIX_FILES
    typedef int FileHandle;
    static const FileHandle NullFileHandle = -1;
#else
    typedef FILE* FileHandle;
    static const FileHandle NullFileHandle = NULL;
#endif

    FileHandle OpenFile(const char *path, bool writable=false);
    void       CloseFile(FileHandle file);
    int        ReadFile(FileHandle file, void *buf, int bufsize);
    int        WriteFile(FileHandle file, const void *buf, int bufsize);

    bool       CheckFileExists(const char *path);

    int        ReadFileLine(const char *path, char *buf, int bufsize);

    int        ReadIntFile(FileHandle file);
    int        ReadIntFile(const char *path);

    void       WriteIntFile(FileHandle  file, int value);
    void       WriteIntFile(const char *path, int value);

} // namespace Capture
NV_NAMESPACE_END

