#pragma once

#include "VByteArray.h"

NV_NAMESPACE_BEGIN

class VPath;

class VResource
{
public:
    VResource(const VPath &path);
    VResource(const char *path);
    ~VResource();

    bool exists() const;

    const VPath &path() const;
    VByteArray data() const;

    uint size() const;
    int length() const;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VResource)
};

NV_NAMESPACE_END
