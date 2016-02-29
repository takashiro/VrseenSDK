#include "VStringBuffer.h"

NV_NAMESPACE_BEGIN

#define OVR_SBUFF_DEFAULT_GROW_SIZE 512
// Constructors / Destructor.
VStringBuffer::VStringBuffer()
    : m_data(NULL), m_size(0), m_bufferSize(0), m_growSize(OVR_SBUFF_DEFAULT_GROW_SIZE), m_lengthIsSize(false)
{
}

VStringBuffer::VStringBuffer(UPInt growSize)
    : m_data(NULL), m_size(0), m_bufferSize(0), m_growSize(OVR_SBUFF_DEFAULT_GROW_SIZE), m_lengthIsSize(false)
{
    setGrowSize(growSize);
}

VStringBuffer::VStringBuffer(const char* data)
    : m_data(NULL), m_size(0), m_bufferSize(0), m_growSize(OVR_SBUFF_DEFAULT_GROW_SIZE), m_lengthIsSize(false)
{
    append(data);
}

VStringBuffer::VStringBuffer(const char* data, UPInt dataSize)
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
void VStringBuffer::setGrowSize(UPInt growSize)
{
    if (growSize <= 16)
        m_growSize = 16;
    else
    {
        UByte bits = Alg::UpperBit(UInt32(growSize-1));
        UPInt size = 1<<bits;
        m_growSize = size == growSize ? growSize : size;
    }
}

UPInt VStringBuffer::length() const
{
    UPInt length, size = this->size();
    if (m_lengthIsSize)
        return size;

    length = (UPInt)UTF8Util::GetLength(m_data, (UPInt) this->size());

    if (length == this->size())
        m_lengthIsSize = true;
    return length;
}

void    VStringBuffer::reserve(UPInt _size)
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
void    VStringBuffer::resize(UPInt _size)
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
    UPInt   origSize = size();

    // Converts ch into UTF8 string and fills it into buff. Also increments index according to the number of bytes
    // in the UTF8 string.
    SPInt   srcSize = 0;
    UTF8Util::EncodeChar(buff, &srcSize, ch);
    OVR_ASSERT(srcSize >= 0);

    UPInt size = origSize + srcSize;
    resize(size);
    memcpy(m_data + origSize, buff, srcSize);
}

// Append a string
void     VStringBuffer::append(const wchar_t* pstr, SPInt len)
{
    if (!pstr)
        return;

    SPInt   srcSize     = UTF8Util::GetEncodeStringSize(pstr, len);
    UPInt   origSize    = size();
    UPInt   size        = srcSize + origSize;

    resize(size);
    UTF8Util::EncodeString(m_data + origSize,  pstr, len);
}

void      VStringBuffer::append(const char* putf8str, SPInt utf8StrSz)
{
    if (!putf8str || !utf8StrSz)
        return;
    if (utf8StrSz == -1)
        utf8StrSz = (SPInt)strlen(putf8str);

    UPInt   origSize    = size();
    UPInt   size        = utf8StrSz + origSize;

    resize(size);
    memcpy(m_data + origSize, putf8str, utf8StrSz);
}


void      VStringBuffer::operator = (const char* pstr)
{
    pstr = pstr ? pstr : "";
    UPInt size = strlen(pstr);
    resize(size);
    memcpy(m_data, pstr, size);
}

void      VStringBuffer::operator = (const wchar_t* pstr)
{
    pstr = pstr ? pstr : L"";
    UPInt size = (UPInt)UTF8Util::GetEncodeStringSize(pstr);
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
void      VStringBuffer::insert(const char* substr, UPInt posAt, SPInt len)
{
    UPInt     oldSize    = m_size;
    UPInt     insertSize = (len < 0) ? strlen(substr) : (UPInt)len;
    UPInt     byteIndex  = m_lengthIsSize ? posAt :
                           (UPInt)UTF8Util::GetByteIndex(posAt, m_data, (SPInt)m_size);

    OVR_ASSERT(byteIndex <= oldSize);
    reserve(oldSize + insertSize);

    memmove(m_data + byteIndex + insertSize, m_data + byteIndex, oldSize - byteIndex + 1);
    memcpy (m_data + byteIndex, substr, insertSize);
    m_lengthIsSize = false;
    m_size = oldSize + insertSize;
    m_data[m_size] = 0;
}

// Inserts character at posAt
UPInt     VStringBuffer::insert(UInt32 c, UPInt posAt)
{
    char    buf[8];
    SPInt   len = 0;
    UTF8Util::EncodeChar(buf, &len, c);
    OVR_ASSERT(len >= 0);
    buf[(UPInt)len] = 0;

    insert(buf, posAt, len);
    return (UPInt)len;
}

NV_NAMESPACE_END
