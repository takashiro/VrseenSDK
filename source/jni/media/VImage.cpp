#include "VImage.h"

#include <3rdparty/stb/stb_image.h>

NV_NAMESPACE_BEGIN

struct VImage::Private
{
    uchar *data;
    int width;
    int height;

    Private()
        : data(nullptr)
        , width(0)
        , height(0)
    {
    }

    void load(const VPath &path)
    {
        int cmp;
        data = stbi_load(path.toUtf8().data(), &width, &height, &cmp, 4);
    }
};

VImage::VImage()
    : d(new Private)
{
}

VImage::VImage(const VPath &path)
    : d(new Private)
{
    d->load(path);
}

VImage::~VImage()
{
    if (d->data) {
        free(d->data);
    }
    delete d;
}

bool VImage::isValid() const
{
    return d->data != nullptr;
}

int VImage::width() const
{
    return d->width;
}

int VImage::height() const
{
    return d->height;
}

const uchar *VImage::data() const
{
    return d->data;
}

VColor VImage::at(int x, int y) const
{
    VColor pixel;
    if (x < 0 || x >= d->width || y < 0 || y >= d->height) {
        return pixel;
    }

    uint index = y * d->width + x;
    VColor *pixels = reinterpret_cast<VColor *>(d->data);
    return pixels[index];
}

NV_NAMESPACE_END
