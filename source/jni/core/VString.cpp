/************************************************************************************

Filename    :   OVR_String.cpp
Content     :   String UTF8 string implementation with copy-on-write semantics
                (thread-safe for assignment but not modification).
Created     :   September 19, 2012
Notes       :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#include "VString.h"

#include <stdlib.h>
#include <ctype.h>

#ifdef OVR_OS_QNX
# include <strings.h>
#endif

namespace NervGear {

#define String_LengthIsSize (UPInt(1) << String::Flag_LengthIsSizeShift)

String::DataDesc String::NullData = {String_LengthIsSize, 1, {0} };


String::String()
{
    pData = &NullData;
    pData->addRef();
};

String::String(const char* pdata)
{
    // Obtain length in bytes; it doesn't matter if _data is UTF8.
    UPInt size = pdata ? OVR_strlen(pdata) : 0;
    pData = allocDataCopy1(size, 0, pdata, size);
};

String::String(const char* pdata1, const char* pdata2, const char* pdata3)
{
    // Obtain length in bytes; it doesn't matter if _data is UTF8.
    UPInt size1 = pdata1 ? OVR_strlen(pdata1) : 0;
    UPInt size2 = pdata2 ? OVR_strlen(pdata2) : 0;
    UPInt size3 = pdata3 ? OVR_strlen(pdata3) : 0;

    DataDesc *pdataDesc = allocDataCopy2(size1 + size2 + size3, 0,
                                         pdata1, size1, pdata2, size2);
    memcpy(pdataDesc->data + size1 + size2, pdata3, size3);
    pData = pdataDesc;
}

String::String(const char* pdata, UPInt size)
{
    OVR_ASSERT((size == 0) || (pdata != 0));
    pData = allocDataCopy1(size, 0, pdata, size);
}

String::String(const String& src)
{
    pData = src.data();
    pData->addRef();
}

String::String(const StringBuffer& src)
{
    pData = allocDataCopy1(src.size(), 0, src.toCString(), src.size());
}

String::String(const wchar_t* data)
{
    pData = &NullData;
    pData->addRef();
    // Simplified logic for wchar_t constructor.
    if (data)
        *this = data;
}


String::DataDesc* String::allocData(UPInt size, UPInt lengthIsSize)
{
    String::DataDesc* pdesc;

    if (size == 0)
    {
        pdesc = &NullData;
        pdesc->addRef();
        return pdesc;
    }

    pdesc = (DataDesc*)OVR_ALLOC(sizeof(DataDesc)+ size);
    pdesc->data[size] = 0;
    pdesc->refCount = 1;
    pdesc->m_size     = size | lengthIsSize;
    return pdesc;
}


String::DataDesc* String::allocDataCopy1(UPInt size, UPInt lengthIsSize,
                                         const char* pdata, UPInt copySize)
{
    String::DataDesc* pdesc = allocData(size, lengthIsSize);
    memcpy(pdesc->data, pdata, copySize);
    return pdesc;
}

String::DataDesc* String::allocDataCopy2(UPInt size, UPInt lengthIsSize,
                                         const char* pdata1, UPInt copySize1,
                                         const char* pdata2, UPInt copySize2)
{
    String::DataDesc* pdesc = allocData(size, lengthIsSize);
    memcpy(pdesc->data, pdata1, copySize1);
    memcpy(pdesc->data + copySize1, pdata2, copySize2);
    return pdesc;
}


UPInt String::length() const
{
    // Optimize length accesses for non-UTF8 character strings.
    DataDesc* pdata = data();
    UPInt     length, size = pdata->size();

    if (pdata->lengthIsSize())
        return size;

    length = (UPInt)UTF8Util::GetLength(pdata->data, (UPInt)size);

    if (length == size)
        pdata->m_size |= String_LengthIsSize;

    return length;
}


//static UInt32 String_CharSearch(const char* buf, )


UInt32 String::at(UPInt index) const
{
    SPInt       i = (SPInt) index;
    DataDesc*   pdata = data();
    const char* buf = pdata->data;
    UInt32      c;

    if (pdata->lengthIsSize())
    {
        OVR_ASSERT(index < pdata->size());
        buf += i;
        return UTF8Util::DecodeNextChar_Advance0(&buf);
    }

    c = UTF8Util::GetCharAt(index, buf, pdata->size());
    return c;
}

UInt32 String::firstCharAt(UPInt index, const char** offset) const
{
    DataDesc*   pdata = data();
    SPInt       i = (SPInt) index;
    const char* buf = pdata->data;
    const char* end = buf + pdata->size();
    UInt32      c;

    do
    {
        c = UTF8Util::DecodeNextChar_Advance0(&buf);
        i--;

        if (buf >= end)
        {
            // We've hit the end of the string; don't go further.
            OVR_ASSERT(i == 0);
            return c;
        }
    } while (i >= 0);

    *offset = buf;

    return c;
}

UInt32 String::nextChar(const char** offset) const
{
    return UTF8Util::DecodeNextChar(offset);
}



void String::append(UInt32 ch)
{
    DataDesc*   pdata = data();
    UPInt       size = pdata->size();
    char        buff[8];
    SPInt       encodeSize = 0;

    // Converts ch into UTF8 string and fills it into buff.
    UTF8Util::EncodeChar(buff, &encodeSize, ch);
    OVR_ASSERT(encodeSize >= 0);

    setData(allocDataCopy2(size + (UPInt)encodeSize, 0,
                           pdata->data, size, buff, (UPInt)encodeSize));
    pdata->release();
}


void String::append(const wchar_t* pstr, SPInt len)
{
    if (!pstr)
        return;

    DataDesc*   pdata = data();
    UPInt       oldSize = pdata->size();
    UPInt       encodeSize = (UPInt)UTF8Util::GetEncodeStringSize(pstr, len);

    DataDesc*   pnewData = allocDataCopy1(oldSize + (UPInt)encodeSize, 0,
                                          pdata->data, oldSize);
    UTF8Util::EncodeString(pnewData->data + oldSize,  pstr, len);

    setData(pnewData);
    pdata->release();
}


void String::append(const char* putf8str, SPInt utf8StrSz)
{
    if (!putf8str || !utf8StrSz)
        return;
    if (utf8StrSz == -1)
        utf8StrSz = (SPInt)OVR_strlen(putf8str);

    DataDesc*   pdata = data();
    UPInt       oldSize = pdata->size();

    setData(allocDataCopy2(oldSize + (UPInt)utf8StrSz, 0,
                           pdata->data, oldSize, putf8str, (UPInt)utf8StrSz));
    pdata->release();
}

void    String::assign(const char* putf8str, UPInt size)
{
    DataDesc* poldData = data();
    setData(allocDataCopy1(size, 0, putf8str, size));
    poldData->release();
}

void    String::operator = (const char* pstr)
{
    assign(pstr, pstr ? OVR_strlen(pstr) : 0);
}

void    String::operator = (const wchar_t* pwstr)
{
    DataDesc*   poldData = data();
    UPInt       size = pwstr ? (UPInt)UTF8Util::GetEncodeStringSize(pwstr) : 0;

    DataDesc*   pnewData = allocData(size, 0);
    UTF8Util::EncodeString(pnewData->data, pwstr);
    setData(pnewData);
    poldData->release();
}


void    String::operator = (const String& src)
{
    DataDesc*    psdata = src.data();
    DataDesc*    pdata = data();

    setData(psdata);
    psdata->addRef();
    pdata->release();
}


void    String::operator = (const StringBuffer& src)
{
    DataDesc* polddata = data();
    setData(allocDataCopy1(src.size(), 0, src.toCString(), src.size()));
    polddata->release();
}

void    String::operator += (const String& src)
{
    DataDesc   *pourData = data(),
               *psrcData = src.data();
    UPInt       ourSize  = pourData->size(),
                srcSize  = psrcData->size();
    UPInt       lflag    = pourData->lengthFlag() & psrcData->lengthFlag();

    setData(allocDataCopy2(ourSize + srcSize, lflag,
                           pourData->data, ourSize, psrcData->data, srcSize));
    pourData->release();
}


String   String::operator + (const char* str) const
{
    String tmp1(*this);
    tmp1 += (str ? str : "");
    return tmp1;
}

String   String::operator + (const String& src) const
{
    String tmp1(*this);
    tmp1 += src;
    return tmp1;
}

void    String::remove(UPInt posAt, SPInt removeLength)
{
    DataDesc*   pdata = data();
    UPInt       oldSize = pdata->size();
    // Length indicates the number of characters to remove.
    UPInt       length = this->length();

    // If index is past the string, nothing to remove.
    if (posAt >= length)
        return;
    // Otherwise, cap removeLength to the length of the string.
    if ((posAt + removeLength) > length)
        removeLength = length - posAt;

    // Get the byte position of the UTF8 char at position posAt.
    SPInt bytePos    = UTF8Util::GetByteIndex(posAt, pdata->data, oldSize);
    SPInt removeSize = UTF8Util::GetByteIndex(removeLength, pdata->data + bytePos, oldSize-bytePos);

    setData(allocDataCopy2(oldSize - removeSize, pdata->lengthFlag(),
                           pdata->data, bytePos,
                           pData->data + bytePos + removeSize, (oldSize - bytePos - removeSize)));
    pdata->release();
}


String   String::mid(UPInt start, UPInt end) const
{
    UPInt length = this->length();
    if ((start >= length) || (start >= end))
        return String();

    DataDesc* pdata = data();

    // If size matches, we know the exact index range.
    if (pdata->lengthIsSize())
        return String(pdata->data + start, end - start);

    // Get position of starting character.
    SPInt byteStart = UTF8Util::GetByteIndex(start, pdata->data, pdata->size());
    SPInt byteSize  = UTF8Util::GetByteIndex(end - start, pdata->data + byteStart, pdata->size()-byteStart);
    return String(pdata->data + byteStart, (UPInt)byteSize);
}

void String::clear()
{
    NullData.addRef();
    data()->release();
    setData(&NullData);
}


String   String::toUpper() const
{
    UInt32      c;
    const char* psource = data()->data;
    const char* pend = psource + data()->size();
    String      str;
    SPInt       bufferOffset = 0;
    char        buffer[512];

    while(psource < pend)
    {
        do {
            c = UTF8Util::DecodeNextChar_Advance0(&psource);
            UTF8Util::EncodeChar(buffer, &bufferOffset, OVR_towupper(wchar_t(c)));
        } while ((psource < pend) && (bufferOffset < SPInt(sizeof(buffer)-8)));

        // Append string a piece at a time.
        str.append(buffer, bufferOffset);
        bufferOffset = 0;
    }

    return str;
}

String   String::toLower() const
{
    UInt32      c;
    const char* psource = data()->data;
    const char* pend = psource + data()->size();
    String      str;
    SPInt       bufferOffset = 0;
    char        buffer[512];

    while(psource < pend)
    {
        do {
            c = UTF8Util::DecodeNextChar_Advance0(&psource);
            UTF8Util::EncodeChar(buffer, &bufferOffset, OVR_towlower(wchar_t(c)));
        } while ((psource < pend) && (bufferOffset < SPInt(sizeof(buffer)-8)));

        // Append string a piece at a time.
        str.append(buffer, bufferOffset);
        bufferOffset = 0;
    }

    return str;
}



String& String::insert(const char* substr, UPInt posAt, SPInt strSize)
{
    DataDesc* poldData   = data();
    UPInt     oldSize    = poldData->size();
    UPInt     insertSize = (strSize < 0) ? OVR_strlen(substr) : (UPInt)strSize;
    UPInt     byteIndex  =  (poldData->lengthIsSize()) ?
                            posAt : (UPInt)UTF8Util::GetByteIndex(posAt, poldData->data, oldSize);

    OVR_ASSERT(byteIndex <= oldSize);

    DataDesc* pnewData = allocDataCopy2(oldSize + insertSize, 0,
                                        poldData->data, byteIndex, substr, insertSize);
    memcpy(pnewData->data + byteIndex + insertSize,
           poldData->data + byteIndex, oldSize - byteIndex);
    setData(pnewData);
    poldData->release();
    return *this;
}

/*
String& String::Insert(const UInt32* substr, UPInt posAt, SPInt len)
{
    for (SPInt i = 0; i < len; ++i)
    {
        UPInt charw = InsertCharAt(substr[i], posAt);
        posAt += charw;
    }
    return *this;
}
*/

