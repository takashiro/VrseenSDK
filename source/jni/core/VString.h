#pragma once

#include "vglobal.h"

#include "Types.h"
#include "Allocator.h"
#include "UTF8Util.h"
#include "Atomic.h"
#include "Std.h"
#include "Alg.h"

#include "VChar.h"

#include <string>

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

    //@to-do: remove this function
    VString(const char *str1, const char *str2, const char *str3 = nullptr);

    // Returns number of bytes
    int length() const { return size(); }

    bool isEmpty() const { return empty(); }

    // Appends a character
    void append(VChar ch) { basic_string::operator +=(ch); }

    // Append a string
    void append(const VString &str) { basic_string::append(str.data()); }
    void append(const char *str) { append(str, strlen(str)); }
    void append(const char *str, uint length);

    // Assigns string with known size.
    void assign(const VChar *str, uint size) { basic_string::assign(str, str + size); }
    void assign(const char *str);

    void remove(uint index, uint length = 1) { basic_string::erase(index, length); }

    VString mid(uint from, uint length = 0) const { return substr(from, length); }
    VString range(uint start, uint end) const { return mid(start, end - start); }
    VString left(uint count) const { return mid(0, count); }
    VString right(uint count) const { return mid(size() - count, count); }

    // Case-conversion
    VString toUpper() const;
    VString toLower() const;

    void insert(const char *substr, uint pos);
    void insert(UInt32 c, uint posAt);

    void stripTrailing(const char *str);

    // Utility: case-insensitive string compare.  stricmp() & strnicmp() are not
    // ANSI or POSIX, do not seem to appear in Linux.
    static int OVR_STDCALL icompare(const char* a, const char* b);
    static int OVR_STDCALL icompare(const char* a, const char* b, int len);

    static bool HasAbsolutePath(const char* path);
    static bool HasExtension(const char* path);
    static bool HasProtocol(const char* path);

    bool    hasAbsolutePath() const { return HasAbsolutePath(toCString()); }
    bool    hasExtension() const    { return HasExtension(toCString()); }
    bool    hHasProtocol() const     { return HasProtocol(toCString()); }

    VString  protocol() const;    // Returns protocol, if any, with trailing '://'.
    VString  path() const;        // Returns path with trailing '/'.
    VString  fileName() const;    // Returns filename, including extension.
    VString  extension() const;   // Returns extension with a dot.

    void    stripProtocol();        // Strips front protocol, if any, from the string.
    void    stripExtension();       // Strips off trailing extension.

    // Assignment
    const VString &operator = (const char *str);
    const VString &operator = (const VString &src);

    // Addition
    const VString &operator += (const VString &str) { append(str); return *this; }
    const VString &operator += (const char *str) { append(str); return *this; }
    const VString &operator += (VChar ch) { basic_string::operator +=(ch); return *this; }

    friend VString operator + (const VString &str1, const VString &str2);

    // Conversion
    std::string toStdString() const;
    const char *toCString() const;

    // Comparison
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
};

NV_NAMESPACE_END
