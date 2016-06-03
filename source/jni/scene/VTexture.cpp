#include "VTexture.h"

#include "VPath.h"
#include "VFile.h"
#include "VResource.h"
#include "VImage.h"
#include "VEglDriver.h"
#include "VAlgorithm.h"

#include "3rdParty/stb/stb_image.h"

#include <fstream>

#define GL_COMPRESSED_RGBA_ASTC_4x4_KHR            0x93B0
#define GL_COMPRESSED_RGBA_ASTC_5x4_KHR            0x93B1
#define GL_COMPRESSED_RGBA_ASTC_5x5_KHR            0x93B2
#define GL_COMPRESSED_RGBA_ASTC_6x5_KHR            0x93B3
#define GL_COMPRESSED_RGBA_ASTC_6x6_KHR            0x93B4
#define GL_COMPRESSED_RGBA_ASTC_8x5_KHR            0x93B5
#define GL_COMPRESSED_RGBA_ASTC_8x6_KHR            0x93B6
#define GL_COMPRESSED_RGBA_ASTC_8x8_KHR            0x93B7
#define GL_COMPRESSED_RGBA_ASTC_10x5_KHR           0x93B8
#define GL_COMPRESSED_RGBA_ASTC_10x6_KHR           0x93B9
#define GL_COMPRESSED_RGBA_ASTC_10x8_KHR           0x93BA
#define GL_COMPRESSED_RGBA_ASTC_10x10_KHR          0x93BB
#define GL_COMPRESSED_RGBA_ASTC_12x10_KHR          0x93BC
#define GL_COMPRESSED_RGBA_ASTC_12x12_KHR          0x93BD
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR    0x93D0
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR    0x93D1
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR    0x93D2
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR    0x93D3
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR    0x93D4
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR    0x93D5
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR    0x93D6
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR    0x93D7
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR   0x93D8
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR   0x93D9
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR   0x93DA
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR  0x93DB
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR  0x93DC
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR  0x93DD

NV_NAMESPACE_BEGIN

enum TextureFormat
{
	Texture_None			= 0x00000,
    Texture_R               = 0x00100,
    Texture_RGB				= 0x00200,
    Texture_RGBA            = 0x00300,
    Texture_DXT1            = 0x01100,
    Texture_DXT3            = 0x01200,
    Texture_DXT5            = 0x01300,
    Texture_PVR4bRGB        = 0x01400,
    Texture_PVR4bRGBA       = 0x01500,
    Texture_ATC_RGB         = 0x01600,
    Texture_ATC_RGBA        = 0x01700,
    Texture_ETC1			= 0x01800,
	Texture_ETC2_RGB		= 0x01900,
	Texture_ETC2_RGBA		= 0x01A00,
	Texture_ASTC_4x4		= 0x01B00,	// single channel, 4x4 block encoded ASTC
	Texture_ASTC_6x6		= 0x01C00,	// single channel, 6x6 block encoded ASTC
    Texture_Depth           = 0x08000,

	Texture_TypeMask        = 0x0ff00,
    Texture_Compressed      = 0x01000,
    Texture_SamplesMask     = 0x000ff,
    Texture_RenderTarget    = 0x10000,
    Texture_GenMipmaps      = 0x20000
};

static bool TextureFormatToGlFormat(const int format, const bool useSrgbFormat, GLenum &glFormat, GLenum &glInternalFormat)
{
    switch (format & Texture_TypeMask) {
    case Texture_RGB:
        glFormat = GL_RGB;
        if (useSrgbFormat) {
            glInternalFormat = GL_SRGB8;
        } else {
            glInternalFormat = GL_RGB;
        }
        return true;

    case Texture_RGBA:
        glFormat = GL_RGBA;
        if (useSrgbFormat) {
            glInternalFormat = GL_SRGB8_ALPHA8;
        } else {
            glInternalFormat = GL_RGBA;
        }
        return true;

    case Texture_R:
        glInternalFormat = GL_R8;
        glFormat = GL_RED;
        return true;

    case Texture_DXT1:
        glFormat = glInternalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        return true;

    //unsupported on OpenGL ES:
    //case Texture_DXT3:  glFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
    //case Texture_DXT5:  glFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;

    case Texture_PVR4bRGB:
        glFormat = GL_RGB;
        glInternalFormat = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
        return true;

    case Texture_PVR4bRGBA:
        glFormat = GL_RGBA;
        glInternalFormat = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
        return true;

    case Texture_ETC1:
        glFormat = GL_RGB;
        if (useSrgbFormat) {
            // Note that ETC2 is backwards compatible with ETC1.
            glInternalFormat = GL_COMPRESSED_SRGB8_ETC2;
        } else {
            glInternalFormat = GL_ETC1_RGB8_OES;
        }
        return true;

    case Texture_ETC2_RGB:
        glFormat = GL_RGB;
        if (useSrgbFormat) {
            glInternalFormat = GL_COMPRESSED_SRGB8_ETC2;
        } else {
            glInternalFormat = GL_COMPRESSED_RGB8_ETC2;
        }
        return true;

    case Texture_ETC2_RGBA:
        glFormat = GL_RGBA;
        if (useSrgbFormat) {
            glInternalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC;
        } else {
            glInternalFormat = GL_COMPRESSED_RGBA8_ETC2_EAC;
        }
        return true;

    case Texture_ASTC_4x4:
        glFormat = GL_RGBA;
        glInternalFormat = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
        return true;

    case Texture_ASTC_6x6:
        glFormat = GL_RGBA;
        glInternalFormat = GL_COMPRESSED_RGBA_ASTC_6x6_KHR;
        return true;

    case Texture_ATC_RGB:
        glFormat = GL_RGB;
        glInternalFormat = GL_ATC_RGB_AMD;
        return true;

    case Texture_ATC_RGBA:
        glFormat = GL_RGBA;
        glInternalFormat = GL_ATC_RGBA_EXPLICIT_ALPHA_AMD;
        return true;
    }
	return false;
}

