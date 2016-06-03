#pragma once

#include "VIODevice.h"
#include "VByteArray.h"

//@to-to: remove this
#include "VTexture.h"

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

    static const VZipFile &CurrentApkFile();

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VZipFile)
};

//@to-do: remove this function
uint LoadTextureFromApplicationPackage(const VString &nameInZip, const TextureFlags_t &flags, int & width, int & height);

NV_NAMESPACE_END
