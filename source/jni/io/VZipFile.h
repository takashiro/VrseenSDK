#pragma once

#include "VIODevice.h"
#include "VByteArray.h"

NV_NAMESPACE_BEGIN

class VString;

class VZipFile
{
public:
    VZipFile();
    VZipFile(const VString &packageName);
    ~VZipFile();

    bool open(const VString &packageName);
    bool isOpen() const;
    void close();

    bool contains(const VString &filePath) const;

    bool read(const VString &filePath, void *&buffer, uint &length) const;
    bool read(const VString &filePath, VIODevice *output) const;
    VByteArray read(const VString &filePath) const;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VZipFile)
};

NV_NAMESPACE_END
