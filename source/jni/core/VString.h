#pragma once

#include "vglobal.h"
#include "VChar.h"

#include <string>
#include <string.h>
#include <iostream>

NV_NAMESPACE_BEGIN

class VString : public std::basic_string<VChar>
{
public:
    VString();
    VString(const char *str);
    VString(const std::string &str);
    VString(const char *data, uint length);
    VString(const VChar *data, uint length) : basic_string(data, length) {}
    VString(const basic_string<VChar> &source) : basic_string(source) {}
    VString(const VString &source) : basic_string(source) {}

    // Returns number of bytes
    int length() const { return size(); }

    bool isEmpty() const { return empty(); }

    void assign(const VChar *str, uint size) { basic_string::assign(str, str + size); }
    void assign(const char *str);

    const VString &operator = (const char *str);
    const VString &operator = (const VString &src);

    void append(VChar ch) { basic_string::operator +=(ch); }
    void append(const VString &str) { basic_string::append(str.data()); }
    void append(const char *str) { append(str, strlen(str)); }
    void append(const char *str, uint length);

    void insert(uint pos, VChar ch);
    void insert(uint pos, const VString &str) { basic_string::insert(pos, str.data()); }
    void insert(uint pos, const char *str);

    void prepend(VChar ch) { insert(0, ch); }
    void prepend(const VString &str) { insert(0, str); }
    void prepend(const char *str) { insert(0, str); }

    void replace(VChar from, VChar to);

    void remove(uint index, uint length = 1) { basic_string::erase(index, length); }

    VString mid(uint from, uint length = 0) const { return substr(from, length ? length : size() - from); }
    VString range(uint start, uint end) const { return mid(start, end - start); }
    VString left(uint count) const { return mid(0, count); }
    VString right(uint count) const { return mid(size() - count, count); }

    VChar first() const { return front(); }
    VChar last() const { return back(); }

    bool contains(VChar ch) const { return find(ch) != npos; }
    bool contains(const VString &substr) const { return find(substr) != npos; }

    bool startsWith(VChar ch) const { return front() == ch; }
    bool startsWith(const VString &prefix) const;

    bool endsWith(VChar ch) const { return back() == ch; }
    bool endsWith(const VString &postfix) const;

    VString toUpper() const;
    VString toLower() const;

    void stripTrailing(const char *str);

    const VString &operator += (const VString &str) { append(str); return *this; }
    const VString &operator += (const char *str) { append(str); return *this; }
    const VString &operator += (VChar ch) { basic_string::operator +=(ch); return *this; }

    friend VString operator + (const VString &str1, const VString &str2);

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
    int toInt() const;

    friend std::ostream &operator << (std::ostream &out, const VString &str) { out << str.toStdString(); return out; }
};

NV_NAMESPACE_END