UPInt String::insert(UInt32 c, UPInt posAt)
{
    char    buf[8];
    SPInt   index = 0;
    UTF8Util::EncodeChar(buf, &index, c);
    OVR_ASSERT(index >= 0);
    buf[(UPInt)index] = 0;

    insert(buf, posAt, index);
    return (UPInt)index;
}

void String::stripTrailing(const char * s)
{
	const UPInt len = OVR_strlen(s);
    if (length() >= len && right(len) == s)
	{
        *this = left(length() - len);
	}
}

int String::CompareNoCase(const char* a, const char* b)
{
    return OVR_stricmp(a, b);
}

int String::CompareNoCase(const char* a, const char* b, SPInt len)
{
    if (len)
    {
        SPInt f,l;
        SPInt slen = len;
        const char *s = b;
        do {
            f = (SPInt)OVR_tolower((int)(*(a++)));
            l = (SPInt)OVR_tolower((int)(*(b++)));
        } while (--len && f && (f == l) && *b != 0);

        if (f == l && (len != 0 || *b != 0))
        {
            f = (SPInt)slen;
            l = (SPInt)OVR_strlen(s);
            return int(f - l);
        }

        return int(f - l);
    }
    else
        return (0-(int)OVR_strlen(b));
}

// ***** Implement hash static functions

