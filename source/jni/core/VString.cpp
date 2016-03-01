/************************************************************************************

Filename    :   OVR_String.cpp
Content     :   String UTF8 string implementation with copy-on-write semantics
                (thread-safe for assignment but not modification).
Created     :   September 19, 2012
Notes       :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#include <stdlib.h>
#include <ctype.h>

#include "VString.h"
#include "VStringBuffer.h"

namespace NervGear {

#define String_LengthIsSize (uint(1) << VString::Flag_LengthIsSizeShift)

VString::DataDesc VString::NullData = {String_LengthIsSize, 1, {0} };


VString::VString()
{
    pData = &NullData;
    pData->addRef();
};

VString::VString(const char* pdata)
{
    // Obtain length in bytes; it doesn't matter if _data is UTF8.
    uint size = pdata ? strlen(pdata) : 0;
    pData = allocDataCopy1(size, 0, pdata, size);
}

VString::VString(const std::string &data)
{
    uint size = data.length();
    pData = allocDataCopy1(size, 0, data.c_str(), size);
}

VString::VString(const char* pdata1, const char* pdata2, const char* pdata3)
{
    // Obtain length in bytes; it doesn't matter if _data is UTF8.
    uint size1 = pdata1 ? strlen(pdata1) : 0;
    uint size2 = pdata2 ? strlen(pdata2) : 0;
    uint size3 = pdata3 ? strlen(pdata3) : 0;

    DataDesc *pdataDesc = allocDataCopy2(size1 + size2 + size3, 0,
                                         pdata1, size1, pdata2, size2);
    memcpy(pdataDesc->data + size1 + size2, pdata3, size3);
    pData = pdataDesc;
}

VString::VString(const char* pdata, uint size)
{
    OVR_ASSERT((size == 0) || (pdata != 0));
    pData = allocDataCopy1(size, 0, pdata, size);
}

VString::VString(const VString& src)
{
    pData = src.data();
    pData->addRef();
}

VString::VString(const VStringBuffer& src)
{
    pData = allocDataCopy1(src.size(), 0, src.toCString(), src.size());
}

VString::VString(const wchar_t* data)
{
    pData = &NullData;
    pData->addRef();
    // Simplified logic for wchar_t constructor.
    if (data)
        *this = data;
}


VString::DataDesc* VString::allocData(uint size, uint lengthIsSize)
{
    VString::DataDesc* pdesc;

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


VString::DataDesc* VString::allocDataCopy1(uint size, uint lengthIsSize,
                                         const char* pdata, uint copySize)
{
    VString::DataDesc* pdesc = allocData(size, lengthIsSize);
    memcpy(pdesc->data, pdata, copySize);
    return pdesc;
}

VString::DataDesc* VString::allocDataCopy2(uint size, uint lengthIsSize,
                                         const char* pdata1, uint copySize1,
                                         const char* pdata2, uint copySize2)
{
    VString::DataDesc* pdesc = allocData(size, lengthIsSize);
    memcpy(pdesc->data, pdata1, copySize1);
    memcpy(pdesc->data + copySize1, pdata2, copySize2);
    return pdesc;
}


uint VString::length() const
{
    // Optimize length accesses for non-UTF8 character strings.
    DataDesc* pdata = data();
    uint     length, size = pdata->size();

    if (pdata->lengthIsSize())
        return size;

    length = (uint)UTF8Util::GetLength(pdata->data, (uint)size);

    if (length == size)
        pdata->m_size |= String_LengthIsSize;

    return length;
}


//static UInt32 String_CharSearch(const char* buf, )


UInt32 VString::at(uint index) const
{
    int       i = (int) index;
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

UInt32 VString::firstCharAt(uint index, const char** offset) const
{
    DataDesc*   pdata = data();
    int       i = (int) index;
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

UInt32 VString::nextChar(const char** offset) const
{
    return UTF8Util::DecodeNextChar(offset);
}



void VString::append(UInt32 ch)
{
    DataDesc*   pdata = data();
    uint       size = pdata->size();
    char        buff[8];
    int       encodeSize = 0;

    // Converts ch into UTF8 string and fills it into buff.
    UTF8Util::EncodeChar(buff, &encodeSize, ch);
    OVR_ASSERT(encodeSize >= 0);

    setData(allocDataCopy2(size + (uint)encodeSize, 0,
                           pdata->data, size, buff, (uint)encodeSize));
    pdata->release();
}


void VString::append(const wchar_t* pstr, int len)
{
    if (!pstr)
        return;

    DataDesc*   pdata = data();
    uint       oldSize = pdata->size();
    uint       encodeSize = (uint)UTF8Util::GetEncodeStringSize(pstr, len);

    DataDesc*   pnewData = allocDataCopy1(oldSize + (uint)encodeSize, 0,
                                          pdata->data, oldSize);
    UTF8Util::EncodeString(pnewData->data + oldSize,  pstr, len);

    setData(pnewData);
    pdata->release();
}


void VString::append(const char* putf8str, int utf8StrSz)
{
    if (!putf8str || !utf8StrSz)
        return;
    if (utf8StrSz == -1)
        utf8StrSz = (int)strlen(putf8str);

    DataDesc*   pdata = data();
    uint       oldSize = pdata->size();

    setData(allocDataCopy2(oldSize + (uint)utf8StrSz, 0,
                           pdata->data, oldSize, putf8str, (uint)utf8StrSz));
    pdata->release();
}

void    VString::assign(const char* putf8str, uint size)
{
    DataDesc* poldData = data();
    setData(allocDataCopy1(size, 0, putf8str, size));
    poldData->release();
}

void    VString::operator = (const char* pstr)
{
    assign(pstr, pstr ? strlen(pstr) : 0);
}

void    VString::operator = (const wchar_t* pwstr)
{
    DataDesc*   poldData = data();
    uint       size = pwstr ? (uint)UTF8Util::GetEncodeStringSize(pwstr) : 0;

    DataDesc*   pnewData = allocData(size, 0);
    UTF8Util::EncodeString(pnewData->data, pwstr);
    setData(pnewData);
    poldData->release();
}


void    VString::operator = (const VString& src)
{
    DataDesc*    psdata = src.data();
    DataDesc*    pdata = data();

    setData(psdata);
    psdata->addRef();
    pdata->release();
}


void    VString::operator = (const VStringBuffer& src)
{
    DataDesc* polddata = data();
    setData(allocDataCopy1(src.size(), 0, src.toCString(), src.size()));
    polddata->release();
}

void    VString::operator += (const VString& src)
{
    DataDesc   *pourData = data(),
               *psrcData = src.data();
    uint       ourSize  = pourData->size(),
                srcSize  = psrcData->size();
    uint       lflag    = pourData->lengthFlag() & psrcData->lengthFlag();

    setData(allocDataCopy2(ourSize + srcSize, lflag,
                           pourData->data, ourSize, psrcData->data, srcSize));
    pourData->release();
}


VString   VString::operator + (const char* str) const
{
    VString tmp1(*this);
    tmp1 += (str ? str : "");
    return tmp1;
}

VString   VString::operator + (const VString& src) const
{
    VString tmp1(*this);
    tmp1 += src;
    return tmp1;
}

void    VString::remove(uint posAt, int removeLength)
{
    DataDesc*   pdata = data();
    uint       oldSize = pdata->size();
    // Length indicates the number of characters to remove.
    uint       length = this->length();

    // If index is past the string, nothing to remove.
    if (posAt >= length)
        return;
    // Otherwise, cap removeLength to the length of the string.
    if ((posAt + removeLength) > length)
        removeLength = length - posAt;

    // Get the byte position of the UTF8 char at position posAt.
    int bytePos    = UTF8Util::GetByteIndex(posAt, pdata->data, oldSize);
    int removeSize = UTF8Util::GetByteIndex(removeLength, pdata->data + bytePos, oldSize-bytePos);

    setData(allocDataCopy2(oldSize - removeSize, pdata->lengthFlag(),
                           pdata->data, bytePos,
                           pData->data + bytePos + removeSize, (oldSize - bytePos - removeSize)));
    pdata->release();
}


VString   VString::mid(uint start, uint end) const
{
    uint length = this->length();
    if ((start >= length) || (start >= end))
        return VString();

    DataDesc* pdata = data();

    // If size matches, we know the exact index range.
    if (pdata->lengthIsSize())
        return VString(pdata->data + start, end - start);

    // Get position of starting character.
    int byteStart = UTF8Util::GetByteIndex(start, pdata->data, pdata->size());
    int byteSize  = UTF8Util::GetByteIndex(end - start, pdata->data + byteStart, pdata->size()-byteStart);
    return VString(pdata->data + byteStart, (uint)byteSize);
}

void VString::clear()
{
    NullData.addRef();
    data()->release();
    setData(&NullData);
}


VString   VString::toUpper() const
{
    UInt32      c;
    const char* psource = data()->data;
    const char* pend = psource + data()->size();
    VString      str;
    int       bufferOffset = 0;
    char        buffer[512];

    while(psource < pend)
    {
        do {
            c = UTF8Util::DecodeNextChar_Advance0(&psource);
            UTF8Util::EncodeChar(buffer, &bufferOffset, OVR_towupper(wchar_t(c)));
        } while ((psource < pend) && (bufferOffset < int(sizeof(buffer)-8)));

        // Append string a piece at a time.
        str.append(buffer, bufferOffset);
        bufferOffset = 0;
    }

    return str;
}

VString   VString::toLower() const
{
    UInt32      c;
    const char* psource = data()->data;
    const char* pend = psource + data()->size();
    VString      str;
    int       bufferOffset = 0;
    char        buffer[512];

    while(psource < pend)
    {
        do {
            c = UTF8Util::DecodeNextChar_Advance0(&psource);
            UTF8Util::EncodeChar(buffer, &bufferOffset, OVR_towlower(wchar_t(c)));
        } while ((psource < pend) && (bufferOffset < int(sizeof(buffer)-8)));

        // Append string a piece at a time.
        str.append(buffer, bufferOffset);
        bufferOffset = 0;
    }

    return str;
}



VString& VString::insert(const char* substr, uint posAt, int strSize)
{
    DataDesc* poldData   = data();
    uint     oldSize    = poldData->size();
    uint     insertSize = (strSize < 0) ? strlen(substr) : (uint)strSize;
    uint     byteIndex  =  (poldData->lengthIsSize()) ?
                            posAt : (uint)UTF8Util::GetByteIndex(posAt, poldData->data, oldSize);

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

uint VString::insert(UInt32 c, uint posAt)
{
    char    buf[8];
    int   index = 0;
    UTF8Util::EncodeChar(buf, &index, c);
    OVR_ASSERT(index >= 0);
    buf[(uint)index] = 0;

    insert(buf, posAt, index);
    return (uint)index;
}

void VString::stripTrailing(const char * s)
{
    const uint len = strlen(s);
    if (length() >= len && right(len) == s)
	{
        *this = left(length() - len);
	}
}

int VString::CompareNoCase(const char* a, const char* b)
{
    return OVR_stricmp(a, b);
}

int VString::CompareNoCase(const char* a, const char* b, int len)
{
    if (len)
    {
        int f,l;
        int slen = len;
        const char *s = b;
        do {
            f = (int)OVR_tolower((int)(*(a++)));
            l = (int)OVR_tolower((int)(*(b++)));
        } while (--len && f && (f == l) && *b != 0);

        if (f == l && (len != 0 || *b != 0))
        {
            f = (int)slen;
            l = (int)strlen(s);
            return int(f - l);
        }

        return int(f - l);
    }
    else
        return (0-(int)strlen(b));
}

// ***** Implement hash static functions

// Hash function
uint VString::BernsteinHashFunction(const void* pdataIn, uint size, uint seed)
{
    const UByte*    pdata   = (const UByte*) pdataIn;
    uint           h       = seed;
    while (size > 0)
    {
        size--;
        h = ((h << 5) + h) ^ (unsigned) pdata[size];
    }

    return h;
}

// Hash function, case-insensitive
uint VString::BernsteinHashFunctionCIS(const void* pdataIn, uint size, uint seed)
{
    const UByte*    pdata = (const UByte*) pdataIn;
    uint           h = seed;
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

} // OVR
