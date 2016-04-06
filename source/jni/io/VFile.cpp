#include "VFile.h"
#include "VLog.h"

#include <fstream>

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
    if (mode.contains(ReadOnly)) {
        std_mode = std::ios_base::in;
    } else if (mode.contains(WriteOnly)) {
        std_mode = std::ios_base::out;
    } else {
        vAssert(false);
    }

    if (mode.contains(Append)) {
        std_mode |= std::ios_base::app;
    }
    if (!mode.contains(Text)) {
        std_mode |= std::ios_base::binary;
    }
    if (mode.contains(Truncate)) {
        std_mode |= std::ios_base::trunc;
    }
    d->data.open(d->path.toUtf8(), std_mode);

    return d->data.is_open() && VIODevice::open(mode);
}

void VFile::close()
{
    d->data.close();
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
