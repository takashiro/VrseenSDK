#include "VString.h"

#include <stdarg.h>
#include <sstream>

NV_NAMESPACE_BEGIN

namespace {
    template<class C, class T>
    void CopyString(C *to, const T *from, uint length)
    {
        for (uint i = 0; i < length; i++) {
            to[i] = from[i];
        }
    }

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
    int CaseCompareString(const C *str1, const T *str2)
    {
        if (str1 == nullptr) {
            return str2 == nullptr ? 0 : -1;
        }

        C ch1;
        T ch2;
        forever {
            ch1 = *str1;
            ch2 = *str2;
            if (ch1 != ch2) {
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

VString::VString()
{
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
        this->at(i) = str.at(i);
    }
}

VString::VString(const char *data, uint length)
{
    resize(length);
    for (uint i = 0; i < length; i++) {
        at(i) = data[i];
    }
}

VString::VString(const std::u16string &source)
{
    uint length = source.length();
    this->resize(length);
    for (uint i = 0; i < length; i++) {
        at(i) = source[i];
    }
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
        at(i) = str[i];
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

    for (uint i = 0; i < size; i++) {
        at(i) = str[i];
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

bool VString::startsWith(const VString &prefix) const
{
    if (prefix.size() > this->size()) {
        return false;
    }

    for (uint i = 0; i < prefix.size(); i++) {
        if (prefix[i] != this->at(i)) {
            return false;
        }
    }
    return true;
}

bool VString::endsWith(const VString &postfix) const
{
    if (postfix.size() > this->size()) {
        return false;
    }

    for (uint i = this->size() - postfix.size(), j = 0; j < postfix.size(); i++, j++) {
        if (this->at(i) != postfix[j]) {
            return false;
        }
    }
    return true;
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

const VString &VString::operator = (const VString &src)
{
    std::u16string::assign(src.data(), src.size());
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

const char *VString::toCString() const
{
    //@to-do: fix the memory leak
    char *str = new char[size() + 1];
    for (uint i = 0; i < size(); i++) {
        str[i] = at(i);
    }
    str[size()] = '\0';
    return str;
}

int VString::compare(const char *str) const
{
    return StringCompare(data(), str);
}

int VString::icompare(const VString &str) const
{
    return CaseCompareString(data(), str.data());
}

int VString::icompare(const char *str) const
{
    return CaseCompareString(data(), str);
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