// Hash function
UPInt String::BernsteinHashFunction(const void* pdataIn, UPInt size, UPInt seed)
{
    const UByte*    pdata   = (const UByte*) pdataIn;
    UPInt           h       = seed;
    while (size > 0)
    {
        size--;
        h = ((h << 5) + h) ^ (unsigned) pdata[size];
    }

    return h;
}

// Hash function, case-insensitive
UPInt String::BernsteinHashFunctionCIS(const void* pdataIn, UPInt size, UPInt seed)
{
    const UByte*    pdata = (const UByte*) pdataIn;
    UPInt           h = seed;
    while (size > 0)
    {
        size--;
        h = ((h << 5) + h) ^ OVR_tolower(pdata[size]);
    }

    // Alternative: "sdbm" hash function, suggested at same web page above.
    // h = 0;
    // for bytes { h = (h << 16) + (h << 6) - hash + *p; }
    return h;
}



// ***** String Buffer used for Building Strings


#define OVR_SBUFF_DEFAULT_GROW_SIZE 512
// Constructors / Destructor.
StringBuffer::StringBuffer()
    : m_data(NULL), m_size(0), m_bufferSize(0), m_growSize(OVR_SBUFF_DEFAULT_GROW_SIZE), m_lengthIsSize(false)
{
}

