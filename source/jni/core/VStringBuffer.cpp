#include "VStringBuffer.h"

NV_NAMESPACE_BEGIN

#define OVR_SBUFF_DEFAULT_GROW_SIZE 512
// Constructors / Destructor.
VStringBuffer::VStringBuffer()
    : m_data(NULL), m_size(0), m_bufferSize(0), m_growSize(OVR_SBUFF_DEFAULT_GROW_SIZE), m_lengthIsSize(false)
{
}

VStringBuffer::VStringBuffer(uint growSize)
    : m_data(NULL), m_size(0), m_bufferSize(0), m_growSize(OVR_SBUFF_DEFAULT_GROW_SIZE), m_lengthIsSize(false)
{
    setGrowSize(growSize);
}

VStringBuffer::VStringBuffer(const char* data)
    : m_data(NULL), m_size(0), m_bufferSize(0), m_growSize(OVR_SBUFF_DEFAULT_GROW_SIZE), m_lengthIsSize(false)
{
    append(data);
}

VStringBuffer::VStringBuffer(const char* data, uint dataSize)
    : m_data(NULL), m_size(0), m_bufferSize(0), m_growSize(OVR_SBUFF_DEFAULT_GROW_SIZE), m_lengthIsSize(false)
{
    append(data, dataSize);
}

VStringBuffer::VStringBuffer(const VString& src)
    : m_data(NULL), m_size(0), m_bufferSize(0), m_growSize(OVR_SBUFF_DEFAULT_GROW_SIZE), m_lengthIsSize(false)
{
    append(src.toCString(), src.size());
}

VStringBuffer::VStringBuffer(const VStringBuffer& src)
    : m_data(NULL), m_size(0), m_bufferSize(0), m_growSize(OVR_SBUFF_DEFAULT_GROW_SIZE), m_lengthIsSize(false)
{
    append(src.toCString(), src.size());
}

VStringBuffer::VStringBuffer(const wchar_t* data)
    : m_data(NULL), m_size(0), m_bufferSize(0), m_growSize(OVR_SBUFF_DEFAULT_GROW_SIZE), m_lengthIsSize(false)
{
    *this = data;
}

VStringBuffer::~VStringBuffer()
{
    if (m_data)
        OVR_FREE(m_data);
}
void VStringBuffer::setGrowSize(uint growSize)
{
    if (growSize <= 16)
        m_growSize = 16;
    else
    {
        UByte bits = Alg::UpperBit(UInt32(growSize-1));
        uint size = 1<<bits;
        m_growSize = size == growSize ? growSize : size;
    }
}

uint VStringBuffer::length() const
{
    uint length, size = this->size();
    if (m_lengthIsSize)
        return size;

    length = (uint)UTF8Util::GetLength(m_data, (uint) this->size());

    if (length == this->size())
        m_lengthIsSize = true;
    return length;
}

void    VStringBuffer::reserve(uint _size)
{
    if (_size >= m_bufferSize) // >= because of trailing zero! (!AB)
    {
        m_bufferSize = (_size + 1 + m_growSize - 1)& ~(m_growSize-1);
        if (!m_data)
            m_data = (char*)OVR_ALLOC(m_bufferSize);
        else
            m_data = (char*)OVR_REALLOC(m_data, m_bufferSize);
    }
}
void    VStringBuffer::resize(uint _size)
{
    reserve(_size);
    m_lengthIsSize = false;
    m_size = _size;
    if (m_data)
        m_data[m_size] = 0;
}

void VStringBuffer::clear()
{
    resize(0);
    /*
    if (pData != pEmptyNullData)
    {
        OVR_FREE(pHeap, pData);
        pData = pEmptyNullData;
        Size = BufferSize = 0;
        LengthIsSize = false;
    }
    */
}
// Appends a character
void     VStringBuffer::append(UInt32 ch)
{
    char    buff[8];
    uint   origSize = size();

    // Converts ch into UTF8 string and fills it into buff. Also increments index according to the number of bytes
    // in the UTF8 string.
    int   srcSize = 0;
    UTF8Util::EncodeChar(buff, &srcSize, ch);
    OVR_ASSERT(srcSize >= 0);

    uint size = origSize + srcSize;
    resize(size);
    memcpy(m_data + origSize, buff, srcSize);
}

// Append a string
void     VStringBuffer::append(const wchar_t* pstr, int len)
{
    if (!pstr)
        return;

    int   srcSize     = UTF8Util::GetEncodeStringSize(pstr, len);
    uint   origSize    = size();
    uint   size        = srcSize + origSize;

    resize(size);
    UTF8Util::EncodeString(m_data + origSize,  pstr, len);
}

void      VStringBuffer::append(const char* putf8str, int utf8StrSz)
{
    if (!putf8str || !utf8StrSz)
        return;
    if (utf8StrSz == -1)
        utf8StrSz = (int)strlen(putf8str);

    uint   origSize    = size();
    uint   size        = utf8StrSz + origSize;

    resize(size);
    memcpy(m_data + origSize, putf8str, utf8StrSz);
}


void      VStringBuffer::operator = (const char* pstr)
{
    pstr = pstr ? pstr : "";
    uint size = strlen(pstr);
    resize(size);
    memcpy(m_data, pstr, size);
}

void      VStringBuffer::operator = (const wchar_t* pstr)
{
    pstr = pstr ? pstr : L"";
    uint size = (uint)UTF8Util::GetEncodeStringSize(pstr);
    resize(size);
    UTF8Util::EncodeString(m_data, pstr);
}

void      VStringBuffer::operator = (const VString& src)
{
    resize(src.size());
    memcpy(m_data, src.toCString(), src.size());
}

void      VStringBuffer::operator = (const VStringBuffer& src)
{
    clear();
    append(src.toCString(), src.size());
}


// Inserts substr at posAt
void      VStringBuffer::insert(const char* substr, uint posAt, int len)
{
    uint     oldSize    = m_size;
    uint     insertSize = (len < 0) ? strlen(substr) : (uint)len;
    uint     byteIndex  = m_lengthIsSize ? posAt :
                           (uint)UTF8Util::GetByteIndex(posAt, m_data, (int)m_size);

    OVR_ASSERT(byteIndex <= oldSize);
    reserve(oldSize + insertSize);

    memmove(m_data + byteIndex + insertSize, m_data + byteIndex, oldSize - byteIndex + 1);
    memcpy (m_data + byteIndex, substr, insertSize);
    m_lengthIsSize = false;
    m_size = oldSize + insertSize;
    m_data[m_size] = 0;
}

// Inserts character at posAt
uint     VStringBuffer::insert(UInt32 c, uint posAt)
{
    char    buf[8];
    int   len = 0;
    UTF8Util::EncodeChar(buf, &len, c);
    OVR_ASSERT(len >= 0);
    buf[(uint)len] = 0;

    insert(buf, posAt, len);
    return (uint)len;
}

NV_NAMESPACE_END
