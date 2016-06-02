#include "VZipFile.h"
#include "VString.h"
#include "VByteArray.h"
#include "VLog.h"

#include "App.h"

#include <3rdparty/minizip/unzip.h>

NV_NAMESPACE_BEGIN

struct VZipFile::Private
{
    unzFile handle;
};

VZipFile::VZipFile()
    : d(new Private)
{
    d->handle = nullptr;
}

VZipFile::VZipFile(const VString &packageName)
    : d(new Private)
{
    open(packageName);
}

VZipFile::~VZipFile()
{
    close();
    delete d;
}

bool VZipFile::open(const VString &packageName)
{
    VByteArray latin1 = packageName.toLatin1();
    vInfo("VZipFile is opening" << latin1);
    d->handle = unzOpen(latin1.c_str());
    return d->handle != nullptr;
}

bool VZipFile::isOpen() const
{
    return d->handle != nullptr;
}

void VZipFile::close()
{
    if (d->handle) {
        unzClose(d->handle);
    }
}

bool VZipFile::contains(const VString &filePath) const
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

bool VZipFile::read(const VString &filePath, void *&buffer, uint &length) const
{
    if (d->handle == nullptr) {
        vError("VZipFile is not open");
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

bool VZipFile::read(const VString &filePath, VIODevice *output) const
{
    if (d->handle == nullptr) {
        vError("VZipFile is not open");
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


const VZipFile &VZipFile::CurrentApkFile()
{
    static VZipFile current(vApp->packageCodePath());
    return current;
}

uint LoadTextureFromApplicationPackage(const VString &nameInZip, const TextureFlags_t &flags, int &width, int &height)
{
    width = 0;
    height = 0;

    void *buffer = nullptr;
    uint bufferLength;

    const VZipFile &apk = VZipFile::CurrentApkFile();
    apk.read(nameInZip, buffer, bufferLength);
    if (buffer == nullptr) {
        return 0;
    }
    VByteArray name = nameInZip.toUtf8();
    unsigned texId = LoadTextureFromBuffer(name.data(), buffer, bufferLength, flags, width, height);

    return texId;
}

NV_NAMESPACE_END
