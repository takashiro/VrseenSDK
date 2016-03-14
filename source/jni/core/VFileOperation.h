#pragma once

#include "VSysFile.h"

#define  GFILE_CXX

#ifndef OVR_OS_WINCE
#include <sys/stat.h>
#endif

#ifndef OVR_OS_WINCE
#include <errno.h>
#endif

NV_NAMESPACE_BEGIN

static int FError ();

// This is the simplest possible file implementation, it wraps around the descriptor
// This file is delegated to by VSysFile.
class VFileOperation : public VFile
{
protected:

    // Allocated filename
    VString m_fileName;

    // File handle & open mode
    bool m_opened;

    // File Open flag
    int m_openFlag;
    // Error code for last request
    int m_errorCode;
    // the previous io operation
    int m_lastOp;

#ifdef OVR_FILE_VERIFY_SEEK_ERRORS
    //UByte*      pFileTestBuffer;
    unsigned char* pFileTestBuffer;
    unsigned FileTestLength;
    unsigned TestPos; // File pointer position during tests.
#endif

public:

    VFileOperation()
        : m_fileName("")
        , m_opened(false)
    {

#ifdef OVR_FILE_VERIFY_SEEK_ERRORS
        pFileTestBuffer =0;
        FileTestLength =0;
        TestPos =0;
#endif
    }
    // Initialize file by opening it
    VFileOperation(const VString& m_fileName, int flags);
    // The 'pfileName' should be encoded as UTF-8 to support international file names.
    VFileOperation(const char* pfileName, int flags);

    ~VFileOperation()
    {
        if (m_opened) {
            fileClose();
        }
    }

    virtual const std::string filePath() override;

    // ** File Information
    virtual bool isOpened() override;
    virtual bool isWritable() override;

    // Return position / file size
    virtual int tell() override;
    virtual long long tell64() override;
    virtual int length() override;
    virtual long long length64() override;
    virtual int errorCode() override;

    // ** Stream implementation & I/O
    virtual int write(const uchar *pbuffer, int numBytes) override;
    virtual int read(uchar *pbuffer, int numBytes) override;
    virtual int skipBytes(int numBytes) override;
    virtual int bytesAvailable() override;
    virtual bool bufferFlush() override;
    virtual int seek(int offset, std::ios_base::seekdir origin) override;
    virtual long long seek64(long long offset, std::ios_base::seekdir origin) override;

    virtual int copyStream(VFile *pStream, int byteSize) override;
    virtual bool fileClose() override;
private:
    void fileInit();
};

NV_NAMESPACE_END

