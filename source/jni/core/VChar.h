#pragma once

#include "vglobal.h"

#include <ctype.h>

NV_NAMESPACE_BEGIN

class VChar
{
public:
    VChar() { m_value = '\0'; }
    VChar(uchar ch) { m_value = ch; }
    VChar(short ch) { m_value = ch; }
    VChar(ushort ch) { m_value = ch; }
    VChar(int ch) { m_value = ch; }
    VChar(uint ch) { m_value = ch; }
    VChar(char16_t ch) { m_value = ch; }
    VChar(const VChar &ch) { m_value = ch.m_value; }

    bool operator == (const VChar &ch) const { return m_value == ch.m_value; }
    bool operator != (const VChar &ch) const { return m_value != ch.m_value; }

    bool operator < (const VChar &ch) const { return m_value < ch.m_value; }
    bool operator <= (const VChar &ch) const { return m_value <= ch.m_value; }

    bool operator > (const VChar &ch) const { return m_value > ch.m_value; }
    bool operator >= (const VChar &ch) const { return m_value >= ch.m_value; }

    VChar operator++() { return ++m_value; }
    VChar operator++(int) { return m_value++; }

    VChar operator--() { return --m_value; }
    VChar operator--(int) { return m_value--; }

    bool isSpace() const { return isspace(m_value); }
    bool isAlpha() const { return isalpha(m_value); }
    bool isControl() const { return iscntrl(m_value); }
    bool isDigit() const { return isdigit(m_value); }
    bool isGraph() const { return isgraph(m_value); }
    bool isLower() const { return islower(m_value); }
    bool isPrint() const { return isprint(m_value); }
    bool isPunct() const { return ispunct(m_value); }
    bool isUpper() const { return isupper(m_value); }

    VChar toLower() const { return tolower(m_value); }
    VChar toUpper() const { return toupper(m_value); }
    char toLatin1() const { return m_value; }

    char16_t unicode() const { return m_value; }
    char16_t &unicode() { return m_value; }

private:
    char16_t m_value;
};

NV_NAMESPACE_END