static bool GlFormatToTextureFormat(int &format, GLenum glFormat, GLenum glInternalFormat)
{
    if (glFormat == GL_RED && glInternalFormat == GL_R8) {
		format = Texture_R;
		return true;
	}

    if (glFormat == GL_RGB && (glInternalFormat == GL_RGB || glInternalFormat == GL_SRGB8)) {
		format = Texture_RGB;
		return true;
	}

    if (glFormat == GL_RGBA && (glInternalFormat == GL_RGBA || glInternalFormat == GL_SRGB8_ALPHA8)) {
		format = Texture_RGBA;
		return true;
	}

    if ((glFormat == 0 || glFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) && glInternalFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT) {
		format = Texture_DXT1;
		return true;
	}

    if ((glFormat == 0 || glFormat == GL_RGB) && glInternalFormat == GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG)
	{
		format = Texture_PVR4bRGB;
		return true;
	}

    if ((glFormat == 0 || glFormat == GL_RGBA) && glInternalFormat == GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG)
	{
		format = Texture_PVR4bRGBA;
		return true;
	}

    if ((glFormat == 0 || glFormat == GL_RGB) && (glInternalFormat == GL_ETC1_RGB8_OES || glInternalFormat == GL_COMPRESSED_SRGB8_ETC2))
	{
		format = Texture_ETC1;
		return true;
	}

    if ((glFormat == 0 || glFormat == GL_RGB) && (glInternalFormat == GL_COMPRESSED_RGB8_ETC2 || glInternalFormat == GL_COMPRESSED_SRGB8_ETC2))
	{
		format = Texture_ETC2_RGB;
		return true;
	}

    if ((glFormat == 0 || glFormat == GL_RGBA) && (glInternalFormat == GL_COMPRESSED_RGBA8_ETC2_EAC || glInternalFormat == GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC))
	{
		format = Texture_ETC2_RGBA;
		return true;
	}

    if ((glFormat == 0 || glFormat == GL_RGB) && glInternalFormat == GL_ATC_RGB_AMD)
	{
		format = Texture_ATC_RGB;
		return true;
	}

    if ((glFormat == 0 || glFormat == GL_RGBA) && glInternalFormat == GL_ATC_RGBA_EXPLICIT_ALPHA_AMD)
	{
		format = Texture_ATC_RGBA;
		return true;
    }

	return false;
}

static int32_t CalculateTextureSize(int format, int width, int height)
{
    switch (format & Texture_TypeMask) {
    case Texture_R: return width * height;
    case Texture_RGB: return width * height * 3;
    case Texture_RGBA: return width * height * 4;
    case Texture_ATC_RGB:
    case Texture_ETC1:
    case Texture_ETC2_RGB:
    case Texture_DXT1: {
        int bw = (width + 3) / 4, bh = (height + 3) / 4;
        return bw * bh * 8;
    }
    case Texture_ATC_RGBA:
    case Texture_ETC2_RGBA:
    case Texture_DXT3:
    case Texture_DXT5: {
        int blockWidth = (width + 3) / 4, blockHeight = (height + 3) / 4;
        return blockWidth * blockHeight * 16;
    }
    case Texture_PVR4bRGB:
    case Texture_PVR4bRGBA: {
        uint uwidth = (uint) width;
        uint uheight = (uint) height;
        uint minWidth = 8;
        uint minHeight = 8;

        // pad the dimensions
        uwidth = uwidth + ((-1 * uwidth) % minWidth);
        uheight = uheight + ((-1 * uheight) % minHeight);
        uint depth = 1;

        uint bpp = 4;
        uint bits = bpp * uwidth * uheight * depth;
        return (int) (bits / 8);
    }
    case Texture_ASTC_4x4: {
        int blocksWidth = (width + 3) / 4;
        int blocksHeight = (height + 3) / 4;
        return blocksWidth * blocksHeight * 16;
    }
    case Texture_ASTC_6x6:
    {
        int blocksWidth = ( width + 5 ) / 6;
        int blocksHeight = ( height + 5 ) / 6;
        return blocksWidth * blocksHeight * 16;
    }
    default:
        vAssert(false);
        break;
    }
    return 0;
}

static VTexture CreateGlTexture(const int format, const int width, const int height,
                        const void * data, uint dataSize,
						const int mipcount, const bool useSrgbFormat, const bool imageSizeStored )
{

	// vInfo("CreateGLTexture(): format " << NameForTextureFormat( static_cast< TextureFormat >( format ) ));

	GLenum glFormat;
	GLenum glInternalFormat;
	if ( !TextureFormatToGlFormat( format, useSrgbFormat, glFormat, glInternalFormat ) )
	{
		return VTexture( 0 );
	}

	if ( mipcount <= 0 )
	{
        vInfo("Invalid mip count " << mipcount);
		return VTexture( 0 );
	}

	// larger than this would require mipSize below to be a larger type
	if ( width <= 0 || width > 32768 || height <= 0 || height > 32768 )
	{
        vInfo("Invalid texture size (" << width << "x" << height << ")");
		return VTexture( 0 );
	}

	GLuint texId;
	glGenTextures( 1, &texId );
	glBindTexture( GL_TEXTURE_2D, texId );

	const unsigned char * level = (const unsigned char*)data;
	const unsigned char * endOfBuffer = level + dataSize;

	int w = width;
	int h = height;
	for ( int i = 0; i < mipcount; i++ )
	{
        int32_t mipSize = CalculateTextureSize( format, w, h );
		if ( imageSizeStored )
		{
			mipSize = *(const size_t *)level;

			level += 4;
			if ( level > endOfBuffer )
			{
                vInfo("Image data exceeds buffer size");
				glBindTexture( GL_TEXTURE_2D, 0 );
				return VTexture( texId, GL_TEXTURE_2D );
			}
		}

		if ( mipSize <= 0 || mipSize > endOfBuffer - level )
		{
            vInfo("Mip level " << i << " exceeds buffer size (" << mipSize << " > " << (endOfBuffer - level) << ")");
			glBindTexture( GL_TEXTURE_2D, 0 );
			return VTexture( texId, GL_TEXTURE_2D );
		}

		if ( format & Texture_Compressed )
		{
			glCompressedTexImage2D( GL_TEXTURE_2D, i, glInternalFormat, w, h, 0, mipSize, level );
            VEglDriver::logErrorsEnum( "Texture_Compressed" );
		}
		else
		{
			glTexImage2D( GL_TEXTURE_2D, i, glInternalFormat, w, h, 0, glFormat, GL_UNSIGNED_BYTE, level );
		}

		level += mipSize;
		if ( imageSizeStored )
		{
			level += 3 - ( ( mipSize + 3 ) % 4 );
			if ( level > endOfBuffer )
			{
                vInfo("Image data exceeds buffer size");
				glBindTexture( GL_TEXTURE_2D, 0 );
				return VTexture( texId, GL_TEXTURE_2D );
			}
		}

		w >>= 1;
		h >>= 1;
		if ( w < 1 ) { w = 1; }
		if ( h < 1 ) { h = 1; }
	}

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	// Surfaces look pretty terrible without trilinear filtering
	if ( mipcount <= 1 )
	{
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	}
	else
	{
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	}
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    VEglDriver::logErrorsEnum( "Texture load" );

    glBindTexture(GL_TEXTURE_2D, 0);

    return VTexture(texId, GL_TEXTURE_2D);
}

