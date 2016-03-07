/*
 * VFile.cpp
 *
 *  Created on: 2016年3月4日
 *      Author: WZTCM
 */

#include "VBufferedFile.h"


NV_NAMESPACE_BEGIN

// Buffered file adds buffering to an existing file
// FILE_BUFFER_SIZE defines the size of internal buffer, while
// FILEBUFFER_TOLERANCE controls the amount of data we'll effectively try to buffer
#define FILE_BUFFER_SIZE         (8192 - 8)
#define FILE_BUFFER_TOLERANCE    4096

// ** Constructor/Destructor

// Hidden constructor
// Not supposed to be used
VBufferedFile::VBufferedFile() : VDelegatedFile(0)
{
    m_buffer      = (UByte*)OVR_ALLOC(FILE_BUFFER_SIZE);
    m_bufferMode  = NoBuffer;
    m_filePos     = 0;
    m_pos         = 0;
    m_dataSize    = 0;
}

// Takes another file as source
VBufferedFile::VBufferedFile(VFile *pfile) : VDelegatedFile(pfile)
{
    m_buffer      = (UByte*)OVR_ALLOC(FILE_BUFFER_SIZE);
    m_bufferMode  = NoBuffer;
    m_filePos     = pfile->tell64();
    m_pos         = 0;
    m_dataSize    = 0;
}


// Destructor
VBufferedFile::~VBufferedFile()
{
    // Flush in case there's data
    if (m_file)
        flushBuffer();
    // Get rid of buffer
    if (m_buffer)
        OVR_FREE(m_buffer);
}

/*
bool    BufferedFile::VCopy(const Object &source)
{
    if (!DelegatedFile::VCopy(source))
        return 0;

    // Data members
    BufferedFile *psource = (BufferedFile*)&source;

    // Buffer & the mode it's in
    pBuffer         = psource->pBuffer;
    BufferMode      = psource->BufferMode;
    Pos             = psource->Pos;
    DataSize        = psource->DataSize;
    return 1;
}
*/

// Initializes buffering to a certain mode
bool    VBufferedFile::setBufferMode(BufferModeType mode)
{
    if (!m_buffer)
        return false;
    if (mode == m_bufferMode)
        return true;

    flushBuffer();

    // Can't set write mode if we can't write
    if ((mode==WriteBuffer) && (!m_file || !m_file->isWritable()) )
        return 0;

    // And SetMode
    m_bufferMode = mode;
    m_pos        = 0;
    m_dataSize   = 0;
    return 1;
}

// Flushes buffer
void    VBufferedFile::flushBuffer()
{
    switch(m_bufferMode)
    {
        case WriteBuffer:
            // Write data in buffer
            m_filePos += m_file->write(m_buffer,m_pos);
            m_pos = 0;
            break;

        case ReadBuffer:
            // Seek back & reset buffer data
            if ((m_dataSize - m_pos)>0)
                m_filePos = m_file->seek64(-(int)(m_dataSize-m_pos), Seek_Cur);
            m_dataSize = 0;
            m_pos      = 0;
            break;
        default:
            // not handled!
            break;
    }
}

// Reloads data for ReadBuffer
void    VBufferedFile::loadBuffer()
{
    if (m_bufferMode == ReadBuffer)
    {
        // We should only reload once all of pre-loaded buffer is consumed.
        OVR_ASSERT(m_pos == m_dataSize);

        // WARNING: Right now LoadBuffer() assumes the buffer's empty
        int sz   = m_file->read(m_buffer,FILE_BUFFER_SIZE);
        m_dataSize = sz<0 ? 0 : (unsigned)sz;
        m_pos      = 0;
        m_filePos  += m_dataSize;
    }
}


// ** Overridden functions

// We override all the functions that can possibly
// require buffer mode switch, flush, or extra calculations

