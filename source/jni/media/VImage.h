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
    VImage(const VImage &source);
    VImage(VImage &&source);
    VImage(uchar *raw, int width, int height);
    ~VImage();

    bool isValid() const;

    int width() const;
    int height() const;

    const uchar *data() const;
    uint length() const;

    VColor at(int x, int y) const;

    void resize(int width, int height, Filter filter = NearestFilter);
    void quarter(bool srgb);

    bool operator==(const VImage &source) const;

private:
    NV_DECLARE_PRIVATE
};

NV_NAMESPACE_END
