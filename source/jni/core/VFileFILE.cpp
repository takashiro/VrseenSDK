#define  GFILE_CXX

#include "VFileFILE.h"

NV_NAMESPACE_BEGIN

// ***** File interface

// ***** FILEFile - C streams file

static int SFerror ()
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


VFile *VFileFILEOpen(const VString& path, int flags, int mode)
{
    return new VFILEFile(path, flags, mode);
}

// Initialize file by opening it
VFILEFile::VFILEFile(const VString& fileName, int flags, int mode)
  : FileName(fileName), OpenFlag(flags)
{
    OVR_UNUSED(mode);
    init();
}

// The 'pfileName' should be encoded as UTF-8 to support international file names.
VFILEFile::VFILEFile(const char* pfileName, int flags, int mode)
  : FileName(pfileName), OpenFlag(flags)
{
    OVR_UNUSED(mode);
    init();
}

void VFILEFile::init()
{
    // Open mode for file's open
    //openmode default is in and binary;
     std::ios_base::openmode omode = std::ios_base::in | std::ios_base::binary;

    if (OpenFlag & Open_Truncate)
    {
        if(OpenFlag & Open_Read) {
            omode = std::ios_base::trunc | std::ios_base::in | std::ios_base::out | std::ios_base::binary;
        } else {
            omode = std::ios_base::trunc | std::ios_base::out | std::ios_base::binary;
        }
    }
    else if (OpenFlag & Open_Create)
    {
        if (OpenFlag & Open_Read) {
            omode = std::ios_base::app | std::ios_base::in | std::ios_base::binary;
        } else {
            omode = std::ios_base::app | std::ios_base::binary;
        }
    }
    else if (OpenFlag & Open_Write) {;
        omode = std::ios_base::in | std::ios_base::out | std::ios_base::binary;
    }

#ifdef OVR_OS_WIN32
    SysErrorModeDisabler disabler(FileName.toCString());
#endif

#if defined(OVR_CC_MSVC) && (OVR_CC_MSVC >= 1400)
    wchar_t womode[16];
    wchar_t *pwFileName = (wchar_t*)OVR_ALLOC((UTF8Util::GetLength(FileName.toCString())+1) * sizeof(wchar_t));
    UTF8Util::DecodeString(pwFileName, FileName.toCString());
    OVR_ASSERT(strlen(omode) < sizeof(womode)/sizeof(womode[0]));
    UTF8Util::DecodeString(womode, omode);
    _wfopen_s(&fs, pwFileName, womode);
    OVR_FREE(pwFileName);
#else
    open(FileName.toCString(), omode);
#endif
    Opened = is_open();

    // Set error code
    if (!Opened)
        ErrorCode = SFerror();
    else
    {
        // If we are testing file seek correctness, pre-load the entire file so
        // that we can do comparison tests later.
#ifdef OVR_FILE_VERIFY_SEEK_ERRORS
        TestPos         = 0;
        fseek(fs, 0, SEEK_END);
        FileTestLength  = ftell(fs);
        fseek(fs, 0, SEEK_SET);
        pFileTestBuffer = (UByte*)OVR_ALLOC(FileTestLength);
        if (pFileTestBuffer)
        {
            OVR_ASSERT(FileTestLength == (unsigned)Read(pFileTestBuffer, FileTestLength));
            Seek(0, Seek_Set);
        }
#endif
        if(OpenFlag & Open_Read) {
            seekg(0);
        } else {
            seekp(0);
        }
        ErrorCode = 0;
    }
    LastOp = 0;
}


const char* VFILEFile::filePath()
{
    return FileName.toCString();
}


// ** File Information
bool    VFILEFile::isValid()
{
    return Opened;
}
bool    VFILEFile::isWritable()
{
    return isValid() && (OpenFlag & Open_Write);
}
/*
bool    VFILEFile::IsRecoverable()
{
    return IsValid() && ((OpenFlag&OVR_FO_SAFETRUNC) == OVR_FO_SAFETRUNC);
}
*/

// Return position / file size
int     VFILEFile::tell()
{
    int pos;

    if (OpenFlag & Open_Read) {
        pos = tellg();
    } else {
        pos = tellp();
    }

    if (pos < 0)
        ErrorCode = SFerror();
    return pos;
}

long long  VFILEFile::tell64()
{
    long long pos;

    if (OpenFlag & Open_Read) {
        pos = tellg();
    } else {
        pos = tellp();
    }

    if (pos < 0)
        ErrorCode = SFerror();
    return pos;
}

int     VFILEFile::length()
{
    int pos = tell();
    if (pos >= 0)
    {
        seek (0, std::ios_base::end);
        int size = tell();
        seek (pos, std::ios_base::beg);
        return size;
    }
    return -1;
}

long long VFILEFile::length64()
{
    long long pos = tell64();
    if (pos >= 0)
    {
        seek64 (0, std::ios_base::end);
        long long size = tell64();
        seek64 (pos, std::ios_base::beg);
        return size;
    }
    return -1;
}

int     VFILEFile::errorCode()
{
    return ErrorCode;
}

// ** Stream implementation & I/O
int     VFILEFile::write(const uchar *pbuffer, int numBytes)
{
    if (LastOp && LastOp != Open_Write) {
        bufferFlush();
    }
    LastOp = Open_Write;
//    int written = (int) fwrite(pbuffer, 1, numBytes, fs);
    write(pbuffer, numBytes);

    if (!good()) {
        ErrorCode = SFerror();
    }

#ifdef OVR_FILE_VERIFY_SEEK_ERRORS
    if (written > 0)
        TestPos += written;
#endif
//  return Writtem;
    return numBytes;
}

