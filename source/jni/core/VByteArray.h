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
    VByteArray(const char *bytes, uint length) : basic_string(bytes, length) {}
    VByteArray(const VByteArray &source) : basic_string(source) {}
    VByteArray(VByteArray &&source) : basic_string(source) {}

    void append(char ch) { basic_string::operator +=(ch); }
};

NV_NAMESPACE_END
