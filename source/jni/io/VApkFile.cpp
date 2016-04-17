#include "VApkFile.h"
#include "VString.h"
#include "VByteArray.h"
#include "VLog.h"

#include "App.h"
#include "VImageManager.h"
#include "VOpenGLTexture.h"
#include <3rdparty/minizip/unzip.h>

NV_NAMESPACE_BEGIN

struct VApkFile::Private
{
    unzFile handle;
};

VApkFile::VApkFile()
    : d(new Private)
{
    d->handle = nullptr;
}

VApkFile::VApkFile(const VString &packageName)
    : d(new Private)
{
    open(packageName);
}

VApkFile::~VApkFile()
{
    close();
    delete d;
}

bool VApkFile::open(const VString &packageName)
{
    VByteArray latin1 = packageName.toLatin1();
    vInfo("VApkFile is opening" << latin1);
    d->handle = unzOpen(latin1.c_str());
    return d->handle != nullptr;
}

bool VApkFile::isOpen() const
{
    return d->handle != nullptr;
}

void VApkFile::close()
{
    if (d->handle) {
        unzClose(d->handle);
    }
}

bool VApkFile::contains(const VString &filePath) const
{
    VByteArray path = filePath.toUtf8();
    const int locateRet = unzLocateFile(d->handle, path.c_str(), 2/* case insensitive */);
    if (locateRet != UNZ_OK) {
        vInfo("File '" << path << "' not found in apk!");
        return false;
    }

    const int openRet = unzOpenCurrentFile(d->handle);
    if (openRet != UNZ_OK) {
        vWarn("Error opening file '" << path << "' from apk!");
        return false;
    }

    unzCloseCurrentFile(d->handle);
    return true;
}

bool VApkFile::read(const VString &filePath, void *&buffer, uint &length) const
{
    if (d->handle == nullptr) {
        vError("VApkFile is not open");
        return false;
    }

    VByteArray path = filePath.toUtf8();
    vInfo("nameInZip is" << path);

    const int locateRet = unzLocateFile(d->handle, path.data(), 2 /* case insensitive */);
    if (locateRet != UNZ_OK) {
        vInfo("File '" << path << "' not found in apk!");
        return false;
    }

    unz_file_info info;
    const int getRet = unzGetCurrentFileInfo(d->handle, &info, NULL, 0, NULL, 0, NULL, 0);
    if (getRet != UNZ_OK) {
        vWarn("File info error reading '" << path << "' from apk!");
        return false;
    }
    const int openRet = unzOpenCurrentFile(d->handle);
    if (openRet != UNZ_OK) {
        vWarn("Error opening file '" << path << "' from apk!");
        return false;
    }

    length = info.uncompressed_size;
    buffer = malloc(length);

    const int readRet = unzReadCurrentFile(d->handle, buffer, length);
    unzCloseCurrentFile(d->handle);
    if (readRet <= 0) {
        vWarn("Error reading file '" << path << "' from apk!");
        free(buffer);
        buffer = NULL;
        length = 0;
        return false;
    }
    return true;
}

bool VApkFile::read(const VString &filePath, VIODevice *output) const
{
    if (d->handle == nullptr) {
        vError("VApkFile is not open");
        return false;
    }

    VByteArray path = filePath.toUtf8();
    vInfo("nameInZip is" << path);

    const int locateRet = unzLocateFile(d->handle, path.data(), 2 /* case insensitive */);
    if (locateRet != UNZ_OK) {
        vInfo("File '" << path << "' not found in apk!");
        return false;
    }

    unz_file_info info;
    const int getRet = unzGetCurrentFileInfo(d->handle, &info, NULL, 0, NULL, 0, NULL, 0);
    if (getRet != UNZ_OK) {
        vWarn("File info error reading '" << path << "' from apk!");
        return false;
    }
    const int openRet = unzOpenCurrentFile(d->handle);
    if (openRet != UNZ_OK) {
        vWarn("Error opening file '" << path << "' from apk!");
        return false;
    }

    void *buffer = malloc(info.uncompressed_size);
    const int readRet = unzReadCurrentFile(d->handle, buffer, info.uncompressed_size);
    unzCloseCurrentFile(d->handle);
    if (readRet <= 0) {
        free(buffer);
        vWarn("Error reading file '" << path << "' from apk!");
        return false;
    }

    output->write(static_cast<char *>(buffer), info.uncompressed_size);
    free(buffer);

    return true;
}


const VApkFile &VApkFile::CurrentApkFile()
{
    static VApkFile current(vApp->packageCodePath());
    return current;
}

uint LoadTextureFromApplicationPackage(const VString &nameInZip, const TextureFlags_t &flags, int &width, int &height)
{
    width = 0;
    height = 0;

    void *buffer = nullptr;
    uint bufferLength;

    const VApkFile &apk = VApkFile::CurrentApkFile();
    apk.read(nameInZip, buffer, bufferLength);
    if (buffer == nullptr) {
        return 0;
    }
    VByteArray name = nameInZip.toUtf8();
    unsigned texId = LoadTextureFromBuffer(name.data(), buffer, bufferLength, flags, width, height );

//    TextureFlags_o flag;
//    switch(flags)
//    {
//        case TEXTUREFLAG_NO_DEFAULT:
//            flag = _NO_DEFAULT;
//        break;

//        case TEXTUREFLAG_USE_SRGB:
//            flag = _USE_SRGB;
//        break;

//        case TEXTUREFLAG_NO_MIPMAPS:
//            flag = _NO_MIPMAPS;
//        break;

//        default:
//        break;
//    }


//    VImageManager* imagemanager = new VImageManager();
//    VImage* image = imagemanager->loadImage(VPath(nameInZip));
//    width = image->getDimension().Width;
//    height = image->getDimension().Height;
//    texId = VOpenGLTexture(image, VPath(nameInZip), flag).getTextureName();

//    delete imagemanager;

    return texId;
}

NV_NAMESPACE_END
