/**************************************************************************

Filename    :   OVR_SysFile.cpp
Content     :   File wrapper class implementation (Win32)

Created     :   April 5, 1999
Authors     :   Michael Antonov

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

**************************************************************************/

#define  GFILE_CXX

// Standard C library (Captain Obvious guarantees!)
#include <stdio.h>

#include "SysFile.h"

namespace NervGear {

// This is - a dummy file that fails on all calls.

class UnopenedFile : public File
{
public:
    UnopenedFile()  { }
    ~UnopenedFile() { }

    virtual const char* filePath()               { return 0; }

    // ** File Information
    virtual bool        isValid()                   { return 0; }
    virtual bool        isWritable()                { return 0; }

    // Return position / file size
    virtual int         tell()                      { return 0; }
    virtual SInt64      tell64()                     { return 0; }
    virtual int         length()                 { return 0; }
    virtual SInt64      length64()                { return 0; }

//  virtual bool        Stat(FileStats *pfs)        { return 0; }
    virtual int         errorCode()              { return FileNotFoundError; }

    // ** Stream implementation & I/O
    virtual int         write(const UByte *pbuffer, int numBytes)     { return -1; OVR_UNUSED2(pbuffer, numBytes); }
    virtual int         read(UByte *pbuffer, int numBytes)            { return -1; OVR_UNUSED2(pbuffer, numBytes); }
    virtual int         skipBytes(int numBytes)                       { return 0;  OVR_UNUSED(numBytes); }
    virtual int         bytesAvailable()                              { return 0; }
    virtual bool        flush()                                       { return 0; }
    virtual int         seek(int offset, int origin)                  { return -1; OVR_UNUSED2(offset, origin); }
    virtual SInt64      seek64(SInt64 offset, int origin)              { return -1; OVR_UNUSED2(offset, origin); }

    virtual int         copyFromStream(File *pstream, int byteSize)   { return -1; OVR_UNUSED2(pstream, byteSize); }
    virtual bool        close()                                       { return 0; }
};



// ***** System File

// System file is created to access objects on file system directly
// This file can refer directly to path

// ** Constructor
SysFile::SysFile() : DelegatedFile(0)
{
    m_file = *new UnopenedFile;
}

File* FileFILEOpen(const VString& path, int flags, int mode);

// Opens a file
SysFile::SysFile(const VString& path, int flags, int mode) : DelegatedFile(0)
{
    open(path, flags, mode);
}


// ** Open & management
// Will fail if file's already open
bool SysFile::open(const VString& path, int flags, int mode)
{
    m_file = *FileFILEOpen(path, flags, mode);
    if ((!m_file) || (!m_file->isValid()))
    {
        m_file = *new UnopenedFile;
        return 0;
    }
    //pFile = *OVR_NEW DelegatedFile(pFile); // MA Testing
    if (flags & Open_Buffered)
        m_file = *new BufferedFile(m_file);
    return 1;
}


// ** Overrides

int SysFile::errorCode()
{
    return m_file ? m_file->errorCode() : FileNotFoundError;
}


// Overrides to provide re-open support
bool SysFile::isValid()
{
    return m_file && m_file->isValid();
}
bool SysFile::close()
{
    if (isValid())
    {
        DelegatedFile::close();
        m_file = *new UnopenedFile;
        return 1;
    }
    return 0;
}

} // OVR
