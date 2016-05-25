#pragma once

//TODO: Remove this file

#include "vglobal.h"

NV_NAMESPACE_BEGIN

enum ImageFilter
{
    IMAGE_FILTER_NEAREST,
    IMAGE_FILTER_LINEAR,
    IMAGE_FILTER_CUBIC
};

class VFileOperation
{
public:
    static void Write32BitPvrTexture(const char * fileName, const unsigned char * texture, int width, int height);

    // The returned buffer should be freed with free()
    // If srgb is true, the resampling will be gamma correct, otherwise it is just sumOf4 >> 2
    static uchar *QuarterImageSize(const uchar *src, const int width, const int height, const bool srgb);

    static uchar *ScaleImageRGBA(const uchar *src, const int width, const int height, const int newWidth, const int newHeight, const ImageFilter filter);
};

NV_NAMESPACE_END

