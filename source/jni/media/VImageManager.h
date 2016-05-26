#pragma once

#include "VPath.h"

NV_NAMESPACE_BEGIN

class VImage;

class VImageManager
{
public:
    VImageManager();
    ~VImageManager();

    VImage *loadImage(const VPath &filename) const;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VImageManager)
};

NV_NAMESPACE_END
