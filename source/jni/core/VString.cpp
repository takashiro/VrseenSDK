#include "VString.h"

#include <stdarg.h>
#include <sstream>

NV_NAMESPACE_BEGIN

namespace {
    template<class C, class T>
    int StringCompare(const C *str1, const T *str2)
    {
        if (str1 == nullptr) {
            return str2 == nullptr ? 0 : -1;
        }

        while (*str1 == *str2) {
            if (*str1 == '\0') {
                return 0;
            }
            str1++;
            str2++;
        }
        return *str1 < *str2 ? -1 : 1;
    }

    template<class C, class T>
    int StringCaseCompare(const C *str1, const T *str2)
    {
        if (str1 == nullptr) {
            return str2 == nullptr ? 0 : -1;
        }

        C ch1;
        T ch2;
        forever {
            ch1 = *str1;
            ch2 = *str2;
            if (ch1 != ch2 && tolower(ch1) != tolower(ch2)) {
                break;
            }
            if (ch1 == '\0') {
                return 0;
            }
            str1++;
            str2++;
        }
        return ch1 < ch2 ? -1 : 1;
    }
}

VString::VString(const char *str)
{
    assign(str);
}

VString::VString(const std::string &str)
{
    uint size = str.size();
    resize(str.size());
    for (uint i = 0; i < size; i++) {
        operator [](i) = str.at(i);
    }
}

VString::VString(const char *data, uint length)
{
    resize(length);
    for (uint i = 0; i < length; i++) {
        operator [](i) = data[i];
    }
}

void VString::assign(const char *str)
{
    if (str == nullptr) {
        clear();
        return;
    }

    assign(str, strlen(str));
}

void VString::append(const char *str, uint length)
{
    for (uint i = 0; i < length; i++) {
        append(str[i]);
    }
}

void VString::assign(const char *str, uint size)
{
    if (str == nullptr) {
        clear();
        return;
    }

    resize(size);
    for (uint i = 0; i < size; i++) {
        operator [](i) = str[i];
    }
}

void VString::assign(const char16_t *str)
{
    if (str == nullptr) {
        clear();
        return;
    }

    uint size = 0;
    while (str[size++]) {}
    resize(size);

    assign(str, size);
}

void VString::assign(const char16_t *str, uint size)
{
    if (str == nullptr) {
        clear();
        return;
    }

    resize(size);
    for (uint i = 0; i < size; i++) {
        operator [](i) = str[i];
    }
}

VString VString::toUpper() const
{
    VString str(*this);
    for (char16_t &ch : str) {
        if (ch >= 'a' && ch <= 'z') {
            ch -= 0x20;
        }
    }
    return str;
}

VString VString::toLower() const
{
    VString str(*this);
    for (char16_t &ch : str) {
        if (ch >= 'A' && ch <= 'Z') {
            ch += 0x20;
        }
    }
    return str;
}

void VString::insert(uint pos, const char *str)
{
    VString vstring(str);
    basic_string::insert(pos, vstring.data());
}

void VString::replace(char16_t from, char16_t to)
{
    for (char16_t &ch : *this) {
        if (ch == from) {
            ch = to;
        }
    }
}

bool VString::startsWith(const VString &prefix, bool caseSensitive) const
{
    if (prefix.size() > this->size()) {
        return false;
    }

    for (uint i = 0; i < prefix.size(); i++) {
        if (prefix[i] != this->at(i)) {
            if (caseSensitive || tolower(prefix[i]) != tolower(at(i))) {
                return false;
            }
        }
    }
    return true;
}

bool VString::endsWith(const VString &postfix, bool caseSensitive) const
{
    if (postfix.size() > this->size()) {
        return false;
    }

    const char16_t *str1 = this->data() + this->size() - postfix.size();
    const char16_t *str2 = postfix.data();
    return (caseSensitive ? StringCompare(str1, str2) : StringCaseCompare(str1, str2)) == 0;
}

void VString::insert(uint pos, char16_t ch)
{
    basic_string::insert(begin() + pos, ch);
}

const VString &VString::operator = (const char *str)
{
    assign(str);
    return *this;
}

const VString &VString::operator =(const char16_t *str)
{
    assign(str);
    return *this;
}

const VString &VString::operator = (const VString &source)
{
    std::u16string::assign(source);
    return *this;
}

const VString &VString::operator =(VString &&source)
{
    std::u16string::assign(std::move(source));
    return *this;
}

VString operator + (const VString &str1, const VString &str2)
{
    VString str(str1);
    str.append(str2);
    return str;
}

VString operator + (const VString &str, char16_t ch)
{
    VString result(str);
    result.append(ch);
    return result;
}

VString operator + (char16_t ch, const VString &str)
{
    VString result;
    result.append(ch);
    result.append(str);
    return result;
}

std::string VString::toStdString() const
{
    uint size = this->size();
    std::string str;
    str.resize(size);
    for (uint i = 0; i < size; i++) {
        str[i] = at(i);
    }
    return str;
}