static VTexture CreateGlCubeTexture(const int format, const int width, const int height,
						const void * data, const size_t dataSize,
						const int mipcount, const bool useSrgbFormat, const bool imageSizeStored )
{

	assert( width == height );

	if ( mipcount <= 0 )
	{
        vInfo("Invalid mip count " << mipcount);
		return VTexture( 0 );
	}

	// larger than this would require mipSize below to be a larger type
	if ( width <= 0 || width > 32768 || height <= 0 || height > 32768 )
	{
        vInfo("Invalid texture size (" << width << "x" << height << ")");
		return VTexture( 0 );
	}

	GLenum glFormat;
	GLenum glInternalFormat;
	if ( !TextureFormatToGlFormat( format, useSrgbFormat, glFormat, glInternalFormat ) )
	{
		return VTexture( 0 );
	}

	GLuint texId;
	glGenTextures( 1, &texId );
	glBindTexture( GL_TEXTURE_CUBE_MAP, texId );

	const unsigned char * level = (const unsigned char*)data;
	const unsigned char * endOfBuffer = level + dataSize;

	for ( int i = 0; i < mipcount; i++ )
	{
		const int w = width >> i;
        int32_t mipSize = CalculateTextureSize( format, w, w );
		if ( imageSizeStored )
		{
			mipSize = *(const size_t *)level;
			level += 4;
			if ( level > endOfBuffer )
			{
                vInfo("Image data exceeds buffer size");
				glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );
				return VTexture( texId, GL_TEXTURE_CUBE_MAP );
			}
		}

		for ( int side = 0; side < 6; side++ )
		{
			if ( mipSize <= 0 || mipSize > endOfBuffer - level )
			{
                vInfo("Mip level " << i << " exceeds buffer size (" << mipSize << " > " << endOfBuffer - level << ")");
				glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );
                return VTexture( texId,  GL_TEXTURE_CUBE_MAP );
			}

			if ( format & Texture_Compressed )
			{
				glCompressedTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, i, glInternalFormat, w, w, 0, mipSize, level );
			}
			else
			{
				glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, i, glInternalFormat, w, w, 0, glFormat, GL_UNSIGNED_BYTE, level );
			}

			level += mipSize;
			if ( imageSizeStored )
			{
				level += 3 - ( ( mipSize + 3 ) % 4 );
				if ( level > endOfBuffer )
				{
                    vInfo("Image data exceeds buffer size");
					glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );
					return VTexture( texId, GL_TEXTURE_CUBE_MAP );
				}
			}
		}
	}

	// Surfaces look pretty terrible without trilinear filtering
	if ( mipcount <= 1 )
	{
		glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	}
	else
	{
		glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	}
	glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    VEglDriver::logErrorsEnum( "Texture load" );

	glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );

	return VTexture( texId, GL_TEXTURE_CUBE_MAP );
}

VTexture LoadRGBATextureFromMemory(const uchar *texture, const int width, const int height, const bool useSrgbFormat)
{
    const size_t dataSize = CalculateTextureSize(Texture_RGBA, width, height);
    return CreateGlTexture(Texture_RGBA, width, height, texture, dataSize, 1, useSrgbFormat, false);
}

VTexture LoadRGBTextureFromMemory(const uchar *texture, const int width, const int height, const bool useSrgbFormat)
{
    const size_t dataSize = CalculateTextureSize(Texture_RGB, width, height);
    return CreateGlTexture(Texture_RGB, width, height, texture, dataSize, 1, useSrgbFormat, false);
}

VTexture LoadRTextureFromMemory(const uchar *texture, const int width, const int height)
{
    const size_t dataSize = CalculateTextureSize(Texture_R, width, height );
    return CreateGlTexture(Texture_R, width, height, texture, dataSize, 1, false, false);
}

struct astcHeader
{
	unsigned char		magic[4];
	unsigned char		blockDim_x;
	unsigned char		blockDim_y;
	unsigned char		blockDim_z;
	unsigned char		xsize[3];
	unsigned char		ysize[3];
	unsigned char		zsize[3];
};

VTexture LoadASTCTextureFromMemory( uint8_t const * buffer, const size_t bufferSize, const int numPlanes )
{
	astcHeader const * header = reinterpret_cast< astcHeader const * >( buffer );

	int const w = ( (int)header->xsize[2] << 16 ) | ( (int)header->xsize[1] << 8 ) | ( (int)header->xsize[0] );
	int const h = ( (int)header->ysize[2] << 16 ) | ( (int)header->ysize[1] << 8 ) | ( (int)header->ysize[0] );

	// only supporting R channel for now
	vAssert( numPlanes == 1 );
	// only supporting 6x6x1 for now
	//vAssert( header->blockDim_x == 6 && header->blockDim_y == 6 && header->blockDim_z == 1 );
	vAssert( header->blockDim_x == 4 && header->blockDim_y == 4 && header->blockDim_z == 1 );
	if ( header->blockDim_z != 1 )
	{
		vAssert( header->blockDim_z == 1 );
		vInfo("Only 2D ASTC textures are supported");
		return VTexture();
	}

	TextureFormat format = Texture_ASTC_4x4;
	if ( header->blockDim_x == 6 && header->blockDim_y == 6 )
	{
		format = Texture_ASTC_6x6;
	}

    return CreateGlTexture(format, w, h, buffer, bufferSize, 1, false, false );
}

