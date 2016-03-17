#define  GFILE_CXX

#include "VFileOperation.h"

NV_NAMESPACE_BEGIN

static int FError ()
{
    if (errno == ENOENT) {
        return VFile::FileNotFound;
    } else if (errno == EACCES || errno == EPERM) {
        return VFile::AccessError;
    } else if (errno == ENOSPC) {
        return VFile::iskFullError;
    } else {
        return VFile::IOError;
    }
};

VFile *VOpenFile(const VString& path, int flags)
{
    return new VFileOperation(path, flags);
}

VFileOperation::VFileOperation(const VString& fileName, int flags)
  : m_fileName(fileName)
  , m_openFlag(flags)
{
    fileInit();
}

VFileOperation::VFileOperation(const char* fileName, int flags)
  : m_fileName(fileName)
  , m_openFlag(flags)
{
    fileInit();
}

void VFileOperation::fileInit()
{
    //openmode default is in and binary;
     std::ios_base::openmode openMode = std::ios_base::in | std::ios_base::binary;

    if (m_openFlag & Open_Truncate) {
        if(m_openFlag & Open_Read) {
            openMode = std::ios_base::trunc
                    | std::ios_base::in
                    | std::ios_base::out
                    | std::ios_base::binary;
        } else {
            openMode = std::ios_base::trunc
                    | std::ios_base::out
                    | std::ios_base::binary;
        }
    } else if (m_openFlag & Open_Create) {
        if (m_openFlag & Open_Read) {
            openMode = std::ios_base::app
                    | std::ios_base::in
                    | std::ios_base::binary;
        } else {
            openMode = std::ios_base::app
                    | std::ios_base::binary;
        }
    } else if (m_openFlag & Open_Write) {
        openMode = std::ios_base::in
                | std::ios_base::out
                | std::ios_base::binary;
    }

    open(m_fileName.toCString(), openMode);

    m_opened = is_open();

    // Set error code
    if (!m_opened) {
        m_errorCode = FError();
    } else {
        if(m_openFlag & Open_Read) {
            seekg(0);
        } else {
            seekp(0);
        }
        m_errorCode = 0;
    }
    m_lastOp = 0;
}

const std::string VFileOperation::filePath()
{
    return m_fileName.toStdString();
}

bool    VFileOperation::isOpened()
{
    return m_opened;
}

bool    VFileOperation::isWritable()
{
    return isOpened() && (m_openFlag & Open_Write);
}

int     VFileOperation::tell()
{
    int position;

    if (m_openFlag & Open_Read) {
        position = tellg();
    } else {
        position = tellp();
    }

    if (position < 0) {
        m_errorCode = FError();
    }
    return position;
}

long long  VFileOperation::tell64()
{
    long long position;

    if (m_openFlag & Open_Read) {
        position = tellg();
    } else {
        position = tellp();
    }

    if (position < 0) {
        m_errorCode = FError();
    }
    return position;
}

int     VFileOperation::length()
{
    int position = tell();
    if (position >= 0) {
        seek(0, std::ios_base::end);
        int len = tell();
        seek(position, std::ios_base::beg);
        return len;
    }
    return -1;
}

long long VFileOperation::length64()
{
    long long position = tell64();
    if (position >= 0) {
        seek64(0, std::ios_base::end);
        long long len = tell64();
        seek64(position, std::ios_base::beg);
        return len;
    }
    return -1;
}

int     VFileOperation::errorCode()
{
    return m_errorCode;
}

int     VFileOperation::write(const uchar *buffer, int byteNum)
{
    if (m_lastOp && m_lastOp != Open_Write) {
        bufferFlush();
    }
    m_lastOp = Open_Write;
    write(buffer, byteNum);

    if (!good()) {
        m_errorCode = FError();
        return 0;
    }

    return byteNum;
}

int     VFileOperation::read(uchar *buffer, int byteNum)
{
    if (m_lastOp && m_lastOp != Open_Read) {
        bufferFlush();
    }
    m_lastOp = Open_Read;

    read(buffer, byteNum);
    if (!good()) {
        m_errorCode = FError();
        return 0;
    }

    return byteNum;
}

// Seeks ahead to skip bytes
int     VFileOperation::skipBytes(int byteNum)
{
    long long oldPosition    = tell64();
    long long newPosition = seek64(byteNum, std::ios_base::cur);

    // Return -1 for major error
    if ((oldPosition==-1) || (newPosition==-1)) {
        return -1;
    }

    m_errorCode =((newPosition - oldPosition) < byteNum) ? errno : 0;
    return static_cast<int>(newPosition - oldPosition);
}

// Return # of bytes till EOF
int     VFileOperation::bytesAvailable()
{
    long long position    = tell64();
    long long endPosition = length64();

    // Return -1 for major error
    if ((position==-1) || (endPosition==-1)) {
        m_errorCode = FError();
        return 0;
    }
    else {
        m_errorCode = 0;
    }

    return static_cast<int>(endPosition - position);
}

// Flush file contents
bool    VFileOperation::bufferFlush()
{
    flush();
    return good();
}

int     VFileOperation::seek(int offset, std::ios_base::seekdir startPos)
{

    if (startPos == std::ios_base::beg && offset == tell()) {
        return tell();
    }

    if(m_openFlag & Open_Read) {
        seekg(offset, startPos);
    } else {
        seekp(offset, startPos);
    }

    if (!good()) {
        return -1;
    }

    return reinterpret_cast<int>(tell());
}

long long  VFileOperation::seek64(long long offset,std::ios_base::seekdir startPos)
{
    return seek(static_cast<int>(offset),startPos);
}

int VFileOperation::copyStream(VFile *fstream, int num)
{
    uchar temp[0x4000];
    int size = 0;
    int tempRead;
    int readNum;
    int tempWrite;

    while (num) {
        tempRead = (num > int(sizeof(temp))) ? int(sizeof(temp)) : num;

        readNum = fstream->read(temp, tempRead);
        tempWrite = 0;
        if (readNum > 0) {
            tempWrite = write(temp, readNum);
        }

        size += tempWrite;
        num -= tempWrite;
        if (tempWrite < tempRead){
            break;
        }
    }
    return size;
}


bool VFileOperation::close()
{
    close();
    bool isCloseRet = good();

    if (!isCloseRet) {
        m_errorCode = FError();
        return false;
    } else {
        m_opened = 0;
        m_errorCode = 0;
    }
    return true;
}

// Helper function: obtain file information time.
bool    VSysFile::GetFileStat(VFileStat* pfileStat, const VString& path)
{
    struct stat fileStat;

    if (stat(path.toCString(), &fileStat) != 0) {
        return false;
    }

    pfileStat->accessTime = fileStat.st_atime;
    pfileStat->modifyTime = fileStat.st_mtime;
    pfileStat->fileSize = fileStat.st_size;
    return true;
}

NV_NAMESPACE_END