StringBuffer::StringBuffer(UPInt growSize)
    : m_data(NULL), m_size(0), m_bufferSize(0), m_growSize(OVR_SBUFF_DEFAULT_GROW_SIZE), m_lengthIsSize(false)
{
    setGrowSize(growSize);
}

StringBuffer::StringBuffer(const char* data)
    : m_data(NULL), m_size(0), m_bufferSize(0), m_growSize(OVR_SBUFF_DEFAULT_GROW_SIZE), m_lengthIsSize(false)
{
    append(data);
}

StringBuffer::StringBuffer(const char* data, UPInt dataSize)
    : m_data(NULL), m_size(0), m_bufferSize(0), m_growSize(OVR_SBUFF_DEFAULT_GROW_SIZE), m_lengthIsSize(false)
{
    append(data, dataSize);
}

StringBuffer::StringBuffer(const String& src)
    : m_data(NULL), m_size(0), m_bufferSize(0), m_growSize(OVR_SBUFF_DEFAULT_GROW_SIZE), m_lengthIsSize(false)
{
    append(src.toCString(), src.size());
}

StringBuffer::StringBuffer(const StringBuffer& src)
    : m_data(NULL), m_size(0), m_bufferSize(0), m_growSize(OVR_SBUFF_DEFAULT_GROW_SIZE), m_lengthIsSize(false)
{
    append(src.toCString(), src.size());
}

