#pragma once

#include "VByteArray.h"
#include "VFlags.h"
#include "VString.h"

NV_NAMESPACE_BEGIN

class VIODevice
{
public:
    enum OpenModeFlag
    {
        NotOpen = 0x0,
        ReadOnly = 0x1,
        WriteOnly = 0x2,
        ReadWrite = ReadOnly | WriteOnly,
        Append = 0x4,
        Truncate = 0x8,
        Text = 0x10,
        Unbuffered = 0x20 //Unused yet
    };
    typedef VFlags<OpenModeFlag> OpenMode;

    VIODevice();
    virtual ~VIODevice();

    virtual bool atEnd() const { return bytesAvailable() == 0; }
    virtual vint64 bytesAvailable() const { return 0; }
    virtual vint64 bytesToWrite() const { return 0; }

    VString errorString() const;

    virtual bool isSequential() const { return false; }

    //Automatically translate \r\n into \n on reading and \n into \r\n on writing
    bool isTextModeEnabled() const;
    void setTextModeEnabled(bool enabled);

    bool isReadable() const;
    bool isWritable() const;

    virtual bool open(OpenMode mode);
    bool isOpen() const;
    OpenMode openMode() const;

    virtual void close();

    virtual vint64 pos() const;
    virtual bool reset();
    virtual bool seek(vint64 pos);

    virtual vint64 size() const { return isSequential() ? bytesAvailable() : 0; }

    vint64 read(char *data, vint64 maxSize) { return readData(data, maxSize); }
    VByteArray read(vint64 maxSize);
    VByteArray readAll();

    vint64 write(const char *data, vint64 maxSize) { return writeData(data, maxSize); }
    vint64 write(const char *data);
    vint64 write(const VByteArray &byteArray) { return writeData(byteArray.data(), byteArray.size()); }

protected:
    virtual vint64 readData(char *data, vint64 maxSize) = 0;
    virtual vint64 writeData(const char *data, vint64 maxSize) = 0;

    void setErrorString(const VString &str);
    void setOpenMode(OpenMode openMode);

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VIODevice)
};

NV_NAMESPACE_END
