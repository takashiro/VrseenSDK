#pragma once

#include "vglobal.h"
#include "VFile.h"

NV_NAMESPACE_BEGIN
// *** Delegated File which is a base class

class VDelegatedFile : public VFile
{
protected:
    // Delegating file pointer
    Ptr<VFile>     m_file;

    // Hidden default constructor
    VDelegatedFile() : m_file(0)                             { }
    VDelegatedFile(const VDelegatedFile &source) : VFile()    { OVR_UNUSED(source); }
public:
    // Constructors
    VDelegatedFile(VFile *pfile) : m_file(pfile)     { }

    // ** Location Information
    virtual const char* filePath() override                              { return m_file->filePath(); }

    // ** File Information
    virtual bool        isValid() override                                  { return m_file && m_file->isValid(); }
    virtual bool        isWritable() override                                { return m_file->isWritable(); }
//  virtual bool        IsRecoverable()                             { return pFile->IsRecoverable(); }

    virtual int         tell() override                                       { return m_file->tell(); }
    virtual SInt64      tell64() override                                      { return m_file->tell64(); }

    virtual int         length() override                                  { return m_file->length(); }
    virtual SInt64      length64() override                                 { return m_file->length64(); }

    //virtual bool      Stat(FileStats *pfs)                        { return pFile->Stat(pfs); }

    virtual int         errorCode() override                              { return m_file->errorCode(); }

    // ** Stream implementation & I/O
    virtual int         write(const UByte *pbuffer, int numBytes) override   { return m_file->write(pbuffer,numBytes); }
    virtual int         read(UByte *pbuffer, int numBytes) override           { return m_file->read(pbuffer,numBytes); }

    virtual int         skipBytes(int numBytes) override                      { return m_file->skipBytes(numBytes); }

    virtual int         bytesAvailable() override                             { return m_file->bytesAvailable(); }

    virtual bool        flush() override                                      { return m_file->flush(); }

    // Seeking
    virtual int         seek(int offset, int origin=Seek_Set) override        { return m_file->seek(offset,origin); }
    virtual SInt64      seek64(SInt64 offset, int origin=Seek_Set) override    { return m_file->seek64(offset,origin); }

    virtual int         copyFromStream(VFile *pstream, int byteSize) override  { return m_file->copyFromStream(pstream,byteSize); }

    // Closing the file
    virtual bool        close() override                                      { return m_file->close(); }
};
NV_NAMESPACE_END


