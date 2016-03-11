#pragma once

#define  GFILE_CXX

//#include "Types.h"
//#include "Log.h"

// Standard C library (Captain Obvious guarantees!)
#include <stdio.h>
#ifndef OVR_OS_WINCE
#include <sys/stat.h>
#endif

#include "VSysFile.h"

#ifndef OVR_OS_WINCE
#include <errno.h>
#endif

NV_NAMESPACE_BEGIN

static int SFerror ();

// This is the simplest possible file implementation, it wraps around the descriptor
// This file is delegated to by SysFile.
class VFILEFile : public VFile
{
protected:

    // Allocated filename
    VString      FileName;

    // File handle & open mode
    bool        Opened;
    //FILE*       fs;
    int         OpenFlag;
    // Error code for last request
    int         ErrorCode;

    int         LastOp;

#ifdef OVR_FILE_VERIFY_SEEK_ERRORS
    //UByte*      pFileTestBuffer;
    unsigned char* pFileTestBuffer;
    unsigned       FileTestLength;
    unsigned       TestPos; // File pointer position during tests.
#endif

public:

    VFILEFile()
    {
        Opened = 0;
        FileName = "";

#ifdef OVR_FILE_VERIFY_SEEK_ERRORS
        pFileTestBuffer =0;
        FileTestLength  =0;
        TestPos         =0;
#endif
    }
    // Initialize file by opening it
    VFILEFile(const VString& fileName, int flags, int Mode);
    // The 'pfileName' should be encoded as UTF-8 to support international file names.
    VFILEFile(const char* pfileName, int flags, int Mode);

    ~VFILEFile()
    {
        if (Opened)
            Close();
    }

    virtual const char* filePath();

    // ** File Information
    virtual bool        isValid() override;
    virtual bool        isWritable() override;

    // Return position / file size
    virtual int         tell() override;
    virtual long long      tell64() override;
    virtual int         length() override;
    virtual long long      length64() override;

//  virtual bool        Stat(FileStats *pfs);
    virtual int         errorCode() override;

    // ** Stream implementation & I/O
    virtual int         write(const uchar *pbuffer, int numBytes) override;
    virtual int         read(uchar *pbuffer, int numBytes) override;
    virtual int         skipBytes(int numBytes) override;
    virtual int         bytesAvailable() override;
    virtual bool        Flush() override;
    virtual int         seek(int offset, std::ios_base::seekdir origin) override;
    virtual long long      seek64(long long offset, std::ios_base::seekdir origin) override;

    virtual int         copyFromStream(VFile *pStream, int byteSize) override;
    virtual bool        Close() override;
private:
    void                init();
};


NV_NAMESPACE_END

