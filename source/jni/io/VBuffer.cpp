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
VBuffer::VBuffer( const uchar * bData, const int bSize )
        : m_data(bData)
        , m_size(bSize)
        , m_offset(0)
    {

    }
VBuffer::~VBuffer()
{
    delete d;
}

uint VBuffer::readUint() const
{
    const int byteNum = sizeof(uint);
    if (m_data == NULL || byteNum > m_size - m_offset) {
        return 0;
    }
    m_offset += byteNum;
    return *(uint *)(m_data + m_offset - byteNum);
}
bool VBuffer::isEnd() const
{
    return (m_offset == m_size);
}

vint64 VBuffer::bytesAvailable() const
{
    std::streampos curPos = d->data.tellp();
    d->data.seekp(0, std::ios::end);
    vint64 pos = d->data.tellp();
    d->data.seekp(curPos);
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
