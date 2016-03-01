#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

// Uncompressed .pvr textures are much more efficient to load than bmp/tga/etc.
// Use when generating thumbnails, etc.
// Use stb_image_write.h for conventional files.
void		Write32BitPvrTexture( const char * fileName, const unsigned char * texture, int width, int height );

// The returned buffer should be freed with free()
// If srgb is true, the resampling will be gamma correct, otherwise it is just sumOf4 >> 2
unsigned char * QuarterImageSize( const unsigned char * src, const int width, const int height, const bool srgb );

// The returned buffer should be freed with free().
enum ImageFilter
{
	IMAGE_FILTER_NEAREST,
	IMAGE_FILTER_LINEAR,
	IMAGE_FILTER_CUBIC
};
// filter: 0 = nearest, 1 = linear, 2 = cubic
unsigned char * ScaleImageRGBA( const unsigned char * src, const int width, const int height, const int newWidth, const int newHeight, const ImageFilter filter );

NV_NAMESPACE_END
