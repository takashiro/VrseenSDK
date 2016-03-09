#pragma once

#include "vglobal.h"

#include "RefCount.h"
#include "Alg.h"
#include "Std.h"

#include <stdio.h>
#include "VString.h"

NV_NAMESPACE_BEGIN

// ***** Declared classes

class VFile;

//-----------------------------------------------------------------------------------
// ***** File Class

// The pure virtual base random-access file
// This is a base class to all files

class VFile : public RefCountBase<VFile>
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
    virtual SInt64      tell64() = 0;

    // File size
    virtual int         length() = 0;
    virtual SInt64      length64() = 0;

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
    virtual int         write(const UByte *pbufer, int numBytes) = 0;
    // Blocking read, will read in the given number of bytes or less from the stream
    // Returns : -1 for error
    //           Otherwise number of bytes read,
    //           if 0 or < numBytes, no more bytes available; end of file or the other side of stream is closed
    virtual int         read(UByte *pbufer, int numBytes) = 0;

    // Skips (ignores) a given # of bytes
    // Same return values as Read
    virtual int         skipBytes(int numBytes) = 0;

    // Returns the number of bytes available to read from a stream without blocking
    // For a file, this should generally be number of bytes to the end
    virtual int         bytesAvailable() = 0;

    // Causes any implementation's buffered data to be delivered to destination
    // Return 0 for error
    virtual bool        flush() = 0;


    // Need to provide a more optimized implementation that doe snot necessarily involve a lot of seeking
    // 找不到调用该函数的文件
     inline bool         atEnd() { return !bytesAvailable(); }


    // Seeking
    // Returns new position, -1 for error
    virtual int         seek(int offset, int origin=Seek_Set) = 0;
    virtual SInt64      seek64(SInt64 offset, int origin=Seek_Set) = 0;
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
    virtual bool        close() = 0;


    // ***** Inlines for convenient primitive type serialization

    // Read/Write helpers
//private:
//    UInt64  _read64()           { UInt64 v = 0; read((UByte*)&v, 8); return v; }
//    UInt32  _read32()           { UInt32 v = 0; read((UByte*)&v, 4); return v; }
//    UInt16  _read16()           { UInt16 v = 0; read((UByte*)&v, 2); return v; }
//    UByte   _read8()            { UByte  v = 0; read((UByte*)&v, 1); return v; }
//    void    _write64(UInt64 v)  { write((UByte*)&v, 8); }
//    void    _write32(UInt32 v)  { write((UByte*)&v, 4); }
//    void    _write16(UInt16 v)  { write((UByte*)&v, 2); }
//    void    _write8(UByte v)    { write((UByte*)&v, 1); }

//public:

//    // Writing primitive types - Little Endian
//    inline void    writeUByte(UByte v)         { _write8((UByte)Alg::ByteUtil::SystemToLE(v));     }
//    inline void    writeSByte(SByte v)         { _write8((UByte)Alg::ByteUtil::SystemToLE(v));     }
//    inline void    writeUInt8(UByte v)         { _write8((UByte)Alg::ByteUtil::SystemToLE(v));     }
//    inline void    writeSInt8(SByte v)         { _write8((UByte)Alg::ByteUtil::SystemToLE(v));     }
//    inline void    writeUInt16(UInt16 v)       { _write16((UInt16)Alg::ByteUtil::SystemToLE(v));   }
//    inline void    writeSInt16(SInt16 v)       { _write16((UInt16)Alg::ByteUtil::SystemToLE(v));   }
//    inline void    writeUInt32(UInt32 v)       { _write32((UInt32)Alg::ByteUtil::SystemToLE(v));   }
//    inline void    writeSInt32(SInt32 v)       { _write32((UInt32)Alg::ByteUtil::SystemToLE(v));   }
//    inline void    writeUInt64(UInt64 v)       { _write64((UInt64)Alg::ByteUtil::SystemToLE(v));   }
//    inline void    writeSInt64(SInt64 v)       { _write64((UInt64)Alg::ByteUtil::SystemToLE(v));   }
//    inline void    writeFloat(float v)         { v = Alg::ByteUtil::SystemToLE(v); write((UByte*)&v, 4); }
//    inline void    writeDouble(double v)       { v = Alg::ByteUtil::SystemToLE(v); write((UByte*)&v, 8); }
//    // Writing primitive types - Big Endian
//    inline void    writeUByteBE(UByte v)       { _write8((UByte)Alg::ByteUtil::SystemToBE(v));     }
//    inline void    writeSByteBE(SByte v)       { _write8((UByte)Alg::ByteUtil::SystemToBE(v));     }
//    inline void    writeUInt8BE(UInt16 v)      { _write8((UByte)Alg::ByteUtil::SystemToBE(v));     }
//    inline void    writeSInt8BE(SInt16 v)      { _write8((UByte)Alg::ByteUtil::SystemToBE(v));     }
//    inline void    writeUInt16BE(UInt16 v)     { _write16((UInt16)Alg::ByteUtil::SystemToBE(v));   }
//    inline void    writeSInt16BE(UInt16 v)     { _write16((UInt16)Alg::ByteUtil::SystemToBE(v));   }
//    inline void    writeUInt32BE(UInt32 v)     { _write32((UInt32)Alg::ByteUtil::SystemToBE(v));   }
//    inline void    writeSInt32BE(UInt32 v)     { _write32((UInt32)Alg::ByteUtil::SystemToBE(v));   }
//    inline void    writeUInt64BE(UInt64 v)     { _write64((UInt64)Alg::ByteUtil::SystemToBE(v));   }
//    inline void    writeSInt64BE(UInt64 v)     { _write64((UInt64)Alg::ByteUtil::SystemToBE(v));   }
//    inline void    writeFloatBE(float v)       { v = Alg::ByteUtil::SystemToBE(v); write((UByte*)&v, 4); }
//    inline void    writeDoubleBE(double v)     { v = Alg::ByteUtil::SystemToBE(v); write((UByte*)&v, 8); }

