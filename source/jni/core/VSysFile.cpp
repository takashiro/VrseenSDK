
#define  GFILE_CXX

// Standard C library (Captain Obvious guarantees!)
#include <stdio.h>


#include "VSysFile.h"
#include "VUnopenedFile.h"

NV_NAMESPACE_BEGIN

// This is - a dummy file that fails on all calls.

//class VUnopenedFile : public VFile
//{
//public:
//    VUnopenedFile()  { }
//    ~VUnopenedFile() { }


//    virtual const char* filePath()               { return 0; }

//    // ** File Information
//    virtual bool        isValid()                   { return 0; }
//    virtual bool        isWritable()                { return 0; }

//    // Return position / file size
//    virtual int         tell()                      { return 0; }
//    virtual SInt64      tell64()                     { return 0; }
//    virtual int         length()                 { return 0; }
//    virtual SInt64      length64()                { return 0; }

//  virtual bool        Stat(FileStats *pfs)        { return 0; }
//    virtual int         errorCode()              { return FileNotFoundError; }

//    // ** Stream implementation & I/O
//    virtual int         write(const UByte *pbuffer, int numBytes)     { return -1; OVR_UNUSED2(pbuffer, numBytes); }
//    virtual int         read(UByte *pbuffer, int numBytes)            { return -1; OVR_UNUSED2(pbuffer, numBytes); }
//    virtual int         skipBytes(int numBytes)                       { return 0;  OVR_UNUSED(numBytes); }
//    virtual int         bytesAvailable()                              { return 0; }
//    virtual bool        flush()                                       { return 0; }
//    virtual int         seek(int offset, int origin)                  { return -1; OVR_UNUSED2(offset, origin); }
//    virtual SInt64      seek64(SInt64 offset, int origin)              { return -1; OVR_UNUSED2(offset, origin); }

//    virtual int         copyFromStream(File *pstream, int byteSize)   { return -1; OVR_UNUSED2(pstream, byteSize); }
//    virtual bool        close()                                       { return 0; }
//};



// ***** System File

// System file is created to access objects on file system directly
// This file can refer directly to path

// ** Constructor
VSysFile::VSysFile() : VDelegatedFile(0)
{
    m_file = *new VUnopenedFile;
}

VFile* FileFILEOpen(const VString& path, int flags, int mode);

// Opens a file
VSysFile::VSysFile(const VString& path, int flags, int mode) : VDelegatedFile(0)
{
    open(path, flags, mode);
}


// ** Open & management
// Will fail if file's already open
bool VSysFile::open(const VString& path, int flags, int mode)
{
    m_file = *FileFILEOpen(path, flags, mode);
    if ((!m_file) || (!m_file->isValid()))
    {
        m_file = *new VUnopenedFile;
        return 0;
    }
    //pFile = *OVR_NEW DelegatedFile(pFile); // MA Testing
    if (flags & Open_Buffered)
        m_file = *new VBufferedFile(m_file);
    return 1;
}


// ** Overrides

int VSysFile::errorCode()
{
    return m_file ? m_file->errorCode() : FileNotFoundError;
}


// Overrides to provide re-open support
bool VSysFile::isValid()
{
    return m_file && m_file->isValid();
}
bool VSysFile::close()
{
    if (isValid())
    {
        VDelegatedFile::close();
        m_file = *new VUnopenedFile;
        return 1;
    }
    return 0;
}

NV_NAMESPACE_END
