#include "VColorConverter.h"
#include "VImageColor.h"
#include "VString.h"

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

namespace NervGear {


static bool TextureFormatToGlFormat( const int format, const bool useSrgbFormat, GLenum & glFormat, GLenum & glInternalFormat )
{
    switch ( format & Texture_TypeMask )
    {
        case Texture_RGB:
        {
            glFormat = GL_RGB;
            if ( useSrgbFormat )
            {
                glInternalFormat = GL_SRGB8;
//				vInfo("GL texture format is GL_RGB / GL_SRGB8");
            }
            else
            {
                glInternalFormat = GL_RGB;
//				vInfo("GL texture format is GL_RGB / GL_RGB");
            }
            return true;
        }
        case Texture_RGBA:
        {
            glFormat = GL_RGBA;
            if ( useSrgbFormat )
            {
                glInternalFormat = GL_SRGB8_ALPHA8;
//				vInfo("GL texture format is GL_RGBA / GL_SRGB8_ALPHA8");
            }
            else
            {
                glInternalFormat = GL_RGBA;
//				vInfo("GL texture format is GL_RGBA / GL_RGBA");
            }
            return true;
        }
        case Texture_R:
        {
            glInternalFormat = GL_R8;
            glFormat = GL_RED;
//			vInfo("GL texture format is GL_R8");
            return true;
        }
        case Texture_DXT1:
        {
            glFormat = glInternalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
//			vInfo("GL texture format is GL_COMPRESSED_RGBA_S3TC_DXT1_EXT");
            return true;
        }
    // unsupported on OpenGL ES:
    //    case Texture_DXT3:  glFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
    //    case Texture_DXT5:  glFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
        case Texture_PVR4bRGB:
        {
            glFormat = GL_RGB;
            glInternalFormat = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
//			vInfo("GL texture format is GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG");
            return true;
        }
        case Texture_PVR4bRGBA:
        {
            glFormat = GL_RGBA;
            glInternalFormat = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
//			vInfo("GL texture format is GL_RGBA / GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG");
            return true;
        }
        case Texture_ETC1:
        {
            glFormat = GL_RGB;
            if ( useSrgbFormat )
            {
                // Note that ETC2 is backwards compatible with ETC1.
                glInternalFormat = GL_COMPRESSED_SRGB8_ETC2;
//				vInfo("GL texture format is GL_RGB / GL_COMPRESSED_SRGB8_ETC2");
            }
            else
            {
                glInternalFormat = GL_ETC1_RGB8_OES;
//				vInfo("GL texture format is GL_RGB / GL_ETC1_RGB8_OES");
            }
            return true;
        }
        case Texture_ETC2_RGB:
        {
            glFormat = GL_RGB;
            if ( useSrgbFormat )
            {
                glInternalFormat = GL_COMPRESSED_SRGB8_ETC2;
//				vInfo("GL texture format is GL_RGB / GL_COMPRESSED_SRGB8_ETC2");
            }
            else
            {
                glInternalFormat = GL_COMPRESSED_RGB8_ETC2;
//				vInfo("GL texture format is GL_RGB / GL_COMPRESSED_RGB8_ETC2");
            }
            return true;
        }
        case Texture_ETC2_RGBA:
        {
            glFormat = GL_RGBA;
            if ( useSrgbFormat )
            {
                glInternalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC;
//				vInfo("GL texture format is GL_RGBA / GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC");
            }
            else
            {
                glInternalFormat = GL_COMPRESSED_RGBA8_ETC2_EAC;
//				vInfo("GL texture format is GL_RGBA / GL_COMPRESSED_RGBA8_ETC2_EAC");
            }
            return true;
        }
        case Texture_ASTC_4x4:
        {
            glFormat = GL_RGBA;
            glInternalFormat = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
            return true;
        }
        case Texture_ASTC_6x6:
        {
            glFormat = GL_RGBA;
            glInternalFormat = GL_COMPRESSED_RGBA_ASTC_6x6_KHR;
            return true;
        }
        case Texture_ATC_RGB:
        {
            glFormat = GL_RGB;
            glInternalFormat = GL_ATC_RGB_AMD;
//			vInfo("GL texture format is GL_RGB / GL_ATC_RGB_AMD");
            return true;
        }
        case Texture_ATC_RGBA:
        {
            glFormat = GL_RGBA;
            glInternalFormat = GL_ATC_RGBA_EXPLICIT_ALPHA_AMD;
//			vInfo("GL texture format is GL_RGBA / GL_ATC_RGBA_EXPLICIT_ALPHA_AMD");
            return true;
        }
    }
    return false;
}

static bool GlFormatToTextureFormat( int & format, const GLenum glFormat, const GLenum glInternalFormat )
{
    if ( glFormat == GL_RED && glInternalFormat == GL_R8 )
    {
        format = Texture_R;
        return true;
    }
    if ( glFormat == GL_RGB && ( glInternalFormat == GL_RGB || glInternalFormat == GL_SRGB8 ) )
    {
        format = Texture_RGB;
        return true;
    }
    if ( glFormat == GL_RGBA && ( glInternalFormat == GL_RGBA || glInternalFormat == GL_SRGB8_ALPHA8 ) )
    {
        format = Texture_RGBA;
        return true;
    }
    if ( ( glFormat == 0 || glFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ) && glInternalFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT )
    {
        format = Texture_DXT1;
        return true;
    }
    if ( ( glFormat == 0 || glFormat == GL_RGB ) && glInternalFormat == GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG )
    {
        format = Texture_PVR4bRGB;
        return true;
    }
    if ( ( glFormat == 0 || glFormat == GL_RGBA ) && glInternalFormat == GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG )
    {
        format = Texture_PVR4bRGBA;
        return true;
    }
    if ( ( glFormat == 0 || glFormat == GL_RGB ) && ( glInternalFormat == GL_ETC1_RGB8_OES || glInternalFormat == GL_COMPRESSED_SRGB8_ETC2 ) )
    {
        format = Texture_ETC1;
        return true;
    }
    if ( ( glFormat == 0 || glFormat == GL_RGB ) && ( glInternalFormat == GL_COMPRESSED_RGB8_ETC2 || glInternalFormat == GL_COMPRESSED_SRGB8_ETC2 ) )
    {
        format = Texture_ETC2_RGB;
        return true;
    }
    if ( ( glFormat == 0 || glFormat == GL_RGBA ) && ( glInternalFormat == GL_COMPRESSED_RGBA8_ETC2_EAC || glInternalFormat == GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC ) )
    {
        format = Texture_ETC2_RGBA;
        return true;
    }
    if ( ( glFormat == 0 || glFormat == GL_RGB ) && glInternalFormat == GL_ATC_RGB_AMD )
    {
        format = Texture_ATC_RGB;
        return true;
    }
    if ( ( glFormat == 0 || glFormat == GL_RGBA ) && glInternalFormat == GL_ATC_RGBA_EXPLICIT_ALPHA_AMD )
    {
        format = Texture_ATC_RGBA;
        return true;
    }
    return false;
}



//! converts a monochrome bitmap to A1R5G5B5 data
void CColorConverter::convert1BitTo16Bit(const char* in, short* out, int width, int height, int linepad, bool flip)
{
    if (!in || !out)
        return;

    if (flip)
        out += width * height;

    for (int y=0; y<height; ++y)
    {
        int shift = 7;
        if (flip)
            out -= width;

        for (int x=0; x<width; ++x)
        {
            out[x] = *in>>shift & 0x01 ? (short)0xffff : (short)0x8000;

            if ((--shift)<0) // 8 pixel done
            {
                shift=7;
                ++in;
            }
        }

        if (shift != 7) // width did not fill last byte
            ++in;

        if (!flip)
            out += width;
        in += linepad;
    }
}



//! converts a 4 bit palettized image to A1R5G5B5
void CColorConverter::convert4BitTo16Bit(const char* in, short* out, int width, int height, const int* palette, int linepad, bool flip)
{
    if (!in || !out || !palette)
        return;

    if (flip)
        out += width*height;

    for (int y=0; y<height; ++y)
    {
        int shift = 4;
        if (flip)
            out -= width;

        for (int x=0; x<width; ++x)
        {
            out[x] = X8R8G8B8toA1R5G5B5(palette[(char)((*in >> shift) & 0xf)]);

            if (shift==0)
            {
                shift = 4;
                ++in;
            }
            else
                shift = 0;
        }

        if (shift == 0) // odd width
            ++in;

        if (!flip)
            out += width;
        in += linepad;
    }
}



//! converts a 8 bit palettized image into A1R5G5B5
void CColorConverter::convert8BitTo16Bit(const char* in, short* out, int width, int height, const int* palette, int linepad, bool flip)
{
    if (!in || !out || !palette)
        return;

    if (flip)
        out += width * height;

    for (int y=0; y<height; ++y)
    {
        if (flip)
            out -= width; // one line back
        for (int x=0; x<width; ++x)
        {
            out[x] = X8R8G8B8toA1R5G5B5(palette[(char)(*in)]);
            ++in;
        }
        if (!flip)
            out += width;
        in += linepad;
    }
}

//! converts a 8 bit palettized or non palettized image (A8) into R8G8B8
void CColorConverter::convert8BitTo24Bit(const char* in, char* out, int width, int height, const char* palette, int linepad, bool flip)
{
    if (!in || !out )
        return;

    const int lineWidth = 3 * width;
    if (flip)
        out += lineWidth * height;

    for (int y=0; y<height; ++y)
    {
        if (flip)
            out -= lineWidth; // one line back
        for (int x=0; x< lineWidth; x += 3)
        {
            if ( palette )
            {
#ifdef __BIG_ENDIAN__
                out[x+0] = palette[ (in[0] << 2 ) + 0];
                out[x+1] = palette[ (in[0] << 2 ) + 1];
                out[x+2] = palette[ (in[0] << 2 ) + 2];
#else
                out[x+0] = palette[ (in[0] << 2 ) + 2];
                out[x+1] = palette[ (in[0] << 2 ) + 1];
                out[x+2] = palette[ (in[0] << 2 ) + 0];
#endif
            }
            else
            {
                out[x+0] = in[0];
                out[x+1] = in[0];
                out[x+2] = in[0];
            }
            ++in;
        }
        if (!flip)
            out += lineWidth;
        in += linepad;
    }
}

//! converts a 8 bit palettized or non palettized image (A8) into R8G8B8
void CColorConverter::convert8BitTo32Bit(const char* in, char* out, int width, int height, const char* palette, int linepad, bool flip)
{
    if (!in || !out )
        return;

    const uint lineWidth = 4 * width;
    if (flip)
        out += lineWidth * height;

    uint x;
    register uint c;
    for (uint y=0; y < (uint) height; ++y)
    {
        if (flip)
            out -= lineWidth; // one line back

        if ( palette )
        {
            for (x=0; x < (uint) width; x += 1)
            {
                c = in[x];
                ((uint*)out)[x] = ((uint*)palette)[ c ];
            }
        }
        else
        {
            for (x=0; x < (uint) width; x += 1)
            {
                c = in[x];
#ifdef __BIG_ENDIAN__
                ((uint*)out)[x] = c << 24 | c << 16 | c << 8 | 0x000000FF;
#else
                ((uint*)out)[x] = 0xFF000000 | c << 16 | c << 8 | c;
#endif
            }

        }

        if (!flip)
            out += lineWidth;
        in += width + linepad;
    }
}



//! converts 16bit data to 16bit data
void CColorConverter::convert16BitTo16Bit(const short* in, short* out, int width, int height, int linepad, bool flip)
{
    if (!in || !out)
        return;

    if (flip)
        out += width * height;

    for (int y=0; y<height; ++y)
    {
        if (flip)
            out -= width;
#ifdef __BIG_ENDIAN__
        for (int x=0; x<width; ++x)
            out[x]=os::Byteswap::byteswap(in[x]);
#else
        memcpy(out, in, width*sizeof(short));
#endif
        if (!flip)
            out += width;
        in += width;
        in += linepad;
    }
}



//! copies R8G8B8 24bit data to 24bit data
void CColorConverter::convert24BitTo24Bit(const char* in, char* out, int width, int height, int linepad, bool flip, bool bgr)
{
    if (!in || !out)
        return;

    const int lineWidth = 3 * width;
    if (flip)
        out += lineWidth * height;

    for (int y=0; y<height; ++y)
    {
        if (flip)
            out -= lineWidth;
        if (bgr)
        {
            for (int x=0; x<lineWidth; x+=3)
            {
                out[x+0] = in[x+2];
                out[x+1] = in[x+1];
                out[x+2] = in[x+0];
            }
        }
        else
        {
            memcpy(out,in,lineWidth);
        }
        if (!flip)
            out += lineWidth;
        in += lineWidth;
        in += linepad;
    }
}



//! Resizes the surface to a new size and converts it at the same time
//! to an A8R8G8B8 format, returning the pointer to the new buffer.
void CColorConverter::convert16bitToA8R8G8B8andResize(const short* in, int* out, int newWidth, int newHeight, int currentWidth, int currentHeight)
{
    if (!newWidth || !newHeight)
        return;

    // note: this is very very slow. (i didn't want to write a fast version.
    // but hopefully, nobody wants to convert surfaces every frame.

    float sourceXStep = (float)currentWidth / (float)newWidth;
    float sourceYStep = (float)currentHeight / (float)newHeight;
    float sy;
    int t;

    for (int x=0; x<newWidth; ++x)
    {
        sy = 0.0f;

        for (int y=0; y<newHeight; ++y)
        {
            t = in[(int)(((int)sy)*currentWidth + x*sourceXStep)];
            t = (((t >> 15)&0x1)<<31) |	(((t >> 10)&0x1F)<<19) |
                (((t >> 5)&0x1F)<<11) |	(t&0x1F)<<3;
            out[(int)(y*newWidth + x)] = t;

            sy+=sourceYStep;
        }
    }
}



//! copies X8R8G8B8 32 bit data
void CColorConverter::convert32BitTo32Bit(const int* in, int* out, int width, int height, int linepad, bool flip)
{
    if (!in || !out)
        return;

    if (flip)
        out += width * height;

    for (int y=0; y<height; ++y)
    {
        if (flip)
            out -= width;
#ifdef __BIG_ENDIAN__
        for (int x=0; x<width; ++x)
            out[x]=os::Byteswap::byteswap(in[x]);
#else
        memcpy(out, in, width*sizeof(int));
#endif
        if (!flip)
            out += width;
        in += width;
        in += linepad;
    }
}



void CColorConverter::convert_A1R5G5B5toR8G8B8(const void* sP, int sN, void* dP)
{
    ushort* sB = (ushort*)sP;
    char * dB = (char *)dP;

    for (int x = 0; x < sN; ++x)
    {
        dB[2] = (*sB & 0x7c00) >> 7;
        dB[1] = (*sB & 0x03e0) >> 2;
        dB[0] = (*sB & 0x1f) << 3;

        sB += 1;
        dB += 3;
    }
}

void CColorConverter::convert_A1R5G5B5toB8G8R8(const void* sP, int sN, void* dP)
{
    ushort* sB = (ushort*)sP;
    char * dB = (char *)dP;

    for (int x = 0; x < sN; ++x)
    {
        dB[0] = (*sB & 0x7c00) >> 7;
        dB[1] = (*sB & 0x03e0) >> 2;
        dB[2] = (*sB & 0x1f) << 3;

        sB += 1;
        dB += 3;
    }
}

void CColorConverter::convert_A1R5G5B5toA8R8G8B8(const void* sP, int sN, void* dP)
{
    ushort* sB = (ushort*)sP;
    uint* dB = (uint*)dP;

    for (int x = 0; x < sN; ++x)
        *dB++ = A1R5G5B5toA8R8G8B8(*sB++);
}

void CColorConverter::convert_A1R5G5B5toA1R5G5B5(const void* sP, int sN, void* dP)
{
    memcpy(dP, sP, sN * 2);
}

void CColorConverter::convert_A1R5G5B5toR5G6B5(const void* sP, int sN, void* dP)
{
    ushort* sB = (ushort*)sP;
    ushort* dB = (ushort*)dP;

    for (int x = 0; x < sN; ++x)
        *dB++ = A1R5G5B5toR5G6B5(*sB++);
}

void CColorConverter::convert_A8R8G8B8toR8G8B8(const void* sP, int sN, void* dP)
{
    char* sB = (char*)sP;
    char* dB = (char*)dP;

    for (int x = 0; x < sN; ++x)
    {
        // sB[3] is alpha
        dB[0] = sB[2];
        dB[1] = sB[1];
        dB[2] = sB[0];

        sB += 4;
        dB += 3;
    }
}

void CColorConverter::convert_A8R8G8B8toB8G8R8(const void* sP, int sN, void* dP)
{
    char* sB = (char*)sP;
    char* dB = (char*)dP;

    for (int x = 0; x < sN; ++x)
    {
        // sB[3] is alpha
        dB[0] = sB[0];
        dB[1] = sB[1];
        dB[2] = sB[2];

        sB += 4;
        dB += 3;
    }
}

void CColorConverter::convert_A8R8G8B8toA8R8G8B8(const void* sP, int sN, void* dP)
{
    memcpy(dP, sP, sN * 4);
}

void CColorConverter::convert_A8R8G8B8toA1R5G5B5(const void* sP, int sN, void* dP)
{
    uint* sB = (uint*)sP;
    ushort* dB = (ushort*)dP;

    for (int x = 0; x < sN; ++x)
        *dB++ = A8R8G8B8toA1R5G5B5(*sB++);
}

void CColorConverter::convert_A8R8G8B8toR5G6B5(const void* sP, int sN, void* dP)
{
    char * sB = (char *)sP;
    ushort* dB = (ushort*)dP;

    for (int x = 0; x < sN; ++x)
    {
        int r = sB[2] >> 3;
        int g = sB[1] >> 2;
        int b = sB[0] >> 3;

        dB[0] = (r << 11) | (g << 5) | (b);

        sB += 4;
        dB += 1;
    }
}

void CColorConverter::convert_A8R8G8B8toR3G3B2(const void* sP, int sN, void* dP)
{
    char* sB = (char*)sP;
    char* dB = (char*)dP;

    for (int x = 0; x < sN; ++x)
    {
        char r = sB[2] & 0xe0;
        char g = (sB[1] & 0xe0) >> 3;
        char b = (sB[0] & 0xc0) >> 6;

        dB[0] = (r | g | b);

        sB += 4;
        dB += 1;
    }
}

void CColorConverter::convert_R8G8B8toR8G8B8(const void* sP, int sN, void* dP)
{
    memcpy(dP, sP, sN * 3);
}

void CColorConverter::convert_R8G8B8toA8R8G8B8(const void* sP, int sN, void* dP)
{
    char*  sB = (char* )sP;
    uint* dB = (uint*)dP;

    for (int x = 0; x < sN; ++x)
    {
        *dB = 0xff000000 | (sB[0]<<16) | (sB[1]<<8) | sB[2];

        sB += 3;
        ++dB;
    }
}

void CColorConverter::convert_R8G8B8toA1R5G5B5(const void* sP, int sN, void* dP)
{
    char * sB = (char *)sP;
    ushort* dB = (ushort*)dP;

    for (int x = 0; x < sN; ++x)
    {
        int r = sB[0] >> 3;
        int g = sB[1] >> 3;
        int b = sB[2] >> 3;

        dB[0] = (0x8000) | (r << 10) | (g << 5) | (b);

        sB += 3;
        dB += 1;
    }
}

void CColorConverter::convert_B8G8R8toA8R8G8B8(const void* sP, int sN, void* dP)
{
    char*  sB = (char* )sP;
    uint* dB = (uint*)dP;

    for (int x = 0; x < sN; ++x)
    {
        *dB = 0xff000000 | (sB[2]<<16) | (sB[1]<<8) | sB[0];

        sB += 3;
        ++dB;
    }
}

void CColorConverter::convert_B8G8R8A8toA8R8G8B8(const void* sP, int sN, void* dP)
{
    char* sB = (char*)sP;
    char* dB = (char*)dP;

    for (int x = 0; x < sN; ++x)
    {
        dB[0] = sB[3];
        dB[1] = sB[2];
        dB[2] = sB[1];
        dB[3] = sB[0];

        sB += 4;
        dB += 4;
    }

}

void CColorConverter::convert_R8G8B8toR5G6B5(const void* sP, int sN, void* dP)
{
    char * sB = (char *)sP;
    ushort* dB = (ushort*)dP;

    for (int x = 0; x < sN; ++x)
    {
        int r = sB[0] >> 3;
        int g = sB[1] >> 2;
        int b = sB[2] >> 3;

        dB[0] = (r << 11) | (g << 5) | (b);

        sB += 3;
        dB += 1;
    }
}

void CColorConverter::convert_R5G6B5toR5G6B5(const void* sP, int sN, void* dP)
{
    memcpy(dP, sP, sN * 2);
}

void CColorConverter::convert_R5G6B5toR8G8B8(const void* sP, int sN, void* dP)
{
    ushort* sB = (ushort*)sP;
    char * dB = (char *)dP;

    for (int x = 0; x < sN; ++x)
    {
        dB[0] = (*sB & 0xf800) >> 8;
        dB[1] = (*sB & 0x07e0) >> 3;
        dB[2] = (*sB & 0x001f) << 3;

        sB += 1;
        dB += 3;
    }
}

void CColorConverter::convert_R5G6B5toB8G8R8(const void* sP, int sN, void* dP)
{
    ushort* sB = (ushort*)sP;
    char * dB = (char *)dP;

    for (int x = 0; x < sN; ++x)
    {
        dB[2] = (*sB & 0xf800) >> 8;
        dB[1] = (*sB & 0x07e0) >> 3;
        dB[0] = (*sB & 0x001f) << 3;

        sB += 1;
        dB += 3;
    }
}

void CColorConverter::convert_R5G6B5toA8R8G8B8(const void* sP, int sN, void* dP)
{
    ushort* sB = (ushort*)sP;
    uint* dB = (uint*)dP;

    for (int x = 0; x < sN; ++x)
        *dB++ = R5G6B5toA8R8G8B8(*sB++);
}

void CColorConverter::convert_R5G6B5toA1R5G5B5(const void* sP, int sN, void* dP)
{
    ushort* sB = (ushort*)sP;
    ushort* dB = (ushort*)dP;

    for (int x = 0; x < sN; ++x)
        *dB++ = R5G6B5toA1R5G5B5(*sB++);
}


void CColorConverter::convert_viaFormat(const void* sP, ColorFormat sF, int sN,
                void* dP, ColorFormat dF)
{
    switch (sF)
    {
        case ECF_A1R5G5B5:
            switch (dF)
            {
                case ECF_A1R5G5B5:
                    convert_A1R5G5B5toA1R5G5B5(sP, sN, dP);
                break;
                case ECF_R5G6B5:
                    convert_A1R5G5B5toR5G6B5(sP, sN, dP);
                break;
                case ECF_A8R8G8B8:
                    convert_A1R5G5B5toA8R8G8B8(sP, sN, dP);
                break;
                case ECF_R8G8B8:
                    convert_A1R5G5B5toR8G8B8(sP, sN, dP);
                break;
#ifndef _DEBUG
                default:
                    break;
#endif
            }
        break;
        case ECF_R5G6B5:
            switch (dF)
            {
                case ECF_A1R5G5B5:
                    convert_R5G6B5toA1R5G5B5(sP, sN, dP);
                break;
                case ECF_R5G6B5:
                    convert_R5G6B5toR5G6B5(sP, sN, dP);
                break;
                case ECF_A8R8G8B8:
                    convert_R5G6B5toA8R8G8B8(sP, sN, dP);
                break;
                case ECF_R8G8B8:
                    convert_R5G6B5toR8G8B8(sP, sN, dP);
                break;
#ifndef _DEBUG
                default:
                    break;
#endif
            }
        break;
        case ECF_A8R8G8B8:
            switch (dF)
            {
                case ECF_A1R5G5B5:
                    convert_A8R8G8B8toA1R5G5B5(sP, sN, dP);
                break;
                case ECF_R5G6B5:
                    convert_A8R8G8B8toR5G6B5(sP, sN, dP);
                break;
                case ECF_A8R8G8B8:
                    convert_A8R8G8B8toA8R8G8B8(sP, sN, dP);
                break;
                case ECF_R8G8B8:
                    convert_A8R8G8B8toR8G8B8(sP, sN, dP);
                break;
#ifndef _DEBUG
                default:
                    break;
#endif
            }
        break;
        case ECF_R8G8B8:
            switch (dF)
            {
                case ECF_A1R5G5B5:
                    convert_R8G8B8toA1R5G5B5(sP, sN, dP);
                break;
                case ECF_R5G6B5:
                    convert_R8G8B8toR5G6B5(sP, sN, dP);
                break;
                case ECF_A8R8G8B8:
                    convert_R8G8B8toA8R8G8B8(sP, sN, dP);
                break;
                case ECF_R8G8B8:
                    convert_R8G8B8toR8G8B8(sP, sN, dP);
                break;
#ifndef _DEBUG
                default:
                    break;
#endif
            }
        break;
    }
}

}