StringBuffer::StringBuffer(const wchar_t* data)
    : m_data(NULL), m_size(0), m_bufferSize(0), m_growSize(OVR_SBUFF_DEFAULT_GROW_SIZE), m_lengthIsSize(false)
{
    *this = data;
}

StringBuffer::~StringBuffer()
{
    if (m_data)
        OVR_FREE(m_data);
}
void StringBuffer::setGrowSize(UPInt growSize)
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

UPInt StringBuffer::length() const
{
    UPInt length, size = this->size();
    if (m_lengthIsSize)
        return size;

    length = (UPInt)UTF8Util::GetLength(m_data, (UPInt) this->size());

    if (length == this->size())
        m_lengthIsSize = true;
    return length;
}

void    StringBuffer::reserve(UPInt _size)
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
void    StringBuffer::resize(UPInt _size)
{
    reserve(_size);
    m_lengthIsSize = false;
    m_size = _size;
    if (m_data)
        m_data[m_size] = 0;
}

void StringBuffer::clear()
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
void     StringBuffer::append(UInt32 ch)
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
void     StringBuffer::append(const wchar_t* pstr, SPInt len)
{
    if (!pstr)
        return;

    SPInt   srcSize     = UTF8Util::GetEncodeStringSize(pstr, len);
    UPInt   origSize    = size();
    UPInt   size        = srcSize + origSize;

    resize(size);
    UTF8Util::EncodeString(m_data + origSize,  pstr, len);
}

void      StringBuffer::append(const char* putf8str, SPInt utf8StrSz)
{
    if (!putf8str || !utf8StrSz)
        return;
    if (utf8StrSz == -1)
        utf8StrSz = (SPInt)OVR_strlen(putf8str);

    UPInt   origSize    = size();
    UPInt   size        = utf8StrSz + origSize;

    resize(size);
    memcpy(m_data + origSize, putf8str, utf8StrSz);
}


void      StringBuffer::operator = (const char* pstr)
{
    pstr = pstr ? pstr : "";
    UPInt size = OVR_strlen(pstr);
    resize(size);
    memcpy(m_data, pstr, size);
}

void      StringBuffer::operator = (const wchar_t* pstr)
{
    pstr = pstr ? pstr : L"";
    UPInt size = (UPInt)UTF8Util::GetEncodeStringSize(pstr);
    resize(size);
    UTF8Util::EncodeString(m_data, pstr);
}

void      StringBuffer::operator = (const String& src)
{
    resize(src.size());
    memcpy(m_data, src.toCString(), src.size());
}

void      StringBuffer::operator = (const StringBuffer& src)
{
    clear();
    append(src.toCString(), src.size());
}


// Inserts substr at posAt
void      StringBuffer::insert(const char* substr, UPInt posAt, SPInt len)
{
    UPInt     oldSize    = m_size;
    UPInt     insertSize = (len < 0) ? OVR_strlen(substr) : (UPInt)len;
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
UPInt     StringBuffer::insert(UInt32 c, UPInt posAt)
{
    char    buf[8];
    SPInt   len = 0;
    UTF8Util::EncodeChar(buf, &len, c);
    OVR_ASSERT(len >= 0);
    buf[(UPInt)len] = 0;

    insert(buf, posAt, len);
    return (UPInt)len;
}

} // OVR