/*

PVR Container Format

Offset    Size       Name           Description
0x0000    4 [DWORD]  Version        0x03525650
0x0004    4 [DWORD]  Flags          0x0000 if no flags set
                                    0x0002 if colors within the texture
0x0008    8 [Union]  Pixel Format   This can either be one of several predetermined enumerated
                                    values (a DWORD) or a 4-character array and a 4-byte array (8 bytes).
                                    If the most significant 4 bytes of the 64-bit (8-byte) value are all zero,
                                    then it indicates that it is the enumeration with the following values:
                                    Value  Pixel Type
                                    0      PVRTC 2bpp RGB
                                    1      PVRTC 2bpp RGBA
                                    2      PVRTC 4bpp RGB
                                    3      PVRTC 4bpp RGBA
                                    4      PVRTC-II 2bpp
                                    5      PVRTC-II 4bpp
                                    6      ETC1
                                    7      DXT1 / BC1
                                    8      DXT2
                                    9      DXT3 / BC2
                                    10     DXT4
                                    11     DXT5 / BC3
                                    12     BC4
                                    13     BC5
                                    14     BC6
                                    15     BC7
                                    16     UYVY
                                    17     YUY2
                                    18     BW1bpp
                                    19     R9G9B9E5 Shared Exponent
                                    20     RGBG8888
                                    21     GRGB8888
                                    22     ETC2 RGB
                                    23     ETC2 RGBA
                                    24     ETC2 RGB A1
                                    25     EAC R11 Unsigned
                                    26     EAC R11 Signed
                                    27     EAC RG11 Unsigned
                                    28     EAC RG11 Signed
                                    If the most significant 4 bytes are not zero then the 8-byte character array
                                    indicates the pixel format as follows:
                                    The least significant 4 bytes indicate channel order, such as:
                                    { 'b', 'g', 'r', 'a' } or { 'b', 'g', 'r', '\0' }
                                    The most significant 4 bytes indicate the width of each channel in bits, as follows:
                                    { 4, 4, 4, 4 } or { 2, 2, 2, 2 }, or {5, 5, 5, 0 }
0x0010  4 [DWORD]    Color Space    This is an enumerated field, currently two values:
                                    Value   Color Space
                                    0       Linear RGB
                                    1       Standard RGB
0x0014  4 [DWORD]    Channel Type   This is another enumerated field:
                                    Value   Data Type
                                    0       Unsigned Byte Normalized
                                    1       Signed Byte Normalized
                                    2       Unsigned Byte
                                    3       Signed Byte
                                    4       Unsigned Short Normalized
                                    5       Signed Short Normalized
                                    6       Unsigned Short
                                    7       Signed Short
                                    8       Unsigned Integer Normalized
                                    9       Signed Integer Normalized
                                    10      Unsigned Integer
                                    11      Signed Integer
                                    12      Float (no size specified)
0x0018  4 [DWORD]    Height         Height of the image.
0x001C  4 [DWORD]    Width          Width of the image.
0x0020  4 [DWORD]    Depth          Depth of the image, in pixels.
0x0024  4 [DWORD]    Surface Count  The number of surfaces to this texture, used for texture arrays.
0x0028  4 [DWORD]    Face Count     The number of faces to this texture, used for cube maps.
0x002C  4 [DWORD]    MIP-Map Count  The number of MIP-Map levels, including a top level.
0x0030  4 [DWORD]    Metadata Size  The size, in bytes, of meta data that immediately follows this header.

*/

#pragma pack(1)
struct OVR_PVR_HEADER
{
    vuint32	Version;
    vuint32	Flags;
    vuint64  PixelFormat;
    vuint32  ColorSpace;
    vuint32  ChannelType;
    vuint32	Height;
    vuint32	Width;
    vuint32	Depth;
    vuint32  NumSurfaces;
    vuint32  NumFaces;
    vuint32  MipMapCount;
    vuint32  MetaDataSize;
};
#pragma pack()

VTexture LoadTexturePVR(const unsigned char * buffer, const int bufferLength,
						bool useSrgbFormat, bool noMipMaps, int & width, int & height )
{
	width = 0;
	height = 0;

	if ( bufferLength < ( int )( sizeof( OVR_PVR_HEADER ) ) )
	{
        vWarn("Invalid PVR file");
        return VTexture();
	}

	const OVR_PVR_HEADER & header = *( OVR_PVR_HEADER * )buffer;
	if ( header.Version != 0x03525650 )
	{
        vWarn("Invalid PVR file version");
        return VTexture();
	}

	int format = 0;
    switch (header.PixelFormat) {
    case 2:						format = Texture_PVR4bRGB;	break;
    case 3:						format = Texture_PVR4bRGBA;	break;
    case 6:						format = Texture_ETC1;		break;
    case 22:					format = Texture_ETC2_RGB;	break;
    case 23:					format = Texture_ETC2_RGBA;	break;
    case 578721384203708274ull:	format = Texture_RGBA;		break;
    default:
        vWarn("Unknown PVR texture format " << header.PixelFormat << "lu, size " << width << "x" << height);
        return VTexture();
	}

	// skip the metadata
	const vuint32 startTex = sizeof( OVR_PVR_HEADER ) + header.MetaDataSize;
	if ( ( startTex < sizeof( OVR_PVR_HEADER ) ) || ( startTex >= static_cast< size_t >( bufferLength ) ) )
	{
        vInfo("Invalid PVR header sizes");
		return VTexture( 0 );
	}

	const vuint32 mipCount = ( noMipMaps ) ? 1 : std::max( 1u, header.MipMapCount );

	width = header.Width;
	height = header.Height;

	if ( header.NumFaces == 1 )
	{
        return CreateGlTexture(format, width, height, buffer + startTex, bufferLength - startTex, mipCount, useSrgbFormat, false );
	}
	else if ( header.NumFaces == 6 )
	{
        return CreateGlCubeTexture(format, width, height, buffer + startTex, bufferLength - startTex, mipCount, useSrgbFormat, false );
	}
	else
	{
        vInfo("PVR file has unsupported number of faces " << header.NumFaces);
	}

	width = 0;
	height = 0;
    return VTexture();
}


