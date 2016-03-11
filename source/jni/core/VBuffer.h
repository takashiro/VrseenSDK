#pragma once

#include "vglobal.h"
#include "VDelegatedFile.h"



NV_NAMESPACE_BEGIN
// ***** Buffered File

// This file class adds buffering to an existing file
// Buffered file never fails by itself; if there's not
// enough memory for buffer, no buffer's used

class VBuffer : public VDelegatedFile
{
protected:
    enum BufferModeType
    {
        NoBuffer,
        ReadBuffer,
        WriteBuffer
    };

    // Buffer & the mode it's in
<<<<<<< HEAD
    UByte*          m_buffer;
=======
    uchar*          m_buffer;
>>>>>>> dev
    BufferModeType  m_bufferMode;
    // Position in buffer
    unsigned        m_pos;
    // Data in buffer if reading
    unsigned        m_dataSize;
    // Underlying file position
<<<<<<< HEAD
    UInt64          m_filePos;
=======
    ulonglong          m_filePos;
>>>>>>> dev

    // Initializes buffering to a certain mode
    bool    setBufferMode(BufferModeType mode);
    // Flushes buffer
    // WriteBuffer - write data to disk, ReadBuffer - reset buffer & fix file position
    void    flushBuffer();
    // Loads data into ReadBuffer
    // WARNING: Right now LoadBuffer() assumes the buffer's empty
    void    loadBuffer();

    // Hidden constructor
    VBuffer();
    inline VBuffer(const VBuffer &source) : VDelegatedFile() { OVR_UNUSED(source); }
public:

    // Constructor
    // - takes another file as source
    VBuffer(VFile *pfile);
    ~VBuffer();


    // ** Overridden functions

    // We override all the functions that can possibly
    // require buffer mode switch, flush, or extra calculations
    virtual int         tell() override;
<<<<<<< HEAD
    virtual SInt64      tell64() override;

    virtual int         length() override;
    virtual SInt64      length64() override;

//  virtual bool        Stat(GFileStats *pfs);

    virtual int         write(const UByte *pbufer, int numBytes) override;
    virtual int         read(UByte *pbufer, int numBytes) override;
=======
    virtual long long      tell64() override;

    virtual int         length() override;
    virtual long long      length64() override;

//  virtual bool        Stat(GFileStats *pfs);


    virtual int         write(const uchar *pbufer, int numBytes) override;
    virtual int         read(uchar *pbufer, int numBytes) override;
>>>>>>> dev

    virtual int         skipBytes(int numBytes) override;

    virtual int         bytesAvailable() override;

<<<<<<< HEAD
    virtual bool        flush() override;

    virtual int         seek(int offset, int origin=Seek_Set) override;
    virtual SInt64      seek64(SInt64 offset, int origin=Seek_Set) override;

    virtual int         copyFromStream(VFile *pstream, int byteSize) override;

    virtual bool        close() override;
=======
    virtual bool        Flush() override;

    virtual int         seek(int offset, std::ios_base::seekdir origin=std::ios_base::beg) override;
    virtual long long      seek64(long long offset, std::ios_base::seekdir origin=std::ios_base::beg) override;

    virtual int         copyFromStream(VFile *pstream, int byteSize) override;

    virtual bool        Close() override;
>>>>>>> dev
};

NV_NAMESPACE_END
