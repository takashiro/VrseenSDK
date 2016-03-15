#pragma once

#include "vglobal.h"
#include "RefCount.h"
#include "VLog.h"
#include "VString.h"

#include <iostream>
#include <fstream>


NV_NAMESPACE_BEGIN

class VFile;

class VFile : public RefCountBase<VFile>,public std::fstream
{
public:

    enum OpenFlags
    {
        Open_Read = 1,
        Open_Write = 2,
        Open_ReadWrite = 3,
        Open_Truncate = 4,
        Open_Create = 8,
        Open_CreateOnly = 24,
    };

    enum OpenMode
    {
        ReadOnly = 0444,
        WriteOnly = 0222,
        ExecuteMode = 0111,

        ReadWrite = 0666
    };

    enum ErrorType
    {
        FileNotFound = 0x1001,
        AccessError = 0x1002,
        IOError = 0x1003,
        iskFullError = 0x1004
    };

public:
    VFile() { }

    virtual const std::string filePath() = 0;

    virtual bool isOpened() = 0;
    virtual bool isWritable() = 0;

    virtual int tell() = 0;
    virtual long long tell64() = 0;
    virtual int length() = 0;
    virtual long long length64() = 0;

    virtual int errorCode() = 0;
    virtual int write(const uchar *pbufer, int numBytes) = 0;
    virtual int read(uchar *pbufer, int numBytes) = 0;
    virtual int skipBytes(int numBytes) = 0;
    virtual int bytesAvailable() = 0;

    virtual bool bufferFlush() = 0;
    inline bool isEnd() { return !bytesAvailable(); }
    virtual int seek(int offset, std::ios_base::seekdir origin=std::ios_base::beg) = 0;
    virtual long long seek64(long long offset, std::ios_base::seekdir origin=std::ios_base::beg) = 0;
    virtual int copyStream(VFile *pstream, int byteSize) = 0;
    virtual bool fileClose() = 0;
};

NV_NAMESPACE_END
