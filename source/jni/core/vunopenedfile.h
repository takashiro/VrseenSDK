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


    virtual const char* filePath() override               { return 0; }

    // ** File Information
    virtual bool        isValid() override                    { return 0; }
    virtual bool        isWritable() override                 { return 0; }

    // Return position / file size
    virtual int         tell() override                       { return 0; }
    virtual SInt64      tell64() override                      { return 0; }
    virtual int         length() override                  { return 0; }
    virtual SInt64      length64() override                 { return 0; }

//  virtual bool        Stat(FileStats *pfs)        { return 0; }
    virtual int         errorCode() override               { return FileNotFoundError; }

    // ** Stream implementation & I/O
    virtual int         write(const UByte *pbuffer, int numBytes) override      { return -1; OVR_UNUSED2(pbuffer, numBytes); }
    virtual int         read(UByte *pbuffer, int numBytes) override             { return -1; OVR_UNUSED2(pbuffer, numBytes); }
    virtual int         skipBytes(int numBytes) override                        { return 0;  OVR_UNUSED(numBytes); }
    virtual int         bytesAvailable() override                               { return 0; }
    virtual bool        flush() override                                        { return 0; }
    virtual int         seek(int offset, int origin) override                   { return -1; OVR_UNUSED2(offset, origin); }
    virtual SInt64      seek64(SInt64 offset, int origin) override               { return -1; OVR_UNUSED2(offset, origin); }

    virtual int         copyFromStream(VFile *pstream, int byteSize) override    { return -1; OVR_UNUSED2(pstream, byteSize); }
    virtual bool        close() override                                        { return 0; }
};
NV_NAMESPACE_END
