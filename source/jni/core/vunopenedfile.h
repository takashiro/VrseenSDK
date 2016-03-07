#pragma  once

#include "VFile.h"
#include "vglobal.h"


NV_NAMESPACE_BEGIN


// This is - a dummy file that fails on all calls.

class VUnopenedFile : public VFile
{
public:
    VUnopenedFile()  { }
    ~VUnopenedFile() { }


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

    virtual int         copyFromStream(VFile *pstream, int byteSize)   { return -1; OVR_UNUSED2(pstream, byteSize); }
    virtual bool        close()                                       { return 0; }
};
NV_NAMESPACE_END
