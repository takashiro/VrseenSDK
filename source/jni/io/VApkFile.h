#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

class VString;
class VByteArray;

class VApkFile
{
public:
    VApkFile();
    VApkFile(const VString &packageName);
    ~VApkFile();

    bool open(const VString &packageName);
    bool isOpen() const;
    void close();

    bool contains(const VString &filePath) const;

    void read(const VString &filePath, void *&buffer, uint &length) const;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VApkFile)
};

NV_NAMESPACE_END
