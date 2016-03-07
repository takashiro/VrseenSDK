#pragma once

#include "vglobal.h"

#include "Std.h"
#include "Alg.h"

NV_NAMESPACE_BEGIN

// ***** Declared classes
class VFileFlags;


// ***** Flags for File & Directory accesses
class VFileFlags
{
    // *** File open flags
    enum OpenFlags
    {
        Open_Read       = 1,
        Open_Write      = 2,
        Open_ReadWrite  = 3,

        // Opens file and truncates it to zero length
        // - file must have write permission
        // - when used with Create, it opens an existing
        //   file and empties it or creates a new file
        Open_Truncate   = 4,

        // Creates and opens new file
        // - does not erase contents if file already
        //   exists unless combined with Truncate
        Open_Create     = 8,

         // Returns an error value if the file already exists
        Open_CreateOnly = 24,

        // Open file with buffering
        Open_Buffered    = 32
    };

    // *** File Mode flags
    enum OpenMode
    {
        ReadOnly       = 0444,
        WriteOnly      = 0222,
        ExecuteMode    = 0111,

        ReadWrite  = 0666
    };

    // *** Seek operations
    enum SeekOperation
    {
        Seek_Set        = 0,
        Seek_Cur        = 1,
        Seek_End        = 2
    };

    // *** Errors
    enum Error
    {
        FileNotFoundError  = 0x1001,
        AccessError        = 0x1002,
        IOError       = 0x1003,
        iskFullError      = 0x1004
    };
};

NV_NAMESPACE_END


