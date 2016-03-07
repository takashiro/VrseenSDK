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
    UByte*          m_buffer;
    BufferModeType  m_bufferMode;
    // Position in buffer
    unsigned        m_pos;
    // Data in buffer if reading
    unsigned        m_dataSize;
    // Underlying file position
    UInt64          m_filePos;

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
    virtual int         tell();
    virtual SInt64      tell64();

    virtual int         length();
    virtual SInt64      length64();

//  virtual bool        Stat(GFileStats *pfs);

    virtual int         write(const UByte *pbufer, int numBytes);
    virtual int         read(UByte *pbufer, int numBytes);

    virtual int         skipBytes(int numBytes);

    virtual int         bytesAvailable();

    virtual bool        flush();

    virtual int         seek(int offset, int origin=Seek_Set);
    virtual SInt64      seek64(SInt64 offset, int origin=Seek_Set);

    virtual int         copyFromStream(VFile *pstream, int byteSize);

    virtual bool        close();
};

NV_NAMESPACE_END