// Tell() requires buffer adjustment
int     VBufferedFile::tell()
{
    if (m_bufferMode == ReadBuffer)
        return int (m_filePos - m_dataSize + m_pos);

    int pos = m_file->tell();
    // Adjust position based on buffer mode & data
    if (pos!=-1)
    {
        OVR_ASSERT(m_bufferMode != ReadBuffer);
        if (m_bufferMode == WriteBuffer)
            pos += m_pos;
    }
    return pos;
}

SInt64  VBufferedFile::tell64()
{
    if (m_bufferMode == ReadBuffer)
        return m_filePos - m_dataSize + m_pos;

    SInt64 pos = m_file->tell64();
    if (pos!=-1)
    {
        OVR_ASSERT(m_bufferMode != ReadBuffer);
        if (m_bufferMode == WriteBuffer)
            pos += m_pos;
    }
    return pos;
}

int     VBufferedFile::length()
{
    int len = m_file->length();
    // If writing through buffer, file length may actually be bigger
    if ((len!=-1) && (m_bufferMode==WriteBuffer))
    {
        int currPos = m_file->tell() + m_pos;
        if (currPos>len)
            len = currPos;
    }
    return len;
}
SInt64  VBufferedFile::length64()
{
    SInt64 len = m_file->length64();
    // If writing through buffer, file length may actually be bigger
    if ((len!=-1) && (m_bufferMode==WriteBuffer))
    {
        SInt64 currPos = m_file->tell64() + m_pos;
        if (currPos>len)
            len = currPos;
    }
    return len;
}

/*
bool    VBufferedFile::Stat(FileStats *pfs)
{
    // Have to fix up length is stat
    if (pFile->Stat(pfs))
    {
        if (BufferMode==WriteBuffer)
        {
            SInt64 currPos = pFile->LTell() + Pos;
            if (currPos > pfs->Size)
            {
                pfs->Size   = currPos;
                // ??
                pfs->Blocks = (pfs->Size+511) >> 9;
            }
        }
        return 1;
    }
    return 0;
}
*/

int     VBufferedFile::write(const UByte *psourceBuffer, int numBytes)
{
    if ( (m_bufferMode==WriteBuffer) || setBufferMode(WriteBuffer))
    {
        // If not data space in buffer, flush
        if ((FILE_BUFFER_SIZE-(int)m_pos)<numBytes)
        {
            flushBuffer();
            // If bigger then tolerance, just write directly
            if (numBytes>FILE_BUFFER_TOLERANCE)
            {
                int sz = m_file->write(psourceBuffer,numBytes);
                if (sz > 0)
                    m_filePos += sz;
                return sz;
            }
        }

        // Enough space in buffer.. so copy to it
        memcpy(m_buffer+m_pos, psourceBuffer, numBytes);
        m_pos += numBytes;
        return numBytes;
    }
    int sz = m_file->write(psourceBuffer,numBytes);
    if (sz > 0)
        m_filePos += sz;
    return sz;
}