VByteArray VString::toUtf8() const
{
    VByteArray utf8;
    uint code = 0;
    for (const char16_t &in : *this) {
        if (in >= 0xd800 && in <= 0xdbff) {
            code = ((in - 0xd800) << 10) + 0x10000;
        } else {
            if (in >= 0xdc00 && in <= 0xdfff) {
                code |= in - 0xdc00;
            } else {
                code = in;
            }

            if (code <= 0x7f) {
                utf8.append(static_cast<char>(code));
            } else if (code <= 0x7ff) {
                utf8.append(static_cast<char>(0xc0 | ((code >> 6) & 0x1f)));
                utf8.append(static_cast<char>(0x80 | (code & 0x3f)));
            } else if (code <= 0xffff) {
                utf8.append(static_cast<char>(0xe0 | ((code >> 12) & 0x0f)));
                utf8.append(static_cast<char>(0x80 | ((code >> 6) & 0x3f)));
                utf8.append(static_cast<char>(0x80 | (code & 0x3f)));
            } else {
                utf8.append(static_cast<char>(0xf0 | ((code >> 18) & 0x07)));
                utf8.append(static_cast<char>(0x80 | ((code >> 12) & 0x3f)));
                utf8.append(static_cast<char>(0x80 | ((code >> 6) & 0x3f)));
                utf8.append(static_cast<char>(0x80 | (code & 0x3f)));
            }
            code = 0;
        }
    }
    return utf8;
}

VString VString::fromUtf8(const VByteArray &utf8)
{
    VString utf16;
    unsigned int code = 0;
    int following = 0;
    for (const uchar &ch : utf8) {
        if (ch <= 0x7f) {
            code = ch;
            following = 0;
        } else if (ch <= 0xbf) {
            if (following > 0) {
                code = (code << 6) | (ch & 0x3f);
                --following;
            }
        } else if (ch <= 0xdf) {
            code = ch & 0x1f;
            following = 1;
        } else if (ch <= 0xef) {
            code = ch & 0x0f;
            following = 2;
        } else {
            code = ch & 0x07;
            following = 3;
        }

        if (following == 0) {
            if (code > 0xffff) {
                utf16.append(static_cast<char16_t>(0xd800 + (code >> 10)));
                utf16.append(static_cast<char16_t>(0xdc00 + (code & 0x03ff)));
            } else {
                utf16.append(static_cast<char16_t>(code));
            }
            code = 0;
        }
    }
    return utf16;
}

VByteArray VString::toLatin1() const
{
    uint size = this->size();
    VByteArray latin1;
    latin1.resize(size);
    for (uint i = 0; i < size; i++) {
        latin1[i] = at(i);
    }
    return latin1;
}

VString VString::fromLatin1(const VByteArray &latin1)
{
    uint size = latin1.size();
    VString utf16;
    utf16.resize(size);
    for (uint i = 0; i < size; i++) {
        utf16[i] = latin1[i];
    }
    return utf16;
}

std::u32string VString::toUcs4() const
{
    std::u32string ucs4;
    char32_t code = 0;
    for (const char16_t &in : *this) {
        if (in >= 0xd800 && in <= 0xdbff) {
            code = ((in - 0xd800) << 10) + 0x10000;
        } else {
            if (in >= 0xdc00 && in <= 0xdfff) {
                code |= in - 0xdc00;
            } else {
                code = in;
            }

            ucs4.append(1, code);
            code = 0;
        }
    }
    return ucs4;
}

VString VString::fromUcs4(const std::u32string &ucs4)
{
    VString utf16;
    for (const char32_t &code : ucs4) {
        if (code > 0xffff) {
            utf16.append(static_cast<char16_t>(0xd800 + (code >> 10)));
            utf16.append(static_cast<char16_t>(0xdc00 + (code & 0x03ff)));
        } else {
            utf16.append(static_cast<char16_t>(code));
        }
    }
    return utf16;
}

int VString::compare(const char *str) const
{
    return StringCompare(data(), str);
}

int VString::icompare(const VString &str) const
{
    return StringCaseCompare(data(), str.data());
}

int VString::icompare(const char *str) const
{
    return StringCaseCompare(data(), str);
}

VString VString::number(int num)
{
    std::stringstream ss;
    ss << num;
    std::string str;
    ss >> str;
    return str;
}

VString VString::number(double num)
{
    std::stringstream ss;
    ss << num;
    std::string str;
    ss >> str;
    return str;
}

int VString::toInt() const
{
    std::stringstream ss;
    ss << toStdString();
    int num;
    ss >> num;
    return num;
}

double VString::toDouble() const
{
    std::stringstream ss;
    ss << toStdString();
    double num;
    ss >> num;
    return num;
}

void VString::sprintf(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    int length = vsnprintf(nullptr, 0, format, arguments);
    char *bytes = new char[length + 1];
    vsprintf(bytes, format, arguments);
    va_end(arguments);
    append(bytes);
    delete[] bytes;
}

void VString::stripTrailing(const char *str)
{
    const uint len = strlen(str);
    //@to-do: add compare(int from, int to, const char *).
    if (size() >= len && right(len) == str) {
        *this = left(length() - len);
    }
}

// ***** Implement hash static functions

/*// Hash function
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
}*/

NV_NAMESPACE_END