unsigned char * LoadPVRBuffer( const char * fileName, int & width, int & height )
{
	width = 0;
	height = 0;

    std::fstream fileBuffer;
    fileBuffer.open(fileName);
    fileBuffer.seekg(0, std::ios_base::end);
    uint fileLength = 0;
    fileLength = fileBuffer.tellg();
    fileBuffer.seekg(0, std::ios_base::beg);
    void *buffer = NULL;
    buffer = malloc(fileLength);
    fileBuffer.read(reinterpret_cast<std::istream::char_type*>(buffer), fileLength);
    if ( fileLength < ( int )( sizeof( OVR_PVR_HEADER ) ) )
	{
		vInfo("Invalid PVR file");
        free( (void *)buffer );
        buffer = NULL;
        fileLength = 0;
		return NULL;
	}

    const OVR_PVR_HEADER & header = *( OVR_PVR_HEADER * )buffer;
	if ( header.Version != 0x03525650 )
	{
		vInfo("Invalid PVR file version");
        free( (void *)buffer );
        buffer = NULL;
        fileLength = 0;
		return NULL;
	}

	int format = 0;
	switch ( header.PixelFormat )
	{
        case 578721384203708274ull:	format = Texture_RGBA;		break;
		default:
			vInfo("Unknown PVR texture format " << header.PixelFormat << "lu, size " << width << "x" << height);
            free( (void *)buffer );
            buffer = NULL;
            fileLength = 0;
			return NULL;
	}

	// skip the metadata
	const vuint32 startTex = sizeof( OVR_PVR_HEADER ) + header.MetaDataSize;
    if ( ( startTex < sizeof( OVR_PVR_HEADER ) ) || ( startTex >= static_cast< size_t >( fileLength ) ) )
	{
		vInfo("Invalid PVR header sizes");
        free( (void *)buffer );
        buffer = NULL;
        fileLength = 0;
		return NULL;
	}

    size_t mipSize = CalculateTextureSize( format, header.Width, header.Height );

    const int outBufferSizeBytes = fileLength - startTex;

	if ( outBufferSizeBytes < 0 || mipSize > static_cast< size_t >( outBufferSizeBytes ) )
	{
        free( (void *)buffer );
        buffer = NULL;
        fileLength = 0;
		return NULL;
	}

	width = header.Width;
	height = header.Height;

	// skip the metadata
	unsigned char * outBuffer = ( unsigned char * )malloc( outBufferSizeBytes );
    memcpy( outBuffer, ( unsigned char * )buffer + startTex, outBufferSizeBytes );
    free( (void *)buffer );
    buffer = NULL;
    fileLength = 0;

	return outBuffer;
}


/*

KTX Container Format

KTX is a format for storing textures for OpenGL and OpenGL ES applications.
It is distinguished by the simplicity of the loader required to instantiate
a GL texture object from the file contents.

Byte[12] identifier
vuint32 endianness
vuint32 glType
vuint32 glTypeSize
vuint32 glFormat
Uint32 glInternalFormat
Uint32 glBaseInternalFormat
vuint32 pixelWidth
vuint32 pixelHeight
vuint32 pixelDepth
vuint32 numberOfArrayElements
vuint32 numberOfFaces
vuint32 numberOfMipmapLevels
vuint32 bytesOfKeyValueData

for each keyValuePair that fits in bytesOfKeyValueData
    vuint32   keyAndValueByteSize
    Byte     keyAndValue[keyAndValueByteSize]
    Byte     valuePadding[3 - ((keyAndValueByteSize + 3) % 4)]
end

for each mipmap_level in numberOfMipmapLevels*
    vuint32 imageSize;
    for each array_element in numberOfArrayElements*
       for each face in numberOfFaces
           for each z_slice in pixelDepth*
               for each row or row_of_blocks in pixelHeight*
                   for each pixel or block_of_pixels in pixelWidth
                       Byte data[format-specific-number-of-bytes]**
                   end
               end
           end
           Byte cubePadding[0-3]
       end
    end
    Byte mipPadding[3 - ((imageSize + 3) % 4)]
end

*/

#pragma pack(1)
struct OVR_KTX_HEADER
{
	uchar	identifier[12];
	vuint32	endianness;
	vuint32	glType;
	vuint32	glTypeSize;
	vuint32	glFormat;
	vuint32	glInternalFormat;
	vuint32	glBaseInternalFormat;
	vuint32	pixelWidth;
	vuint32	pixelHeight;
	vuint32	pixelDepth;
	vuint32	numberOfArrayElements;
	vuint32	numberOfFaces;
	vuint32	numberOfMipmapLevels;
	vuint32	bytesOfKeyValueData;
};
#pragma pack()

VTexture LoadTextureKTX(const unsigned char * buffer, const int bufferLength, bool useSrgbFormat, bool noMipMaps, int & width, int & height )
{
	width = 0;
	height = 0;

	if ( bufferLength < (int)( sizeof( OVR_KTX_HEADER ) ) )
	{
        vWarn("Invalid KTX file");
        return VTexture();
	}

	const uchar fileIdentifier[12] =
	{
        171, 75, 84, 88, 32, 49, 49, 187, 13, 10, 26, 10
	};

	const OVR_KTX_HEADER & header = *(OVR_KTX_HEADER *)buffer;
	if ( memcmp( header.identifier, fileIdentifier, sizeof( fileIdentifier ) ) != 0 )
	{
        vWarn("Invalid KTX file");
        return VTexture();
	}
	// only support little endian
	if ( header.endianness != 0x04030201 )
	{
        vWarn("KTX file has wrong endianess");
        return VTexture();
	}
	// only support compressed or unsigned byte
	if ( header.glType != 0 && header.glType != GL_UNSIGNED_BYTE )
	{
        vWarn("KTX file has unsupported glType " << header.glType);
        return VTexture();
	}
	// no support for texture arrays
	if ( header.numberOfArrayElements != 0 )
	{
        vWarn("KTX file has unsupported number of array elements " << header.numberOfArrayElements);
        return VTexture();
	}
	// derive the texture format from the GL format
	int format = 0;
	if ( !GlFormatToTextureFormat( format, header.glFormat, header.glInternalFormat ) )
	{
        vWarn("KTX file has unsupported glFormat " << header.glFormat << ", glInternalFormat " << header.glInternalFormat);
        return VTexture();
	}
	// skip the key value data
	const uintptr_t startTex = sizeof( OVR_KTX_HEADER ) + header.bytesOfKeyValueData;
	if ( ( startTex < sizeof( OVR_KTX_HEADER ) ) || ( startTex >= static_cast< size_t >( bufferLength ) ) )
	{
        vWarn("Invalid KTX header sizes");
        return VTexture();
	}

	width = header.pixelWidth;
	height = header.pixelHeight;

	const vuint32 mipCount = ( noMipMaps ) ? 1 : std::max( 1u, header.numberOfMipmapLevels );

	if ( header.numberOfFaces == 1 )
	{
        return CreateGlTexture(format, width, height, buffer + startTex, bufferLength - startTex, mipCount, useSrgbFormat, true );
	}
	else if ( header.numberOfFaces == 6 )
	{
        return CreateGlCubeTexture(format, width, height, buffer + startTex, bufferLength - startTex, mipCount, useSrgbFormat, true );
	}
	else
	{
        vWarn("KTX file has unsupported number of faces " << header.numberOfFaces);
	}

	width = 0;
	height = 0;
    return VTexture();
}

