#pragma once

#include "VPath.h"
#include "VColor.h"

NV_NAMESPACE_BEGIN

class VPath;

class VImage
{
public:
    enum Filter
    {
        NearestFilter,
        LinearFilter,
        CubicFilter
    };

    VImage();
    VImage(const VPath &path);
    ~VImage();

    bool isValid() const;

    int width() const;
    int height() const;

    const uchar *data() const;
    VColor at(int x, int y) const;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VImage)
};

NV_NAMESPACE_END
