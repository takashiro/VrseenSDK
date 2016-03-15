#pragma once

#include "VFile.h"

NV_NAMESPACE_BEGIN

//io授权类，父类将相关的文件操作授权给子类完成
class VDelegatedFile : public VFile
{
protected:
    Ptr<VFile> m_file;

    VDelegatedFile() : m_file(0) { }
//    VDelegatedFile(const VDelegatedFile &source) : VFile() { }

public:
    VDelegatedFile(VFile *pfile) : m_file(pfile) { }

    virtual const std::string filePath() override { return m_file->filePath(); }
    virtual bool isOpened() override { return m_file && m_file->isOpened(); }
    virtual bool isWritable() override { return m_file->isWritable(); }


    virtual int tell() override { return m_file->tell(); }
    virtual long long tell64() override { return m_file->tell64(); }
    virtual int length() override { return m_file->length(); }
    virtual long long length64() override { return m_file->length64(); }
    virtual int errorCode() override { return m_file->errorCode(); }

    virtual int write(const uchar *pbuffer, int numBytes) override { return m_file->write(pbuffer,numBytes); }
    virtual int read(uchar *pbuffer, int numBytes) override { return m_file->read(pbuffer,numBytes); }
    virtual int skipBytes(int numBytes) override { return m_file->skipBytes(numBytes); }
    virtual int bytesAvailable() override { return m_file->bytesAvailable(); }
    virtual bool bufferFlush() override { return m_file->bufferFlush(); }
    virtual int seek(int offset, std::ios_base::seekdir origin=std::ios_base::beg) override { return m_file->seek(offset,origin); }
    virtual long long seek64(long long offset, std::ios_base::seekdir origin=std::ios_base::beg) override { return m_file->seek64(offset,origin); }

    virtual int copyStream(VFile *pstream, int byteSize) override { return m_file->copyStream(pstream,byteSize); }
    virtual bool fileClose() override { return m_file->fileClose(); }
};
NV_NAMESPACE_END