//    // Reading primitive types - Little Endian
//    inline UByte   readUByte()                 { return (UByte)Alg::ByteUtil::LEToSystem(_read8());    }
//    inline SByte   readSByte()                 { return (SByte)Alg::ByteUtil::LEToSystem(_read8());    }
//    inline UByte   readUInt8()                 { return (UByte)Alg::ByteUtil::LEToSystem(_read8());    }
//    inline SByte   readSInt8()                 { return (SByte)Alg::ByteUtil::LEToSystem(_read8());    }
//    inline UInt16  readUInt16()                { return (UInt16)Alg::ByteUtil::LEToSystem(_read16());  }
//    inline SInt16  readSInt16()                { return (SInt16)Alg::ByteUtil::LEToSystem(_read16());  }
//    inline UInt32  readUInt32()                { return (UInt32)Alg::ByteUtil::LEToSystem(_read32());  }
//    inline SInt32  readSInt32()                { return (SInt32)Alg::ByteUtil::LEToSystem(_read32());  }
//    inline UInt64  readUInt64()                { return (UInt64)Alg::ByteUtil::LEToSystem(_read64());  }
//    inline SInt64  readSInt64()                { return (SInt64)Alg::ByteUtil::LEToSystem(_read64());  }
//    inline float   readFloat()                 { float v = 0.0f; read((UByte*)&v, 4); return Alg::ByteUtil::LEToSystem(v); }
//    inline double  readDouble()                { double v = 0.0; read((UByte*)&v, 8); return Alg::ByteUtil::LEToSystem(v); }
//    // Reading primitive types - Big Endian
//    inline UByte   readUByteBE()               { return (UByte)Alg::ByteUtil::BEToSystem(_read8());    }
//    inline SByte   readSByteBE()               { return (SByte)Alg::ByteUtil::BEToSystem(_read8());    }
//    inline UByte   readUInt8BE()               { return (UByte)Alg::ByteUtil::BEToSystem(_read8());    }
//    inline SByte   readSInt8BE()               { return (SByte)Alg::ByteUtil::BEToSystem(_read8());    }
//    inline UInt16  readUInt16BE()              { return (UInt16)Alg::ByteUtil::BEToSystem(_read16());  }
//    inline SInt16  readSInt16BE()              { return (SInt16)Alg::ByteUtil::BEToSystem(_read16());  }
//    inline UInt32  readUInt32BE()              { return (UInt32)Alg::ByteUtil::BEToSystem(_read32());  }
//    inline SInt32  readSInt32BE()              { return (SInt32)Alg::ByteUtil::BEToSystem(_read32());  }
//    inline UInt64  readUInt64BE()              { return (UInt64)Alg::ByteUtil::BEToSystem(_read64());  }
//    inline SInt64  readSInt64BE()              { return (SInt64)Alg::ByteUtil::BEToSystem(_read64());  }
//    inline float   readFloatBE()               { float v = 0.0f; read((UByte*)&v, 4); return Alg::ByteUtil::BEToSystem(v); }
//    inline double  readDoubleBE()              { double v = 0.0; read((UByte*)&v, 8); return Alg::ByteUtil::BEToSystem(v); }
};

NV_NAMESPACE_END
