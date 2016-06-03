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
};

NV_NAMESPACE_END

