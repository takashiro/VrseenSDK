#include "VBuffer.h"

#include <sstream>

NV_NAMESPACE_BEGIN

struct VBuffer::Private
{
    std::stringstream data;
};

VBuffer::VBuffer()
    : d(new Private)
{
}

VBuffer::~VBuffer()
{
    delete d;
}

vint64 VBuffer::bytesAvailable() const
{
    vint64 curPos = d->data.tellg();
    d->data.seekg(0, std::ios::end);
    vint64 pos = d->data.tellg();
    d->data.seekg(curPos);
    return pos - curPos;
}

vint64 VBuffer::size() const
{
    std::streampos curPos = d->data.tellg();
    d->data.seekg(0, std::ios::end);
    vint64 pos = d->data.tellg();
    d->data.seekg(curPos);
    return pos;
}

vint64 VBuffer::readData(char *data, vint64 maxSize)
{
    return d->data.readsome(data, maxSize);

}

vint64 VBuffer::writeData(const char *data, vint64 maxSize)
{
    d->data.write(data, maxSize);
    return maxSize;
}

NV_NAMESPACE_END
