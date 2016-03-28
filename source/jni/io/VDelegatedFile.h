#pragma once

#include "VFile.h"

NV_NAMESPACE_BEGIN

//io授权类，父类将相关的文件操作授权给子类完成
class VDelegatedFile : public VFile
{
protected:
    VFile *m_file;

    VDelegatedFile() : m_file(0) { }
public:
    VDelegatedFile(VFile *pfile) : m_file(pfile) { }
    ~VDelegatedFile() { delete m_file; }

    const std::string filePath() override { return m_file->filePath(); }
    bool isOpened() override { return m_file && m_file->isOpened(); }
    bool isWritable() override { return m_file->isWritable(); }


    int tell() override { return m_file->tell(); }
    long long tell64() override { return m_file->tell64(); }
    int length() override { return m_file->length(); }
    long long length64() override { return m_file->length64(); }
    int errorCode() override { return m_file->errorCode(); }

    int write(const uchar *pbuffer, int numBytes) override { return m_file->write(pbuffer,numBytes); }
    int read(uchar *pbuffer, int numBytes) override { return m_file->read(pbuffer,numBytes); }
    int skipBytes(int numBytes) override { return m_file->skipBytes(numBytes); }
    int bytesAvailable() override { return m_file->bytesAvailable(); }
    bool bufferFlush() override { return m_file->bufferFlush(); }
    int seek(int offset, std::ios_base::seekdir origin=std::ios_base::beg) override { return m_file->seek(offset,origin); }
    long long seek64(long long offset, std::ios_base::seekdir origin=std::ios_base::beg) override { return m_file->seek64(offset,origin); }

    int copyStream(VFile *pstream, int byteSize) override { return m_file->copyStream(pstream,byteSize); }
    bool close() override { return m_file->close(); }
};
NV_NAMESPACE_END