VTexture LoadTextureFromBuffer( const char * fileName, const void*  buffer, uint length,
        const VTexture::Flags & flags, int & width, int & height )
{
	const VString ext = VPath(fileName).extension().toLower();

    // vInfo("Loading texture buffer " << fileName << " (" << ext.toCString() << "), length " << buffer.Length);

	VTexture texId = 0;
	width = 0;
	height = 0;

    if ( fileName == NULL || buffer == NULL || length < 1 )
	{
		// can't load anything from an empty buffer
	}
	else if (	ext == "jpg" || ext == "tga" ||
				ext == "png" || ext == "bmp" ||
				ext == "psd" || ext == "gif" ||
				ext == "hdr" || ext == "pic" )
	{
		// Uncompressed files loaded by stb_image
		int comp;
        stbi_uc * image = stbi_load_from_memory( (unsigned char *)buffer, length, &width, &height, &comp, 4 );
		if ( image != NULL )
		{
            const size_t dataSize = CalculateTextureSize( Texture_RGBA, width, height );
            texId = CreateGlTexture(Texture_RGBA, width, height, image, dataSize,
                    1 /* one mip level */, flags & VTexture::UseSRGB, false );
			free( image );
            if ( !( flags & VTexture::NoMipmaps ) )
			{
                glBindTexture(texId.target(), texId.id());
                glGenerateMipmap(texId.target());
                glTexParameteri(texId.target(), GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
			}
	    }
	}
	else if ( ext == "pvr" )
	{
        texId = LoadTexturePVR((const unsigned char *) buffer, length,
                        ( flags & VTexture::UseSRGB ),
                        ( flags & VTexture::NoMipmaps ),
						width, height );
	}
	else if ( ext == "ktx" )
	{
        texId = LoadTextureKTX((const unsigned char *)buffer, length,
                        ( flags & VTexture::UseSRGB ),
                        ( flags & VTexture::NoMipmaps ),
						width, height );
	}
	else if ( ext == "pkm" )
	{
		vInfo("PKM format not supported");
	}
	else
	{
        vInfo("unsupported file extension " << ext);
	}

	// Create a default texture if the load failed
    if ( texId.id() == 0 )
	{
    	vWarn("Failed to load " << fileName);
        if ( ( flags & VTexture::NoDefault ) == 0 )
    	{
            static uchar defaultTexture[8 * 8 * 3] =
			{
					255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255,
					255,255,255,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64, 255,255,255,
					255,255,255,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64, 255,255,255,
					255,255,255,  64, 64, 64,  64, 64, 64, 255,255,255, 255,255,255,  64, 64, 64,  64, 64, 64, 255,255,255,
					255,255,255,  64, 64, 64,  64, 64, 64, 255,255,255, 255,255,255,  64, 64, 64,  64, 64, 64, 255,255,255,
					255,255,255,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64, 255,255,255,
					255,255,255,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64, 255,255,255,
					255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255
			};
            texId = LoadRGBTextureFromMemory( defaultTexture, 8, 8, flags & VTexture::UseSRGB );
    	}
	}

	return texId;
}

void MakeTextureClamped(const VTexture &texture)
{
    glBindTexture(texture.target(), texture.id());
    glTexParameteri(texture.target(), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(texture.target(), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(texture.target(), 0);
}

void MakeTextureLodClamped(const VTexture &texture, int maxLod)
{
    glBindTexture(texture.target(), texture.id());
    glTexParameteri(texture.target(), GL_TEXTURE_MAX_LEVEL, maxLod);
    glBindTexture(texture.target(), 0);
}

void MakeTextureTrilinear(const VTexture &texture)
{
    glBindTexture(texture.target(), texture.id());
    glTexParameteri(texture.target(), GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameteri(texture.target(), GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glBindTexture(texture.target(), 0);
}

void MakeTextureLinear(const VTexture &texture)
{
    glBindTexture(texture.target(), texture.id());
    glTexParameteri(texture.target(), GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri(texture.target(), GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glBindTexture(texture.target(), 0);
}

void MakeTextureAniso(const VTexture &texture, float maxAniso)
{
    glBindTexture(texture.target(), texture.id());
    glTexParameterf(texture.target(), GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
    glBindTexture(texture.target(), 0);
}

void BuildTextureMipmaps(const VTexture &texture)
{
    glBindTexture(texture.target(), texture.id());
    glGenerateMipmap(texture.target());
    glBindTexture(texture.target(), 0);
}

struct VTexture::Private
{
    uint id;
    uint target;
    int width;
    int height;

    Private()
        : id(0)
        , target(0)
        , width(0)
        , height(0)
    {
    }

    void load(const VPath &path, const VByteArray &data, const VTexture::Flags &flags)
    {
        const VString ext = path.extension().toLower();

        if (ext.isEmpty() || data.isEmpty()) {
            // can't load anything from an empty buffer
            return;
        }

        if (ext == "jpg" || ext == "tga" || ext == "png" || ext == "bmp"
            || ext == "psd" || ext == "gif" || ext == "hdr" || ext == "pic") {
            // Uncompressed files loaded by stb_image
            VImage image(data);
            if (image.isValid()) {
                width = image.width();
                height = image.height();
                create2D(Texture_RGBA, image.data(), image.length(), 1, flags & VTexture::UseSRGB, false);

                if (!(flags & VTexture::NoMipmaps)) {
                    glBindTexture(target, id);
                    glGenerateMipmap(target);
                    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                }
            }
        } else if (ext == "pvr") {
            loadPVR(data, flags & VTexture::UseSRGB, flags & VTexture::NoMipmaps);
        } else if (ext == "ktx") {
            loadKTX(data, flags & VTexture::UseSRGB, flags & VTexture::NoMipmaps);
        } else {
            vWarn("unsupported file extension " << ext);
        }

        // Create a default texture if the load failed
        if (id == 0) {
            vWarn("Failed to load " << path);
            if (!(flags & VTexture::NoDefault)) {
                static uchar defaultTexture[8 * 8 * 3] = {
                    255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255,
                    255,255,255,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64, 255,255,255,
                    255,255,255,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64, 255,255,255,
                    255,255,255,  64, 64, 64,  64, 64, 64, 255,255,255, 255,255,255,  64, 64, 64,  64, 64, 64, 255,255,255,
                    255,255,255,  64, 64, 64,  64, 64, 64, 255,255,255, 255,255,255,  64, 64, 64,  64, 64, 64, 255,255,255,
                    255,255,255,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64, 255,255,255,
                    255,255,255,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64, 255,255,255,
                    255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255
                };
                const size_t dataSize = CalculateTextureSize(Texture_RGB, width, height);
                width = height = 8;
                create2D(Texture_RGB, defaultTexture, dataSize, 1, true, false);
            }
        }
    }

    void create2D(int format, const uchar *data, uint dataSize, int mipCount, bool useSrgbFormat, bool imageSizeStored)
    {
        GLenum glFormat;
        GLenum glInternalFormat;
        if (!TextureFormatToGlFormat(format, useSrgbFormat, glFormat, glInternalFormat)) {
            return;
        }

        if (mipCount <= 0) {
            vWarn("Invalid mip count " << mipCount);
            return;
        }

        // larger than this would require mipSize below to be a larger type
        if (width <= 0 || width > 32768 || height <= 0 || height > 32768) {
            vWarn("Invalid texture size (" << width << "x" << height << ")");
            return;
        }

        GLuint texId;
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_2D, texId);

        const uchar *level = data;
        const uchar *endOfBuffer = level + dataSize;

        int w = width;
        int h = height;
        for (int i = 0; i < mipCount; i++) {
            int32_t mipSize = CalculateTextureSize(format, w, h);
            if (imageSizeStored) {
                mipSize = * (const size_t *) level;

                level += 4;
                if (level > endOfBuffer) {
                    vWarn("Image data exceeds buffer size");
                    glBindTexture(GL_TEXTURE_2D, 0);
                    id = texId;
                    target = GL_TEXTURE_2D;
                    return;
                }
            }

            if (mipSize <= 0 || mipSize > endOfBuffer - level) {
                vWarn("Mip level " << i << " exceeds buffer size (" << mipSize << " > " << (endOfBuffer - level) << ")");
                glBindTexture(GL_TEXTURE_2D, 0);

                id = texId;
                target = GL_TEXTURE_2D;
                return;
            }

            if (format & Texture_Compressed) {
                glCompressedTexImage2D(GL_TEXTURE_2D, i, glInternalFormat, w, h, 0, mipSize, level);
                VEglDriver::logErrorsEnum("Texture_Compressed");
            } else {
                glTexImage2D(GL_TEXTURE_2D, i, glInternalFormat, w, h, 0, glFormat, GL_UNSIGNED_BYTE, level);
            }

            level += mipSize;
            if (imageSizeStored) {
                level += 3 - ((mipSize + 3) % 4);
                if (level > endOfBuffer) {
                    vWarn("Image data exceeds buffer size");
                    glBindTexture(GL_TEXTURE_2D, 0);

                    id = texId;
                    target = GL_TEXTURE_2D;
                    return;
                }
            }

            w >>= 1;
            h >>= 1;
            if (w < 1) {
                w = 1;
            }
            if (h < 1) {
                h = 1;
            }
        }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        // Surfaces look pretty terrible without trilinear filtering
        if (mipCount <= 1) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        } else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        VEglDriver::logErrorsEnum("Texture load");

        glBindTexture(GL_TEXTURE_2D, 0);

        id = texId;
        target = GL_TEXTURE_2D;
    }

    void createCube(const int format, const uchar *data, uint dataSize, const int mipCount, const bool useSrgbFormat, const bool imageSizeStored)
    {
        vAssert(width == height);

        if (mipCount <= 0) {
            vInfo("Invalid mip count " << mipCount);
            return;
        }

        // larger than this would require mipSize below to be a larger type
        height = width;
        if (width <= 0 || width > 32768) {
            vInfo("Invalid texture size (" << width << "x" << height << ")");
            return;
        }

        GLenum glFormat;
        GLenum glInternalFormat;
        if (!TextureFormatToGlFormat(format, useSrgbFormat, glFormat, glInternalFormat)) {
            return;
        }

        GLuint texId;
        glGenTextures(1, &texId);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texId);

        const uchar *level = data;
        const uchar *endOfBuffer = level + dataSize;

        for (int i = 0; i < mipCount; i++) {
            const int w = width >> i;
            int32_t mipSize = CalculateTextureSize(format, w, w);
            if (imageSizeStored) {
                mipSize = * (const size_t *) level;
                level += 4;
                if (level > endOfBuffer) {
                    vWarn("Image data exceeds buffer size");
                    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

                    id = texId;
                    target = GL_TEXTURE_CUBE_MAP;
                    return;
                }
            }

            for (int side = 0; side < 6; side++) {
                if (mipSize <= 0 || mipSize > endOfBuffer - level) {
                    vWarn("Mip level " << i << " exceeds buffer size (" << mipSize << " > " << endOfBuffer - level << ")");
                    glBindTexture( GL_TEXTURE_CUBE_MAP, 0);

                    id = texId;
                    target = GL_TEXTURE_CUBE_MAP;
                    return;
                }

                if (format & Texture_Compressed) {
                    glCompressedTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, i, glInternalFormat, w, w, 0, mipSize, level);
                } else {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, i, glInternalFormat, w, w, 0, glFormat, GL_UNSIGNED_BYTE, level);
                }

                level += mipSize;
                if (imageSizeStored) {
                    level += 3 - ((mipSize + 3) % 4);
                    if (level > endOfBuffer) {
                        vWarn("Image data exceeds buffer size");
                        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

                        id = texId;
                        target = GL_TEXTURE_CUBE_MAP;
                        return;
                    }
                }
            }
        }

        // Surfaces look pretty terrible without trilinear filtering
        if (mipCount <= 1) {
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        } else {
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        VEglDriver::logErrorsEnum("Texture load");

        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        id = texId;
        target = GL_TEXTURE_CUBE_MAP;
    }

    void loadPVR(const VByteArray &buffer, bool useSrgbFormat, bool noMipMaps)
    {
        width = 0;
        height = 0;

        if (buffer.size() < sizeof(OVR_PVR_HEADER)) {
            vWarn("Invalid PVR file");
            return;
        }

        const OVR_PVR_HEADER *header = reinterpret_cast<const OVR_PVR_HEADER *>(buffer.data());
        if (header->Version != 0x03525650) {
            vWarn("Invalid PVR file version");
            return;
        }

        int format = 0;
        switch (header->PixelFormat) {
        case 2: format = Texture_PVR4bRGB; break;
        case 3: format = Texture_PVR4bRGBA; break;
        case 6: format = Texture_ETC1; break;
        case 22: format = Texture_ETC2_RGB; break;
        case 23: format = Texture_ETC2_RGBA; break;
        case 578721384203708274ull: format = Texture_RGBA; break;
        default:
            vWarn("Unknown PVR texture format " << header->PixelFormat << "lu, size " << width << "x" << height);
            return;
        }

        // skip the metadata
        const vuint32 startTex = sizeof(OVR_PVR_HEADER) + header->MetaDataSize;
        if ((startTex < sizeof(OVR_PVR_HEADER)) || (startTex >= buffer.size())) {
            vWarn("Invalid PVR header sizes");
            return;
        }

        const vuint32 mipCount = (noMipMaps) ? 1 : std::max(1u, header->MipMapCount);

        width = header->Width;
        height = header->Height;

        const uchar *data = reinterpret_cast<const uchar *>(buffer.data());
        if (header->NumFaces == 1) {
            create2D(format, data + startTex, buffer.size() - startTex, mipCount, useSrgbFormat, false);
        } else if (header->NumFaces == 6) {
            createCube(format, data + startTex, buffer.size() - startTex, mipCount, useSrgbFormat, false);
        } else {
            vWarn("PVR file has unsupported number of faces " << header->NumFaces);

            width = 0;
            height = 0;
        }
    }

    void loadKTX(const VByteArray &buffer, bool useSrgbFormat, bool noMipMaps)
    {
        width = 0;
        height = 0;

        if (buffer.size() < sizeof(OVR_KTX_HEADER)) {
            vWarn("Invalid KTX file");
            return;
        }

        const uchar fileIdentifier[12] = {
            171, 75, 84, 88, 32, 49, 49, 187, 13, 10, 26, 10
        };

        const OVR_KTX_HEADER *header = reinterpret_cast<const OVR_KTX_HEADER *>(buffer.data());
        if (memcmp(header->identifier, fileIdentifier, sizeof(fileIdentifier)) != 0) {
            vWarn("Invalid KTX file");
            return;
        }

        // only support little endian
        if (header->endianness != 0x04030201) {
            vWarn("KTX file has wrong endianess");
            return;
        }

        // only support compressed or unsigned byte
        if (header->glType != 0 && header->glType != GL_UNSIGNED_BYTE) {
            vWarn("KTX file has unsupported glType " << header->glType);
            return;
        }

        // no support for texture arrays
        if (header->numberOfArrayElements != 0) {
            vWarn("KTX file has unsupported number of array elements " << header->numberOfArrayElements);
            return;
        }

        // derive the texture format from the GL format
        int format = 0;
        if (!GlFormatToTextureFormat(format, header->glFormat, header->glInternalFormat)) {
            vWarn("KTX file has unsupported glFormat " << header->glFormat << ", glInternalFormat " << header->glInternalFormat);
            return;
        }

        // skip the key value data
        const uintptr_t startTex = sizeof(OVR_KTX_HEADER) + header->bytesOfKeyValueData;
        if ((startTex < sizeof(OVR_KTX_HEADER)) || (startTex >= buffer.size())) {
            vWarn("Invalid KTX header sizes");
            return;
        }

        width = header->pixelWidth;
        height = header->pixelHeight;

        const vuint32 mipCount = noMipMaps ? 1 : std::max(1u, header->numberOfMipmapLevels);

        const uchar *bufferData = reinterpret_cast<const uchar *>(buffer.data());
        if (header->numberOfFaces == 1) {
            create2D(format, bufferData + startTex, buffer.size() - startTex, mipCount, useSrgbFormat, true);
        } else if (header->numberOfFaces == 6) {
            createCube(format, bufferData + startTex, buffer.size() - startTex, mipCount, useSrgbFormat, true);
        } else {
            vWarn("KTX file has unsupported number of faces " << header->numberOfFaces);

            width = 0;
            height = 0;
        }
    }
};

// Not declared inline in the header to avoid having to use GL_TEXTURE_2D
VTexture::VTexture()
    : d(new Private)
{
}

VTexture::VTexture(uint id)
    : d(new Private)
{
    d->id = id;
    d->target = GL_TEXTURE_2D;
}

VTexture::VTexture(uint id, uint target)
    : d(new Private)
{
    d->id = id;
    d->target = target;
}

VTexture::VTexture(const VTexture &source)
    : d(new Private)
{
    d->id = source.id();
    d->target = source.target();
}

VTexture::VTexture(VTexture &&source)
    : d(source.d)
{
    source.d = nullptr;
}

VTexture::VTexture(VFile *file, const Flags &flags)
    : d(new Private)
{
    d->load(file->path(), file->readAll(), flags);
}

VTexture::VTexture(const VResource &resource, const Flags &flags)
    : d(new Private)
{
    d->load(resource.path(), resource.data(), flags);
}

VTexture::~VTexture()
{
    if (d) {
        //glDeleteTextures(1, &d->id);
        delete d;
    }
}

VTexture &VTexture::operator=(const VTexture &source)
{
    d->id = source.id();
    d->target = source.target();
    return *this;
}

VTexture &VTexture::operator=(VTexture &&source)
{
    delete d;
    d = source.d;
    source.d = nullptr;
    return *this;
}

const uint &VTexture::id() const
{
    return d->id;
}

const uint &VTexture::target() const
{
    return d->target;
}

int VTexture::width() const
{
    return d->width;
}

int VTexture::height() const
{
    return d->height;
}

NV_NAMESPACE_END
