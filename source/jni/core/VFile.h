#pragma once

#include "vglobal.h"

#include "RefCount.h"
#include "Alg.h"
#include "Std.h"
<<<<<<< HEAD

=======
#include "VLog.h"

#include <iostream>
#include <fstream>
>>>>>>> dev
#include <stdio.h>
#include "VString.h"

NV_NAMESPACE_BEGIN

// ***** Declared classes

class VFile;

//-----------------------------------------------------------------------------------
// ***** File Class

// The pure virtual base random-access file
// This is a base class to all files

<<<<<<< HEAD
class VFile : public RefCountBase<VFile>
=======
class VFile : public RefCountBase<VFile>,public std::fstream
>>>>>>> dev
{

// ***** Flags for File & Directory accesses

public:

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

<<<<<<< HEAD
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
=======
//    // *** Seek operations
//    enum SeekFlag
//    {
//        Seek_Set        = 0,
//        Seek_Cur        = 1,
//        Seek_End        = 2
//    };

    // *** Errors
    enum ErrorType
    {
        FileNotFound       = 0x1001,
        AccessError        = 0x1002,
        IOError            = 0x1003,
        iskFullError       = 0x1004
>>>>>>> dev
    };


public:
    VFile() { }
    // ** Location Information

    // Returns a file name path relative to the 'reference' directory
    // This is often a path that was used to create a file
    // (this is not a global path, global path can be obtained with help of directory)
    virtual const char* filePath() = 0;


    // ** File Information

    // Return 1 if file's usable (open)
    virtual bool        isValid() = 0;
    // Return 1 if file's writable, otherwise 0
    virtual bool        isWritable() = 0;

    // Return position
    virtual int         tell() = 0;
<<<<<<< HEAD
    virtual SInt64      tell64() = 0;

    // File size
    virtual int         length() = 0;
    virtual SInt64      length64() = 0;
=======
    virtual long long   tell64() = 0;

    // File size
    virtual int         length() = 0;
    virtual long long   length64() = 0;
>>>>>>> dev

    // Returns file stats
    // 0 for failure
    //virtual bool      Stat(FileStats *pfs) = 0;

    // Return errno-based error code
    // Useful if any other function failed
    virtual int         errorCode() = 0;


    // ** Stream implementation & I/O

    // Blocking write, will write in the given number of bytes to the stream
    // Returns : -1 for error
    //           Otherwise number of bytes read
<<<<<<< HEAD
    virtual int         write(const UByte *pbufer, int numBytes) = 0;
=======
    virtual int         write(const uchar *pbufer, int numBytes) = 0;
>>>>>>> dev
    // Blocking read, will read in the given number of bytes or less from the stream
    // Returns : -1 for error
    //           Otherwise number of bytes read,
    //           if 0 or < numBytes, no more bytes available; end of file or the other side of stream is closed
<<<<<<< HEAD
    virtual int         read(UByte *pbufer, int numBytes) = 0;
=======
    virtual int         read(uchar *pbufer, int numBytes) = 0;
>>>>>>> dev

    // Skips (ignores) a given # of bytes
    // Same return values as Read
    virtual int         skipBytes(int numBytes) = 0;

    // Returns the number of bytes available to read from a stream without blocking
    // For a file, this should generally be number of bytes to the end
    virtual int         bytesAvailable() = 0;

    // Causes any implementation's buffered data to be delivered to destination
    // Return 0 for error
<<<<<<< HEAD
    virtual bool        flush() = 0;
=======
    virtual bool        Flush() = 0;
>>>>>>> dev


    // Need to provide a more optimized implementation that doe snot necessarily involve a lot of seeking
    // 找不到调用该函数的文件
     inline bool         atEnd() { return !bytesAvailable(); }

<<<<<<< HEAD

    // Seeking
    // Returns new position, -1 for error
    virtual int         seek(int offset, int origin=Seek_Set) = 0;
    virtual SInt64      seek64(SInt64 offset, int origin=Seek_Set) = 0;
=======
    // Seeking
    // Returns new position, -1 for error
    virtual int         seek(int offset, std::ios_base::seekdir origin=std::ios_base::beg) = 0;
    virtual long long   seek64(long long offset, std::ios_base::seekdir origin=std::ios_base::beg) = 0;
>>>>>>> dev
    // Seek simplification
    // 找不到调用这些函数的文件
    // int                 seekToBegin()           {return seek(0); }
    // int                 seekToEnd()             {return seek(0,Seek_End); }
    // int                 skip(int numBytes)     {return seek(numBytes,Seek_Cur); }


    // Appends other file data from a stream
    // Return -1 for error, else # of bytes written
    virtual int         copyFromStream(VFile *pstream, int byteSize) = 0;

    // Closes the file
    // After close, file cannot be accessed
<<<<<<< HEAD
    virtual bool        close() = 0;
=======
    virtual bool        Close() = 0;
>>>>>>> dev
};

NV_NAMESPACE_END
