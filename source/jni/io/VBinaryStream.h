#pragma once

#include "VArray.h"
#include "VIODevice.h"

NV_NAMESPACE_BEGIN

class VBinaryStream
{
public:
    VBinaryStream(VIODevice *device);

    template<typename T>
    VBinaryStream &operator>>(T &element)
    {
        constexpr size_t length = sizeof(T);
        char buffer[length];
        uint count = 0;
        do {
            count += m_device->read(buffer + count, length - count);
        } while (count < length);
        element = *(reinterpret_cast<T *>(buffer));
        return *this;
    }

    template<typename T>
    VBinaryStream &operator<<(const T &element)
    {
        constexpr size_t length = sizeof(T);
        const char *data = reinterpret_cast<const char *>(&element);
        uint count = 0;
        do {
            count += m_device->write(data + count, length - count);
        } while (count < length);
        return *this;
    }

    template<typename T>
    bool read(VArray<T> &elements, uint num)
    {
        size_t byteNum = sizeof(T) * num;
        if (m_device->bytesAvailable() < byteNum) {
            return false;
        }

        elements.resize(num);

        uint count = 0;
        char *data = reinterpret_cast<char *>(elements.data());
        do {
            count += m_device->read(data + count, byteNum - count);
        } while (count < byteNum);

        return true;
    }

    bool atEnd() const { return m_device->atEnd(); }

private:
    VIODevice *m_device;
};

NV_NAMESPACE_END