int     VFILEFile::read(uchar *pbuffer, int numBytes)
{
    if (LastOp && LastOp != Open_Read) {
        bufferFlush();
    }
    LastOp = Open_Read;

//    int read = (int) fread(pbuffer, 1, numBytes, fs);
    read(pbuffer, numBytes);
    if (!good()) {
        ErrorCode = SFerror();
    }

#ifdef OVR_FILE_VERIFY_SEEK_ERRORS
    if (read > 0)
    {
        // Read-in data must match our pre-loaded buffer data!
        UByte* pcompareBuffer = pFileTestBuffer + TestPos;
        for (int i=0; i< read; i++)
        {
            OVR_ASSERT(pcompareBuffer[i] == pbuffer[i]);
        }

        //OVR_ASSERT(!memcmp(pFileTestBuffer + TestPos, pbuffer, read));
        TestPos += read;
        OVR_ASSERT(ftell(fs) == (int)TestPos);
    }
#endif

//    return read;
    return numBytes;
}

// Seeks ahead to skip bytes
int     VFILEFile::skipBytes(int numBytes)
{
    long long pos    = tell64();
    long long newPos = seek64(numBytes, std::ios_base::cur);

    // Return -1 for major error
    if ((pos==-1) || (newPos==-1))
    {
        return -1;
    }
//    ErrorCode = ((NewPos-Pos)<numBytes) ? errno : 0;
    ErrorCode =((newPos - pos) < numBytes) ? errno : 0;
    return int (newPos-(int)pos);
}

// Return # of bytes till EOF
int     VFILEFile::bytesAvailable()
{
    long long pos    = tell64();
    long long endPos = length64();

    // Return -1 for major error
    if ((pos==-1) || (endPos==-1))
    {
        ErrorCode = SFerror();
        return 0;
    }
    else
        ErrorCode = 0;

    return int (endPos-(int)pos);
}

// Flush file contents
bool    VFILEFile::bufferFlush()
{
    flush();
    return good();
}

int     VFILEFile::seek(int offset, std::ios_base::seekdir origin)
{

    if (origin == std::ios_base::beg && offset == tell()) {
        return tell();
    }

    if(OpenFlag & Open_Read) {
        seekg(offset, origin);
    } else {
        seekp(offset, origin);
    }
    if (!good()) {

        return -1;
    }
#ifdef OVR_FILE_VERIFY_SEEK_ERRORS
    // Track file position after seeks for read verification later.
    switch(origin)
    {
    case Seek_Set:  TestPos = offset;       break;
    case Seek_Cur:  TestPos += offset;      break;
    case Seek_End:  TestPos = FileTestLength + offset; break;
    }
    OVR_ASSERT((int)TestPos == Tell());
#endif

    return (int)tell();
}

long long  VFILEFile::seek64(long long offset,std::ios_base::seekdir origin)
{
    return seek((int)offset,origin);
}

int VFILEFile::copyFromStream(VFile *pstream, int byteSize)
{
    uchar   buff[0x4000];
    int     count = 0;
    int     szRequest, szRead, szWritten;

    while (byteSize)
    {
        szRequest = (byteSize > int(sizeof(buff))) ? int(sizeof(buff)) : byteSize;

        szRead    = pstream->read(buff, szRequest);
        szWritten = 0;
        if (szRead > 0)
            szWritten = write(buff, szRead);

        count    += szWritten;
        byteSize -= szWritten;
        if (szWritten < szRequest)
            break;
    }
    return count;
}


bool VFILEFile::fileClose()
{
#ifdef OVR_FILE_VERIFY_SEEK_ERRORS
    if (pFileTestBuffer)
    {
        OVR_FREE(pFileTestBuffer);
        pFileTestBuffer = 0;
        FileTestLength  = 0;
    }
#endif

//    bool closeRet = !fclose(fs);
    close();
    bool closeRet = good();

    if (!closeRet)
    {
        ErrorCode = SFerror();
        return 0;
    }
    else
    {
        Opened    = 0;
//        fs        = 0;
        ErrorCode = 0;
    }

    return 1;
}

// Helper function: obtain file information time.
bool    VSysFile::getFileStat(VFileStat* pfileStat, const VString& path)
{
#if defined(OVR_OS_WIN32)
    // 64-bit implementation on Windows.
    struct __stat64 fileStat;
    // Stat returns 0 for success.
    wchar_t *pwpath = (wchar_t*)OVR_ALLOC((UTF8Util::GetLength(path.toCString())+1)*sizeof(wchar_t));
    UTF8Util::DecodeString(pwpath, path.toCString());

    int ret = _wstat64(pwpath, &fileStat);
    OVR_FREE(pwpath);
    if (ret) return false;
#else
    struct stat fileStat;
    // Stat returns 0 for success.
    if (stat(path.toCString(), &fileStat) != 0)
        return false;
#endif
    pfileStat->accessTime = fileStat.st_atime;
    pfileStat->modifyTime = fileStat.st_mtime;
    pfileStat->fileSize   = fileStat.st_size;
    return true;
}

NV_NAMESPACE_END