int     VBufferedFile::read(UByte *pdestBuffer, int numBytes)
{
    if ( (m_bufferMode==ReadBuffer) || setBufferMode(ReadBuffer))
    {
        // Data in buffer... copy it
        if ((int)(m_dataSize-m_pos) >= numBytes)
        {
            memcpy(pdestBuffer, m_buffer+m_pos, numBytes);
            m_pos += numBytes;
            return numBytes;
        }

        // Not enough data in buffer, copy buffer
        int     readBytes = m_dataSize-m_pos;
        memcpy(pdestBuffer, m_buffer+m_pos, readBytes);
        numBytes    -= readBytes;
        pdestBuffer += readBytes;
        m_pos = m_dataSize;

        // Don't reload buffer if more then tolerance
        // (No major advantage, and we don't want to write a loop)
        if (numBytes>FILE_BUFFER_TOLERANCE)
        {
            numBytes = m_file->read(pdestBuffer,numBytes);
            if (numBytes > 0)
            {
                m_filePos += numBytes;
                m_pos = m_dataSize = 0;
            }
            return readBytes + ((numBytes==-1) ? 0 : numBytes);
        }

        // Reload the buffer
        // WARNING: Right now LoadBuffer() assumes the buffer's empty
        loadBuffer();
        if ((int)(m_dataSize-m_pos) < numBytes)
            numBytes = (int)m_dataSize-m_pos;

        memcpy(pdestBuffer, m_buffer+m_pos, numBytes);
        m_pos += numBytes;
        return numBytes + readBytes;

        /*
        // Alternative Read implementation. The one above is probably better
        // due to FILEBUFFER_TOLERANCE.
        int     total = 0;

        do {
            int     bufferBytes = (int)(DataSize-Pos);
            int     copyBytes = (bufferBytes > numBytes) ? numBytes : bufferBytes;

            memcpy(pdestBuffer, pBuffer+Pos, copyBytes);
            numBytes    -= copyBytes;
            pdestBuffer += copyBytes;
            Pos         += copyBytes;
            total       += copyBytes;

            if (numBytes == 0)
                break;
            LoadBuffer();

        } while (DataSize > 0);

        return total;
        */
    }
    int sz = m_file->read(pdestBuffer,numBytes);
    if (sz > 0)
        m_filePos += sz;
    return sz;
}


int     VBufferedFile::skipBytes(int numBytes)
{
    int skippedBytes = 0;

    // Special case for skipping a little data in read buffer
    if (m_bufferMode==ReadBuffer)
    {
        skippedBytes = (((int)m_dataSize-(int)m_pos) >= numBytes) ? numBytes : (m_dataSize-m_pos);
        m_pos          += skippedBytes;
        numBytes     -= skippedBytes;
    }

    if (numBytes)
    {
        numBytes = m_file->skipBytes(numBytes);
        // Make sure we return the actual number skipped, or error
        if (numBytes!=-1)
        {
            skippedBytes += numBytes;
            m_filePos += numBytes;
            m_pos = m_dataSize = 0;
        }
        else if (skippedBytes <= 0)
            skippedBytes = -1;
    }
    return skippedBytes;
}

int     VBufferedFile::bytesAvailable()
{
    int available = m_file->bytesAvailable();
    // Adjust available size based on buffers
    switch(m_bufferMode)
    {
        case ReadBuffer:
            available += m_dataSize-m_pos;
            break;
        case WriteBuffer:
            available -= m_pos;
            if (available<0)
                available= 0;
            break;
        default:
            break;
    }
    return available;
}

bool    VBufferedFile::flush()
{
    flushBuffer();
    return m_file->flush();
}

// Seeking could be optimized better..
int     VBufferedFile::seek(int offset, int origin)
{
    if (m_bufferMode == ReadBuffer)
    {
        if (origin == Seek_Cur)
        {
            // Seek can fall either before or after Pos in the buffer,
            // but it must be within bounds.
            if (((unsigned(offset) + m_pos)) <= m_dataSize)
            {
                m_pos += offset;
                return int (m_filePos - m_dataSize + m_pos);
            }

            // Lightweight buffer "Flush". We do this to avoid an extra seek
            // back operation which would take place if we called FlushBuffer directly.
            origin = Seek_Set;
            OVR_ASSERT(((m_filePos - m_dataSize + m_pos) + (UInt64)offset) < ~(UInt64)0);
            offset = (int)(m_filePos - m_dataSize + m_pos) + offset;
            m_pos = m_dataSize = 0;
        }
        else if (origin == Seek_Set)
        {
            if (((unsigned)offset - (m_filePos-m_dataSize)) <= m_dataSize)
            {
                OVR_ASSERT((m_filePos-m_dataSize) < ~(UInt64)0);
                m_pos = (unsigned)offset - (unsigned)(m_filePos-m_dataSize);
                return offset;
            }
            m_pos = m_dataSize = 0;
        }
        else
        {
            flushBuffer();
        }
    }
    else
    {
        flushBuffer();
    }

    /*
    // Old Seek Logic
    if (origin == Seek_Cur && offset + Pos < DataSize)
    {
        //OVR_ASSERT((FilePos - DataSize) >= (FilePos - DataSize + Pos + offset));
        Pos += offset;
        OVR_ASSERT(int (Pos) >= 0);
        return int (FilePos - DataSize + Pos);
    }
    else if (origin == Seek_Set && unsigned(offset) >= FilePos - DataSize && unsigned(offset) < FilePos)
    {
        Pos = unsigned(offset - FilePos + DataSize);
        OVR_ASSERT(int (Pos) >= 0);
        return int (FilePos - DataSize + Pos);
    }

    FlushBuffer();
    */


    m_filePos = m_file->seek(offset,origin);
    return int (m_filePos);
}

