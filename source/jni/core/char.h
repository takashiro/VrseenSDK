#pragma once

#include "global.h"

#include <ctype.h>

NERVGEAR_NAMESPACE_BEGIN

class Char
{
public:
    Char() { value = '\0'; }
    Char(char ch) { value = ch; }
    Char(wchar_t ch) { value = ch; }
    Char(short ch) { value = ch; }
    Char(ushort ch) { value = ch; }
    Char(int ch) { value = ch; }
    Char(uint ch) { value = ch; }

    operator char() const { return value; }
    operator wchar_t() const { return value; }
    operator short() const { return value; }
    operator ushort() const { return value; }
    operator int() const { return value; }
    operator uint() const { return value; }

    Char &operator = (char ch) { value = ch; return *this; }
    Char &operator = (wchar_t ch) { value = ch; return *this; }
    Char &operator = (short ch) { value = ch; return *this; }
    Char &operator = (ushort ch) { value = ch; return *this; }
    Char &operator = (int ch) { value = ch; return *this; }
    Char &operator = (uint ch) { value = ch; return *this; }

    bool isSpace() const { return isspace(value); }
    bool isAlpha() const { return isalpha(value); }
    bool isControl() const { return iscntrl(value); }
    bool isDigit() const { return isdigit(value); }
    bool isGraph() const { return isgraph(value); }
    bool isLower() const { return islower(value); }
    bool isPrint() const { return isprint(value); }
    bool isPunct() const { return ispunct(value); }
    bool isUpper() const { return isupper(value); }

    Char toLower() const { return tolower(value); }
    Char toUpper() const { return toupper(value); }

private:
    wchar_t value;
};

NERVGEAR_NAMESPACE_END
