#pragma once

#include "vglobal.h"

#include <ctype.h>

NV_NAMESPACE_BEGIN

class VChar
{
public:
    VChar() { value = '\0'; }
    VChar(char ch) { value = ch; }
    VChar(wchar_t ch) { value = ch; }
    VChar(short ch) { value = ch; }
    VChar(ushort ch) { value = ch; }
    VChar(int ch) { value = ch; }
    VChar(uint ch) { value = ch; }

    operator char() const { return value; }
    operator wchar_t() const { return value; }
    operator short() const { return value; }
    operator ushort() const { return value; }
    operator int() const { return value; }
    operator uint() const { return value; }

    VChar &operator = (char ch) { value = ch; return *this; }
    VChar &operator = (wchar_t ch) { value = ch; return *this; }
    VChar &operator = (short ch) { value = ch; return *this; }
    VChar &operator = (ushort ch) { value = ch; return *this; }
    VChar &operator = (int ch) { value = ch; return *this; }
    VChar &operator = (uint ch) { value = ch; return *this; }

    bool isSpace() const { return isspace(value); }
    bool isAlpha() const { return isalpha(value); }
    bool isControl() const { return iscntrl(value); }
    bool isDigit() const { return isdigit(value); }
    bool isGraph() const { return isgraph(value); }
    bool isLower() const { return islower(value); }
    bool isPrint() const { return isprint(value); }
    bool isPunct() const { return ispunct(value); }
    bool isUpper() const { return isupper(value); }

    VChar toLower() const { return tolower(value); }
    VChar toUpper() const { return toupper(value); }

private:
    wchar_t value;
};

NV_NAMESPACE_END
