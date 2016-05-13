#pragma once

#include "VIODevice.h"
#include "core/VArray.h"

NV_NAMESPACE_BEGIN

class VBuffer : public VIODevice
{
public:
    VBuffer();
    VBuffer( const uchar * bData, const int bSize );
    ~VBuffer();

    vint64 bytesAvailable() const override;

    template< typename _type_ >
    bool readArray( VArray< _type_ > & oArray, const int num ) const
    {
        const int byteNum = num * sizeof(oArray[0]);
        if (m_data == NULL || byteNum > m_size - m_offset) {
            oArray.resize(0);
            return false;
        }
        oArray.resize(num);
        memcpy(&oArray[0], &m_data[m_offset], byteNum);

        m_offset += byteNum;
        return true;
    }
    uint readUint() const
    {
        const int byteNum = sizeof(uint);
        if (m_data == NULL || byteNum > m_size - m_offset) {
            return 0;
        }
        m_offset += byteNum;
        return *(uint *)(m_data + m_offset - byteNum);
    }
    bool isEnd() const
    {
        return (m_offset == m_size);
    }
protected:
    vint64 readData(char *data, vint64 maxSize) override;
    vint64 writeData(const char *data, vint64 maxSize) override;

private:
    const uchar* m_data;
    int m_size;
    mutable int m_offset;
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VBuffer)
};

NV_NAMESPACE_END
