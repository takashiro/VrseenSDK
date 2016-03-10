#include "VApkFile.h"
#include "VString.h"
#include "VByteArray.h"
#include "VLog.h"

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
}

bool VApkFile::open(const VString &packageName)
{
    VByteArray utf8 = packageName.toUtf8();
    d->handle = unzOpen(utf8.data());
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
    const int locateRet = unzLocateFile(d->handle, path.data(), 2/* case insensitive */);
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

void VApkFile::read(const VString &filePath, void *&buffer, uint &length) const
{
    VByteArray path = filePath.toUtf8();
    vInfo("nameInZip is" << path);
    if (d->handle == nullptr) {
        return;
    }

    const int locateRet = unzLocateFile(d->handle, path.data(), 2 /* case insensitive */);
    if ( locateRet != UNZ_OK) {
        vInfo("File '" << path << "' not found in apk!");
        return;
    }

    unz_file_info info;
    const int getRet = unzGetCurrentFileInfo(d->handle, &info, NULL, 0, NULL, 0, NULL, 0);
    if (getRet != UNZ_OK) {
        vWarn("File info error reading '" << path << "' from apk!");
        return;
    }
    const int openRet = unzOpenCurrentFile(d->handle);
    if (openRet != UNZ_OK) {
        vWarn("Error opening file '" << path << "' from apk!");
        return;
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
    }
}

NV_NAMESPACE_END
