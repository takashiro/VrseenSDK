/************************************************************************************

PublicHeader:   Kernel
Filename    :   OVR_File.h
Content     :   Header for all internal file management - functions and structures
                to be inherited by OS specific subclasses.
Created     :   September 19, 2012
Notes       :

Notes       :   errno may not be preserved across use of BaseFile member functions
            :   Directories cannot be deleted while files opened from them are in use
                (For the GetFullName function)

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#ifndef OVR_File_h
#define OVR_File_h

#include "RefCount.h"
#include "Std.h"
#include "Alg.h"

#include <stdio.h>
#include "VString.h"

namespace NervGear {

// ***** Declared classes
class   FileConstants;
class   File;
class   DelegatedFile;
class   BufferedFile;


// ***** Flags for File & Directory accesses

class FileConstants
{
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
};


//-----------------------------------------------------------------------------------
// ***** File Class

// The pure virtual base random-access file
// This is a base class to all files

class File : public RefCountBase<File>, public FileConstants
{
public:
    File() { }
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
    inline bool         atEnd() { return !bytesAvailable(); }


    // Seeking
    // Returns new position, -1 for error
    virtual int         seek(int offset, int origin=Seek_Set) = 0;
    virtual SInt64      seek64(SInt64 offset, int origin=Seek_Set) = 0;
    // Seek simplification
    int                 seekToBegin()           {return seek(0); }
    int                 seekToEnd()             {return seek(0,Seek_End); }
    int                 skip(int numBytes)     {return seek(numBytes,Seek_Cur); }


    // Appends other file data from a stream
    // Return -1 for error, else # of bytes written
    virtual int         copyFromStream(File *pstream, int byteSize) = 0;

    // Closes the file
    // After close, file cannot be accessed
    virtual bool        close() = 0;


    // ***** Inlines for convenient primitive type serialization

    // Read/Write helpers
private:
    UInt64  _read64()           { UInt64 v = 0; read((UByte*)&v, 8); return v; }
    UInt32  _read32()           { UInt32 v = 0; read((UByte*)&v, 4); return v; }
    UInt16  _read16()           { UInt16 v = 0; read((UByte*)&v, 2); return v; }
    UByte   _read8()            { UByte  v = 0; read((UByte*)&v, 1); return v; }
    void    _write64(UInt64 v)  { write((UByte*)&v, 8); }
    void    _write32(UInt32 v)  { write((UByte*)&v, 4); }
    void    _write16(UInt16 v)  { write((UByte*)&v, 2); }
    void    _write8(UByte v)    { write((UByte*)&v, 1); }

public:

    // Writing primitive types - Little Endian
    inline void    writeUByte(UByte v)         { _write8((UByte)Alg::ByteUtil::SystemToLE(v));     }
    inline void    writeSByte(SByte v)         { _write8((UByte)Alg::ByteUtil::SystemToLE(v));     }
    inline void    writeUInt8(UByte v)         { _write8((UByte)Alg::ByteUtil::SystemToLE(v));     }
    inline void    writeSInt8(SByte v)         { _write8((UByte)Alg::ByteUtil::SystemToLE(v));     }
    inline void    writeUInt16(UInt16 v)       { _write16((UInt16)Alg::ByteUtil::SystemToLE(v));   }
    inline void    writeSInt16(SInt16 v)       { _write16((UInt16)Alg::ByteUtil::SystemToLE(v));   }
    inline void    writeUInt32(UInt32 v)       { _write32((UInt32)Alg::ByteUtil::SystemToLE(v));   }
    inline void    writeSInt32(SInt32 v)       { _write32((UInt32)Alg::ByteUtil::SystemToLE(v));   }
    inline void    writeUInt64(UInt64 v)       { _write64((UInt64)Alg::ByteUtil::SystemToLE(v));   }
    inline void    writeSInt64(SInt64 v)       { _write64((UInt64)Alg::ByteUtil::SystemToLE(v));   }
    inline void    writeFloat(float v)         { v = Alg::ByteUtil::SystemToLE(v); write((UByte*)&v, 4); }
    inline void    writeDouble(double v)       { v = Alg::ByteUtil::SystemToLE(v); write((UByte*)&v, 8); }
    // Writing primitive types - Big Endian
    inline void    writeUByteBE(UByte v)       { _write8((UByte)Alg::ByteUtil::SystemToBE(v));     }
    inline void    writeSByteBE(SByte v)       { _write8((UByte)Alg::ByteUtil::SystemToBE(v));     }
    inline void    writeUInt8BE(UInt16 v)      { _write8((UByte)Alg::ByteUtil::SystemToBE(v));     }
    inline void    writeSInt8BE(SInt16 v)      { _write8((UByte)Alg::ByteUtil::SystemToBE(v));     }
    inline void    writeUInt16BE(UInt16 v)     { _write16((UInt16)Alg::ByteUtil::SystemToBE(v));   }
    inline void    writeSInt16BE(UInt16 v)     { _write16((UInt16)Alg::ByteUtil::SystemToBE(v));   }
    inline void    writeUInt32BE(UInt32 v)     { _write32((UInt32)Alg::ByteUtil::SystemToBE(v));   }
    inline void    writeSInt32BE(UInt32 v)     { _write32((UInt32)Alg::ByteUtil::SystemToBE(v));   }
    inline void    writeUInt64BE(UInt64 v)     { _write64((UInt64)Alg::ByteUtil::SystemToBE(v));   }
    inline void    writeSInt64BE(UInt64 v)     { _write64((UInt64)Alg::ByteUtil::SystemToBE(v));   }
    inline void    writeFloatBE(float v)       { v = Alg::ByteUtil::SystemToBE(v); write((UByte*)&v, 4); }
    inline void    writeDoubleBE(double v)     { v = Alg::ByteUtil::SystemToBE(v); write((UByte*)&v, 8); }

    // Reading primitive types - Little Endian
    inline UByte   readUByte()                 { return (UByte)Alg::ByteUtil::LEToSystem(_read8());    }
    inline SByte   readSByte()                 { return (SByte)Alg::ByteUtil::LEToSystem(_read8());    }
    inline UByte   readUInt8()                 { return (UByte)Alg::ByteUtil::LEToSystem(_read8());    }
    inline SByte   readSInt8()                 { return (SByte)Alg::ByteUtil::LEToSystem(_read8());    }
    inline UInt16  readUInt16()                { return (UInt16)Alg::ByteUtil::LEToSystem(_read16());  }
    inline SInt16  readSInt16()                { return (SInt16)Alg::ByteUtil::LEToSystem(_read16());  }
    inline UInt32  readUInt32()                { return (UInt32)Alg::ByteUtil::LEToSystem(_read32());  }
    inline SInt32  readSInt32()                { return (SInt32)Alg::ByteUtil::LEToSystem(_read32());  }
    inline UInt64  readUInt64()                { return (UInt64)Alg::ByteUtil::LEToSystem(_read64());  }
    inline SInt64  readSInt64()                { return (SInt64)Alg::ByteUtil::LEToSystem(_read64());  }
    inline float   readFloat()                 { float v = 0.0f; read((UByte*)&v, 4); return Alg::ByteUtil::LEToSystem(v); }
    inline double  readDouble()                { double v = 0.0; read((UByte*)&v, 8); return Alg::ByteUtil::LEToSystem(v); }
    // Reading primitive types - Big Endian
    inline UByte   readUByteBE()               { return (UByte)Alg::ByteUtil::BEToSystem(_read8());    }
    inline SByte   readSByteBE()               { return (SByte)Alg::ByteUtil::BEToSystem(_read8());    }
    inline UByte   readUInt8BE()               { return (UByte)Alg::ByteUtil::BEToSystem(_read8());    }
    inline SByte   readSInt8BE()               { return (SByte)Alg::ByteUtil::BEToSystem(_read8());    }
    inline UInt16  readUInt16BE()              { return (UInt16)Alg::ByteUtil::BEToSystem(_read16());  }
    inline SInt16  readSInt16BE()              { return (SInt16)Alg::ByteUtil::BEToSystem(_read16());  }
    inline UInt32  readUInt32BE()              { return (UInt32)Alg::ByteUtil::BEToSystem(_read32());  }
    inline SInt32  readSInt32BE()              { return (SInt32)Alg::ByteUtil::BEToSystem(_read32());  }
    inline UInt64  readUInt64BE()              { return (UInt64)Alg::ByteUtil::BEToSystem(_read64());  }
    inline SInt64  readSInt64BE()              { return (SInt64)Alg::ByteUtil::BEToSystem(_read64());  }
    inline float   readFloatBE()               { float v = 0.0f; read((UByte*)&v, 4); return Alg::ByteUtil::BEToSystem(v); }
    inline double  readDoubleBE()              { double v = 0.0; read((UByte*)&v, 8); return Alg::ByteUtil::BEToSystem(v); }
};


// *** Delegated File

class DelegatedFile : public File
{
protected:
    // Delegating file pointer
    Ptr<File>     m_file;

    // Hidden default constructor
    DelegatedFile() : m_file(0)                             { }
    DelegatedFile(const DelegatedFile &source) : File()    { OVR_UNUSED(source); }
public:
    // Constructors
    DelegatedFile(File *pfile) : m_file(pfile)     { }

    // ** Location Information
    virtual const char* filePath()                               { return m_file->filePath(); }

    // ** File Information
    virtual bool        isValid()                                   { return m_file && m_file->isValid(); }
    virtual bool        isWritable()                                { return m_file->isWritable(); }
//  virtual bool        IsRecoverable()                             { return pFile->IsRecoverable(); }

    virtual int         tell()                                      { return m_file->tell(); }
    virtual SInt64      tell64()                                     { return m_file->tell64(); }

    virtual int         length()                                 { return m_file->length(); }
    virtual SInt64      length64()                                { return m_file->length64(); }

    //virtual bool      Stat(FileStats *pfs)                        { return pFile->Stat(pfs); }

    virtual int         errorCode()                              { return m_file->errorCode(); }

    // ** Stream implementation & I/O
    virtual int         write(const UByte *pbuffer, int numBytes)   { return m_file->write(pbuffer,numBytes); }
    virtual int         read(UByte *pbuffer, int numBytes)          { return m_file->read(pbuffer,numBytes); }

    virtual int         skipBytes(int numBytes)                     { return m_file->skipBytes(numBytes); }

    virtual int         bytesAvailable()                            { return m_file->bytesAvailable(); }

    virtual bool        flush()                                     { return m_file->flush(); }

    // Seeking
    virtual int         seek(int offset, int origin=Seek_Set)       { return m_file->seek(offset,origin); }
    virtual SInt64      seek64(SInt64 offset, int origin=Seek_Set)   { return m_file->seek64(offset,origin); }

    virtual int         copyFromStream(File *pstream, int byteSize) { return m_file->copyFromStream(pstream,byteSize); }

    // Closing the file
    virtual bool        close()                                     { return m_file->close(); }
};


//-----------------------------------------------------------------------------------
// ***** Buffered File

// This file class adds buffering to an existing file
// Buffered file never fails by itself; if there's not
// enough memory for buffer, no buffer's used

class BufferedFile : public DelegatedFile
{
protected:
    enum BufferModeType
    {
        NoBuffer,
        ReadBuffer,
        WriteBuffer
    };

    // Buffer & the mode it's in
    UByte*          m_buffer;
    BufferModeType  m_bufferMode;
    // Position in buffer
    unsigned        m_pos;
    // Data in buffer if reading
    unsigned        m_dataSize;
    // Underlying file position
    UInt64          m_filePos;

    // Initializes buffering to a certain mode
    bool    setBufferMode(BufferModeType mode);
    // Flushes buffer
    // WriteBuffer - write data to disk, ReadBuffer - reset buffer & fix file position
    void    flushBuffer();
    // Loads data into ReadBuffer
    // WARNING: Right now LoadBuffer() assumes the buffer's empty
    void    loadBuffer();

    // Hidden constructor
    BufferedFile();
    inline BufferedFile(const BufferedFile &source) : DelegatedFile() { OVR_UNUSED(source); }
public:

    // Constructor
    // - takes another file as source
    BufferedFile(File *pfile);
    ~BufferedFile();


    // ** Overridden functions

    // We override all the functions that can possibly
    // require buffer mode switch, flush, or extra calculations
    virtual int         tell();
    virtual SInt64      tell64();

    virtual int         length();
    virtual SInt64      length64();

//  virtual bool        Stat(GFileStats *pfs);

    virtual int         write(const UByte *pbufer, int numBytes);
    virtual int         read(UByte *pbufer, int numBytes);

    virtual int         skipBytes(int numBytes);

    virtual int         bytesAvailable();

    virtual bool        flush();

    virtual int         seek(int offset, int origin=Seek_Set);
    virtual SInt64      seek64(SInt64 offset, int origin=Seek_Set);

    virtual int         copyFromStream(File *pstream, int byteSize);

    virtual bool        close();
};


//-----------------------------------------------------------------------------------
// ***** Memory File

class MemoryFile : public File
{
public:

    const char* filePath()       { return FilePath.toCString(); }

    bool        isValid()           { return Valid; }
    bool        isWritable()        { return false; }

    bool        flush()             { return true; }
    int         errorCode()      { return 0; }

    int         tell()              { return FileIndex; }
    SInt64      tell64()             { return (SInt64) FileIndex; }

    int         length()         { return FileSize; }
    SInt64      length64()        { return (SInt64) FileSize; }

    bool        close()
    {
        Valid = false;
        return false;
    }

    int         copyFromStream(File *pstream, int byteSize)
    {   OVR_UNUSED2(pstream, byteSize);
        return 0;
    }

    int         write(const UByte *pbuffer, int numBytes)
    {   OVR_UNUSED2(pbuffer, numBytes);
        return 0;
    }

    int         read(UByte *pbufer, int numBytes)
    {
        if (FileIndex + numBytes > FileSize)
        {
            numBytes = FileSize - FileIndex;
        }

        if (numBytes > 0)
        {
            ::memcpy (pbufer, &FileData [FileIndex], numBytes);

            FileIndex += numBytes;
        }

        return numBytes;
    }

    int         skipBytes(int numBytes)
    {
        if (FileIndex + numBytes > FileSize)
        {
            numBytes = FileSize - FileIndex;
        }

        FileIndex += numBytes;

        return numBytes;
    }

    int         bytesAvailable()
    {
        return (FileSize - FileIndex);
    }

    int         seek(int offset, int origin = Seek_Set)
    {
        switch (origin)
        {
        case Seek_Set : FileIndex  = offset;               break;
        case Seek_Cur : FileIndex += offset;               break;
        case Seek_End : FileIndex  = FileSize - offset;  break;
        }

        return FileIndex;
    }

    SInt64      seek64(SInt64 offset, int origin = Seek_Set)
    {
        return (SInt64) seek((int) offset, origin);
    }

public:

    MemoryFile (const String& fileName, const UByte *pBuffer, int buffSize)
        : FilePath(fileName)
    {
        FileData  = pBuffer;
        FileSize  = buffSize;
        FileIndex = 0;
        Valid     = (!fileName.isEmpty() && pBuffer && buffSize > 0) ? true : false;
    }

    // pfileName should be encoded as UTF-8 to support international file names.
    MemoryFile (const char* pfileName, const UByte *pBuffer, int buffSize)
        : FilePath(pfileName)
    {
        FileData  = pBuffer;
        FileSize  = buffSize;
        FileIndex = 0;
        Valid     = (pfileName && pBuffer && buffSize > 0) ? true : false;
    }
private:

    String       FilePath;
    const UByte *FileData;
    int          FileSize;
    int          FileIndex;
    bool         Valid;
};


// ***** Global path helpers

// Find trailing short filename in a path.
const char* OVR_CDECL GetShortFilename(const char* purl);

} // OVR

#endif
