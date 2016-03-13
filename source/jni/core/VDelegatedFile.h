#pragma once

#include "VFile.h"

NV_NAMESPACE_BEGIN
// *** Delegated File which is a base class

class VDelegatedFile : public VFile
{
protected:
    // Delegating file pointer
    Ptr<VFile> m_file;

    // Hidden default constructor
    VDelegatedFile() : m_file(0) { }
    VDelegatedFile(const VDelegatedFile &source) : VFile() { }
public:
    // Constructors
    VDelegatedFile(VFile *pfile) : m_file(pfile) { }

    // ** Location Information
    virtual const std::string filePath() override { return m_file->filePath(); }

    // ** File Information
    virtual bool isOpened() override { return m_file && m_file->isOpened(); }
    virtual bool isWritable() override { return m_file->isWritable(); }


    virtual int tell() override { return m_file->tell(); }
    virtual long long tell64() override { return m_file->tell64(); }

    virtual int length() override { return m_file->length(); }
    virtual long long length64() override { return m_file->length64(); }

    virtual int errorCode() override { return m_file->errorCode(); }

    // ** Stream implementation & I/O

    virtual int write(const uchar *pbuffer, int numBytes) override { return m_file->write(pbuffer,numBytes); }
    virtual int read(uchar *pbuffer, int numBytes) override { return m_file->read(pbuffer,numBytes); }


    virtual int skipBytes(int numBytes) override { return m_file->skipBytes(numBytes); }

    virtual int bytesAvailable() override { return m_file->bytesAvailable(); }


    virtual bool bufferFlush() override { return m_file->bufferFlush(); }

    // Seeking
    virtual int seek(int offset, std::ios_base::seekdir origin=std::ios_base::beg) override { return m_file->seek(offset,origin); }
    virtual long long seek64(long long offset, std::ios_base::seekdir origin=std::ios_base::beg) override { return m_file->seek64(offset,origin); }

    virtual int copyStream(VFile *pstream, int byteSize) override { return m_file->copyStream(pstream,byteSize); }

    // Closing the file

    virtual bool fileClose() override { return m_file->fileClose(); }
};
NV_NAMESPACE_END


