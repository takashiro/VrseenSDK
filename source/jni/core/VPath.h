#pragma once

#include "VString.h"

NV_NAMESPACE_BEGIN

class VPath : public VString
{
public:
    VPath();
    VPath(const char *str) : VString(str) {}
    VPath(const VString &source) : VString(source) {}
    VPath(const VPath &path) : VString(path.data(), path.size()) {}

    bool isAbsolute() const;

    bool hasExtension() const;
    VString extension() const;

    VString fileName() const;
    VString baseName() const;
};

NV_NAMESPACE_END
