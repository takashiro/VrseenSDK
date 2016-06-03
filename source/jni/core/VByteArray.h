#pragma once

#include "vglobal.h"

#include <string>

NV_NAMESPACE_BEGIN

class VByteArray : public std::basic_string<char>
{
public:
    VByteArray() {}
    VByteArray(const std::string &source) : basic_string(source) {}
    VByteArray(uint length, char ch = '\0');
    VByteArray(const char *str) : basic_string(str) {}
    VByteArray(const char *bytes, uint length) : basic_string(bytes, length) {}

    void append(char ch) { basic_string::operator +=(ch); }
    void append(char *bytes, vint64 length) { basic_string::append(bytes, length); }

    bool isEmpty() const { return empty(); }
};

NV_NAMESPACE_END