SInt64  VBufferedFile::seek64(SInt64 offset, int origin)
{
    if (m_bufferMode == ReadBuffer)
    {
        if (origin == Seek_Cur)
        {
            // Seek can fall either before or after Pos in the buffer,
            // but it must be within bounds.
            if (((unsigned(offset) + m_pos)) <= m_dataSize)
            {
                m_pos += (unsigned)offset;
                return SInt64(m_filePos - m_dataSize + m_pos);
            }

            // Lightweight buffer "Flush". We do this to avoid an extra seek
            // back operation which would take place if we called FlushBuffer directly.
            origin = Seek_Set;
            offset = (SInt64)(m_filePos - m_dataSize + m_pos) + offset;
            m_pos = m_dataSize = 0;
        }
        else if (origin == Seek_Set)
        {
            if (((UInt64)offset - (m_filePos-m_dataSize)) <= m_dataSize)
            {
                m_pos = (unsigned)((UInt64)offset - (m_filePos-m_dataSize));
                return offset;
            }
            m_pos = m_dataSize = 0;
        }
        else
        {
            flushBuffer();
        }
    }
    else
    {
        flushBuffer();
    }

/*
    OVR_ASSERT(BufferMode != NoBuffer);

    if (origin == Seek_Cur && offset + Pos < DataSize)
    {
        Pos += int (offset);
        return FilePos - DataSize + Pos;
    }
    else if (origin == Seek_Set && offset >= SInt64(FilePos - DataSize) && offset < SInt64(FilePos))
    {
        Pos = unsigned(offset - FilePos + DataSize);
        return FilePos - DataSize + Pos;
    }

    FlushBuffer();
    */

    m_filePos = m_file->seek64(offset,origin);
    return m_filePos;
}

int    VBufferedFile::copyFromStream(VFile *pstream, int byteSize)
{
    // We can't rely on overridden Write()
    // because delegation doesn't override virtual pointers
    // So, just re-implement
    UByte   buff[0x4000];
    int     count = 0;
    int     szRequest, szRead, szWritten;

    while(byteSize)
    {
        szRequest = (byteSize > int(sizeof(buff))) ? int(sizeof(buff)) : byteSize;

        szRead    = pstream->read(buff,szRequest);
        szWritten = 0;
        if (szRead > 0)
            szWritten = write(buff,szRead);

        count   +=szWritten;
        byteSize-=szWritten;
        if (szWritten < szRequest)
            break;
    }
    return count;
}

// Closing files
bool    VBufferedFile::close()
{
    switch(m_bufferMode)
    {
        case WriteBuffer:
            flushBuffer();
            break;
        case ReadBuffer:
            // No need to seek back on close
            m_bufferMode = NoBuffer;
            break;
        default:
            break;
    }
    return m_file->close();
}

/* defninetin of GetShortFilename()

// ***** Global path helpers

// Find trailing short filename in a path.
const char* OVR_CDECL GetShortFilename(const char* purl)
{
    uint len = strlen(purl);
    for (uint i=len; i>0; i--)
        if (purl[i]=='\\' || purl[i]=='/')
            return purl+i+1;
    return purl;
}
*/
NV_NAMESPACE_END



