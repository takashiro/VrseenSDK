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
    VPath(const char *data, uint length) : VString(data, length) {}
    VPath(const char16_t *data, uint length) : VString(data, length) {}

    bool isAbsolute() const;

    bool hasProtocol() const;
    VString protocol() const;

    bool hasExtension() const;
    void setExtension(const VString &ext);
    VString extension() const;

    VString fileName() const;
    VString baseName() const;

    VString dirName() const;
    VString dirPath() const;
};

NV_NAMESPACE_END
