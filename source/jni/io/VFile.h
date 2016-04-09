#pragma once

#include "VIODevice.h"

NV_NAMESPACE_BEGIN

class VFile : public VIODevice
{
public:
    VFile();
    VFile(const VString &path, OpenMode mode);
    ~VFile();

    const VString &path() const;
    bool open(const VString &path, OpenMode mode);

    bool open(OpenMode mode) override;
    void close() override;

protected:
    vint64 readData(char *data, vint64 maxSize) override;
    vint64 writeData(const char *data, vint64 maxSize) override;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VFile)
};

NV_NAMESPACE_END
