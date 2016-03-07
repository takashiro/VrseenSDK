#pragma once

#include "vglobal.h"

#include <string>
#include <string.h>
#include <iostream>

NV_NAMESPACE_BEGIN

class VString : public std::u16string
{
public:
    VString();

    VString(const char *str);
    VString(const char *data, uint length);
    VString(const std::string &str);

    VString(const char16_t *data, uint length) { assign(data, length); }
    VString(const std::u16string &source);

    VString(const VString &source) : basic_string(source) {}

    // Returns number of bytes
    int length() const { return size(); }

    bool isEmpty() const { return empty(); }

    void assign(const char *str) { assign(str, strlen(str)); }
    void assign(const char *str, uint size);
    void assign(const char16_t *str);
    void assign(const char16_t *str, uint size);

    const VString &operator = (const char *str);
    const VString &operator = (const VString &src);

    void append(char16_t ch) { basic_string::operator +=(ch); }
    void append(const VString &str) { basic_string::append(str.data()); }
    void append(const char *str) { append(str, strlen(str)); }
    void append(const char *str, uint length);

    void insert(uint pos, char16_t ch);
    void insert(uint pos, const VString &str) { basic_string::insert(pos, str.data()); }
    void insert(uint pos, const char *str);

    void prepend(char16_t ch) { insert(0, ch); }
    void prepend(const VString &str) { insert(0, str); }
    void prepend(const char *str) { insert(0, str); }

    void replace(char16_t from, char16_t to);

    void remove(uint index, uint length = 1) { basic_string::erase(index, length); }

    VString mid(uint from, uint length = 0) const { return substr(from, length ? length : size() - from); }
    VString range(uint start, uint end) const { return mid(start, end - start); }
    VString left(uint count) const { return mid(0, count); }
    VString right(uint count) const { return mid(size() - count, count); }

    char16_t first() const { return front(); }
    char16_t last() const { return back(); }

    bool contains(char16_t ch) const { return find(ch) != npos; }
    bool contains(const VString &substr) const { return find(substr) != npos; }

    bool startsWith(char16_t ch) const { return front() == ch; }
    bool startsWith(const VString &prefix) const;

    bool endsWith(char16_t ch) const { return back() == ch; }
    bool endsWith(const VString &postfix) const;

    VString toUpper() const;
    VString toLower() const;

    void stripTrailing(const char *str);

    const VString &operator += (const VString &str) { append(str); return *this; }
    const VString &operator += (const char *str) { append(str); return *this; }
    const VString &operator += (char16_t ch) { basic_string::operator +=(ch); return *this; }

    friend VString operator + (const VString &str1, const VString &str2);
    friend VString operator + (const VString &str, char16_t ch);
    friend VString operator + (char16_t ch, const VString &str);

    std::string toStdString() const;
    const char *toCString() const;

    int compare(const VString &str) const { return basic_string::compare(str.data()); }
    int compare(const char *str) const;

    bool operator == (const VString &str) const { return compare(str) == 0; }
    bool operator == (const char *str) const { return compare(str) == 0; }

    bool operator != (const VString& str) const { return compare(str) != 0; }
    bool operator != (const char *str) const { return compare(str) != 0; }

    bool operator < (const VString &str) const { return compare(str) < 0; }
    bool operator < (const char* str) const { return compare(str) < 0; }

    bool operator > (const VString &str) const { return compare(str) > 0; }
    bool operator > (const char *str) const { return compare(str) > 0; }

    int icompare(const VString &str) const;
    int icompare(const char* str) const;

    static VString number(int num);
    static VString number(double num);
    int toInt() const;
    double toDouble() const;

    friend std::ostream &operator << (std::ostream &out, const VString &str) { out << str.toStdString(); return out; }
    void sprintf(const char *format, ...);
};

NV_NAMESPACE_END
