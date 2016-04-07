#include "VIODevice.h"

NV_NAMESPACE_BEGIN

struct VIODevice::Private
{
    VIODevice::OpenMode openMode;
    bool textModeEnabled;
    vint64 pos;
    VString errorString;

    Private()
        : openMode(VIODevice::NotOpen)
        , textModeEnabled(false)
        , pos(0)
    {
    }
};

VIODevice::VIODevice()
    : d(new Private)
{
}

VIODevice::~VIODevice()
{
    close();
    delete d;
}

VString VIODevice::errorString() const
{
    return d->errorString;
}

bool VIODevice::isTextModeEnabled() const
{
    return d->textModeEnabled;
}

void VIODevice::setTextModeEnabled(bool enabled)
{
    d->textModeEnabled = enabled;
}

bool VIODevice::isReadable() const
{
    return d->openMode & ReadOnly;
}

bool VIODevice::isWritable() const
{
    return d->openMode & WriteOnly;
}

bool VIODevice::open(VIODevice::OpenMode mode)
{
    setOpenMode(mode);
    return true;
}

bool VIODevice::isOpen() const
{
    return d->openMode != NotOpen;
}

VIODevice::OpenMode VIODevice::openMode() const
{
    return d->openMode;
}

void VIODevice::close()
{
    setOpenMode(NotOpen);
}

vint64 VIODevice::pos() const
{
    return d->pos;
}

bool VIODevice::reset()
{
    d->pos = 0;
    return true;
}

bool VIODevice::seek(vint64 pos)
{
    d->pos = pos;
    return true;
}

VByteArray VIODevice::read(vint64 maxSize)
{
    char *data = new char[maxSize];
    vint64 length = readData(data, maxSize);
    VByteArray array(data, length);
    delete[] data;
    return array;
}

VByteArray VIODevice::readAll()
{
    VByteArray data;

    char buffer[4096];
    vint64 length = 0;
    forever {
        length = readData(buffer, 4096);
        if (length <= 0) {
            break;
        }
        data.append(buffer, length);
    }
    return data;
}

vint64 VIODevice::write(const char *data)
{
    vint64 size = strlen(data);
    return writeData(data, size);
}

void VIODevice::setErrorString(const VString &str)
{
    d->errorString = str;
}

void VIODevice::setOpenMode(VIODevice::OpenMode openMode)
{
    d->openMode = openMode;
}

NV_NAMESPACE_END
