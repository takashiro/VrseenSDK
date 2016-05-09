#include "VFile.h"
#include "VLog.h"

#include <fstream>

#include <unistd.h>
#include <sys/stat.h>

NV_NAMESPACE_BEGIN

struct VFile::Private
{
    VString path;
    std::fstream data;
};

VFile::VFile()
    : d(new Private)
{
}

VFile::VFile(const VString &path, VIODevice::OpenMode mode)
    : d(new Private)
{
    d->path = path;
    open(mode);
}

VFile::~VFile()
{
    delete d;
}

const VString &VFile::path() const
{
    return d->path;
}

bool VFile::open(const VString &path, VIODevice::OpenMode mode)
{
    d->path = path;
    return open(mode);
}

bool VFile::open(VIODevice::OpenMode mode)
{
    std::ios_base::openmode std_mode;
    if (mode & ReadOnly) {
        std_mode = std::ios_base::in;
    } else if (mode & WriteOnly) {
        std_mode = std::ios_base::out;
    } else {
        vAssert(false);
    }

    if (mode & Append) {
        std_mode |= std::ios_base::app;
    }
    if (!(mode & Text)) {
        std_mode |= std::ios_base::binary;
    }
    if (mode & Truncate) {
        std_mode |= std::ios_base::trunc;
    }
    d->data.open(d->path.toUtf8(), std_mode);

    return d->data.is_open() && VIODevice::open(mode);
}

void VFile::close()
{
    d->data.close();
}

bool VFile::exists() const
{
    return Exists(d->path);
}

bool VFile::Exists(const VString &path)
{
    struct stat buffer;
    return stat(path.toUtf8().data(), &buffer) == 0;
}

bool VFile::IsReadable(const VString &path)
{
    return access(path.toUtf8().data(), R_OK);
}

bool VFile::IsWritable(const VString &path)
{
    return access(path.toUtf8().data(), W_OK);
}

vint64 VFile::readData(char *data, vint64 maxSize)
{
    return d->data.readsome(data, maxSize);
}

vint64 VFile::writeData(const char *data, vint64 maxSize)
{
    d->data.write(data, maxSize);
    return maxSize;
}

NV_NAMESPACE_END
