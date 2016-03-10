#include "VApkFile.h"
#include "VString.h"
#include "VByteArray.h"
#include "VLog.h"

#include "AppLocal.h"
#include "MemBuffer.h"

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

void VApkFile::read(const VString &filePath, void *&buffer, uint &length) const
{
    if (d->handle == nullptr) {
        vError("VApkFile is not open");
        return;
    }

    VByteArray path = filePath.toUtf8();
    vInfo("nameInZip is" << path);

    const int locateRet = unzLocateFile(d->handle, path.data(), 2 /* case insensitive */);
    if (locateRet != UNZ_OK) {
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

bool VApkFile::read(const VString &filePath, MemBufferFile &memBufferFile) const
{
    memBufferFile.freeData();

    uint length = 0;
    void *buffer = nullptr;
    read(filePath, buffer, length);

    if (buffer != nullptr) {
        memBufferFile.buffer = buffer;
        memBufferFile.length = length;
        return true;
    }

    return false;
}

const VApkFile &VApkFile::CurrentApkFile()
{
    static VApkFile current(vApp->packageCodePath());
    return current;
}

NV_NAMESPACE_END
