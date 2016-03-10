#pragma once

#include "vglobal.h"

//@to-to: remove this
#include "GlTexture.h"

NV_NAMESPACE_BEGIN

class VString;
class VByteArray;

//@to-do: remove this
class MemBufferFile;

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

    //@to-do: remove this function
    bool read(const VString &filePath, MemBufferFile &mem) const;

    static const VApkFile &CurrentApkFile();

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VApkFile)
};

//@to-do: remove this function
uint LoadTextureFromApplicationPackage(const VString &nameInZip, const TextureFlags_t &flags, int & width, int & height);

NV_NAMESPACE_END
