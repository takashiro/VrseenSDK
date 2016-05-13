#include "VImage.h"
#include "VBlit.h"
#include <math.h>
#include <cmath>
#include <cerrno>
#include <cfenv>
#include "core/VLog.h"
namespace  NervGear {

static const float BICUBIC_SHARPEN = 0.75f; // same as default PhotoShop bicubic filter

static void FilterWeights( const float s, const int filter, float weights[ 4 ] )
{
    switch ( filter )
    {
    case IMAGE_FILTER_NEAREST:
    {
                weights[ 0 ] = 1.0f;
                break;
    }
    case IMAGE_FILTER_LINEAR:
    {
                weights[ 0 ] = 1.0f - s;
                weights[ 1 ] = s;
                break;
    }
    case IMAGE_FILTER_CUBIC:
    {
                weights[ 0 ] = ( ( ( ( +0.0f - BICUBIC_SHARPEN ) * s + ( +0.0f + 2.0f * BICUBIC_SHARPEN ) ) * s + ( -BICUBIC_SHARPEN ) ) * s + ( 0.0f ) );
                weights[ 1 ] = ( ( ( ( +2.0f - BICUBIC_SHARPEN ) * s + ( -3.0f + 1.0f * BICUBIC_SHARPEN ) ) * s + ( 0.0f ) ) * s + ( 1.0f ) );
                weights[ 2 ] = ( ( ( ( -2.0f + BICUBIC_SHARPEN ) * s + ( +3.0f - 2.0f * BICUBIC_SHARPEN ) ) * s + ( BICUBIC_SHARPEN ) ) * s + ( 0.0f ) );
                weights[ 3 ] = ( ( ( ( +0.0f + BICUBIC_SHARPEN ) * s + ( +0.0f - 1.0f * BICUBIC_SHARPEN ) ) * s + ( 0.0f ) ) * s + ( 0.0f ) );
                break;
    }
    }
}
static float SRGBToLinear( float c )
{
    const float a = 0.055f;
    if ( c <= 0.04045f )
    {
        return c * ( 1.0f / 12.92f );
    }
    else
    {
        return powf( ( ( c + a ) * ( 1.0f / ( 1.0f + a ) ) ), 2.4f );
    }
}

static float LinearToSRGB( float c )
{
    const float a = 0.055f;
    if ( c <= 0.0031308f )
    {
        return c * 12.92f;
    }
    else
    {
        return ( 1.0f + a ) * powf( c, ( 1.0f / 2.4f ) ) - a;
    }
}

inline float FracFloat( const float x )
{
    return x - floorf( x );
}

inline int AbsInt( const int x )
{
    const int mask = x >> ( sizeof( int )* 8 - 1 );
    return ( x + mask ) ^ mask;
}

inline int ClampInt( const int x, const int min, const int max )
{
    return min + ( ( AbsInt( x - min ) - AbsInt( x - min - max ) + max ) >> 1 );
}

VImage::VImage(ColorFormat format, const VDimension<uint>& size)
:m_data(0), m_size(size), m_format(format), m_length(0), m_info(),DeleteMemory(true)
{
    initData();
}


//! Constructor from raw data
VImage::VImage(ColorFormat format, const VDimension<uint>& size, void* data,
            bool ownForeignMemory, bool deleteForeignMemory)
: m_data(0), m_size(size), m_format(format), m_length(0), m_info(), DeleteMemory(deleteForeignMemory)
{
    if (ownForeignMemory)
    {
        m_data = (char*)0xbadf00d;
        initData();
        m_data = (char*)data;
    }
    else
    {
        m_data = 0;
        initData();
        memcpy(m_data, data, m_size.Height * m_pitch);
    }
}

//Construct from another raw data
VImage::VImage(ColorFormat format, const VDimension<uint>& size, void* data, uint length, VMap<VString, VString> &info)
    :m_data(0), m_size(size), m_format(format), m_length(length), m_info(info), DeleteMemory(true)
{
    m_data = (char *)data;

}


//! assumes format and size has been set and creates the rest
void VImage::initData()
{
#ifdef _DEBUG
    setDebugName("VImage");
#endif
    m_bytesPerPixel = getBitsPerPixelFromFormat(m_format) / 8;

    // Pitch should be aligned...
    m_pitch = m_bytesPerPixel * m_size.Width;

    if (!m_data)
    {
        DeleteMemory=true;
        m_data = new char[m_size.Height * m_pitch];
    }
}


//! destructor
VImage::~VImage()
{
    if ( DeleteMemory )
        delete [] m_data;
}


//! Returns width and height of image data.
const VDimension<uint>& VImage::getDimension() const
{
    return m_size;
}


//! Returns bits per pixel.
uint VImage::getBitsPerPixel() const
{
    return getBitsPerPixelFromFormat(m_format);
}


//! Returns bytes per pixel
uint VImage::getBytesPerPixel() const
{
    return m_bytesPerPixel;
}


//! Returns image data size in bytes
uint VImage::getImageDataSizeInBytes() const
{
    return m_pitch * m_size.Height;
}


//! Returns image data size in pixels
uint VImage::getImageDataSizeInPixels() const
{
    return m_size.Width * m_size.Height;
}


//! returns mask for red value of a pixel
uint VImage::getRedMask() const
{
    switch(m_format)
    {
    case ECF_A1R5G5B5:
        return 0x1F<<10;
    case ECF_R5G6B5:
        return 0x1F<<11;
    case ECF_R8G8B8:
        return 0x00FF0000;
    case ECF_A8R8G8B8:
        return 0x00FF0000;
    default:
        return 0x0;
    }
}


//! returns mask for green value of a pixel
uint VImage::getGreenMask() const
{
    switch(m_format)
    {
    case ECF_A1R5G5B5:
        return 0x1F<<5;
    case ECF_R5G6B5:
        return 0x3F<<5;
    case ECF_R8G8B8:
        return 0x0000FF00;
    case ECF_A8R8G8B8:
        return 0x0000FF00;
    default:
        return 0x0;
    }
}


//! returns mask for blue value of a pixel
uint VImage::getBlueMask() const
{
    switch(m_format)
    {
    case ECF_A1R5G5B5:
        return 0x1F;
    case ECF_R5G6B5:
        return 0x1F;
    case ECF_R8G8B8:
        return 0x000000FF;
    case ECF_A8R8G8B8:
        return 0x000000FF;
    default:
        return 0x0;
    }
}


//! returns mask for alpha value of a pixel
uint VImage::getAlphaMask() const
{
    switch(m_format)
    {
    case ECF_A1R5G5B5:
        return 0x1<<15;
    case ECF_R5G6B5:
        return 0x0;
    case ECF_R8G8B8:
        return 0x0;
    case ECF_A8R8G8B8:
        return 0xFF000000;
    default:
        return 0x0;
    }
}

uint VImage::getLength() const
{
    return m_length;
}

//! sets a pixel
void VImage::setPixel(uint x, uint y, const VImageColor &color, bool blend)
{
    if (x >= m_size.Width || y >= m_size.Height)
        return;

    switch(m_format)
    {
        case ECF_A1R5G5B5:
        {
            ushort * dest = (ushort*) (m_data + ( y * m_pitch ) + ( x << 1 ));
            *dest = A8R8G8B8toA1R5G5B5( color.color );
        } break;

        case ECF_R5G6B5:
        {
            ushort * dest = (ushort*) (m_data + ( y * m_pitch ) + ( x << 1 ));
            *dest = A8R8G8B8toR5G6B5( color.color );
        } break;

        case ECF_R8G8B8:
        {
            char* dest = m_data + ( y * m_pitch ) + ( x * 3 );
            dest[0] = (char)color.getRed();
            dest[1] = (char)color.getGreen();
            dest[2] = (char)color.getBlue();
        } break;

        case ECF_A8R8G8B8:
        {
            uint * dest = (uint*) (m_data + ( y * m_pitch ) + ( x << 2 ));
            *dest = blend ? PixelBlend32 ( *dest, color.color ) : color.color;
        } break;
#ifndef _DEBUG
        default:
            break;
#endif
    }
}


//! returns a pixel
VImageColor VImage::getPixel(uint x, uint y) const
{
    if (x >= m_size.Width || y >= m_size.Height)
        return VImageColor(0);

    switch(m_format)
    {
    case ECF_A1R5G5B5:
        return A1R5G5B5toA8R8G8B8(((ushort*)m_data)[y*m_size.Width + x]);
    case ECF_R5G6B5:
        return R5G6B5toA8R8G8B8(((ushort*)m_data)[y*m_size.Width + x]);
    case ECF_A8R8G8B8:
        return ((uint*)m_data)[y*m_size.Width + x];
    case ECF_R8G8B8:
        {
            char* p = m_data+(y*3)*m_size.Width + (x*3);
            return VImageColor(255,p[0],p[1],p[2]);
        }
#ifndef _DEBUG
    default:
        break;
#endif
    }

    return VImageColor(0);
}


//! returns the color format
ColorFormat VImage::getColorFormat() const
{
    return m_format;
}


//! copies this surface into another at given position
void VImage::copyTo(VImage* target, const V2Vect<int>& pos)
{
    Blit(BLITTER_TEXTURE, target, 0, &pos, this, 0, 0);
}


//! copies this surface partially into another at given position
void VImage::copyTo(VImage* target, const V2Vect<int>& pos, const VRectangle<int>& sourceRect, const VRectangle<int>* clipRect)
{
    Blit(BLITTER_TEXTURE, target, clipRect, &pos, this, &sourceRect, 0);
}


//! copies this surface into another, scaling it to the target image size
// note: this is very very slow.
void VImage::copyToScaling(void* target, uint width, uint height, ColorFormat format, uint pitch)
{
    if (!target || !width || !height)
        return;

    const uint bpp=getBitsPerPixelFromFormat(format)/8;
    if (0==pitch)
        pitch = width*bpp;

    if (m_format==format && m_size.Width==width && m_size.Height==height)
    {
        if (pitch==m_pitch)
        {
            memcpy(target, m_data, height*pitch);
            return;
        }
        else
        {
            char* tgtpos = (char*) target;
            char* srcpos = m_data;
            const uint bwidth = width*bpp;
            const uint rest = pitch-bwidth;
            for (uint y=0; y<height; ++y)
            {
                // copy scanline
                memcpy(tgtpos, srcpos, bwidth);
                // clear pitch
                memset(tgtpos+bwidth, 0, rest);
                tgtpos += pitch;
                srcpos += m_pitch;
            }
            return;
        }
    }

    const float sourceXStep = (float)m_size.Width / (float)width;
    const float sourceYStep = (float)m_size.Height / (float)height;
    int yval=0, syval=0;
    float sy = 0.0f;
    for (uint y=0; y<height; ++y)
    {
        float sx = 0.0f;
        for (uint x=0; x<width; ++x)
        {
            CColorConverter::convert_viaFormat(m_data+ syval + ((int)sx)*m_bytesPerPixel, m_format, 1, ((char*)target)+ yval + (x*bpp), format);
            sx+=sourceXStep;
        }
        sy+=sourceYStep;
        syval=((int)sy)*m_pitch;
        yval+=pitch;
    }
}


//! copies this surface into another, scaling it to the target image size
// note: this is very very slow.
void VImage::copyToScaling(VImage* target)
{
    if (!target)
        return;

    const VDimension<uint>& targetSize = target->getDimension();

    if (targetSize==m_size)
    {
        copyTo(target);
        return;
    }

    copyToScaling(target->lock(), targetSize.Width, targetSize.Height, target->getColorFormat());
    target->unlock();
}


//! copies this surface into another, scaling it to fit it.
void VImage::copyToScalingBoxFilter(VImage* target, int bias, bool blend)
{
    const VDimension<uint> destSize = target->getDimension();

    const float sourceXStep = (float) m_size.Width / (float) destSize.Width;
    const float sourceYStep = (float) m_size.Height / (float) destSize.Height;

    target->lock();

    int fx = (int)ceilf( sourceXStep );
    int fy = (int)ceilf( sourceYStep );
    float sx;
    float sy;

    sy = 0.f;
    for ( uint y = 0; y != destSize.Height; ++y )
    {
        sx = 0.f;
        for ( uint x = 0; x != destSize.Width; ++x )
        {
            target->setPixel( x, y,
                getPixelBox( (int)floorf(sx), (int)floorf(sy), fx, fy, bias ), blend );
            sx += sourceXStep;
        }
        sy += sourceYStep;
    }

    target->unlock();
}


VMap<VString, VString> VImage::getInfo()
{
    return m_info;
}

//! fills the surface with given color
void VImage::fill(const VImageColor &color)
{
    uint c;

    switch ( m_format )
    {
        case ECF_A1R5G5B5:
            c = color.toA1R5G5B5();
            c |= c << 16;
            break;
        case ECF_R5G6B5:
            c = A8R8G8B8toR5G6B5( color.color );
            c |= c << 16;
            break;
        case ECF_A8R8G8B8:
            c = color.color;
            break;
        case ECF_R8G8B8:
        {
            char rgb[3];
            CColorConverter::convert_A8R8G8B8toR8G8B8(&color, 1, rgb);
            const uint size = getImageDataSizeInBytes();
            for (uint i=0; i<size; i+=3)
            {
                memcpy(m_data+i, rgb, 3);
            }
            return;
        }
        break;
        default:
        // TODO: Handle other formats
            return;
    }
    memset32( m_data, c, getImageDataSizeInBytes() );
}


//! get a filtered pixel
inline VImageColor VImage::getPixelBox( int x, int y, int fx, int fy, int bias ) const
{
    VImageColor c;
    int a = 0, r = 0, g = 0, b = 0;

    for ( int dx = 0; dx != fx; ++dx )
    {
        for ( int dy = 0; dy != fy; ++dy )
        {
            c = getPixel(	std::min ( x + dx, (int)m_size.Width - 1 ) ,
                            std::min ( y + dy, (int)m_size.Height - 1 )
                        );

            a += c.getAlpha();
            r += c.getRed();
            g += c.getGreen();
            b += c.getBlue();
        }

    }

    int sdiv = std::log10(fx * fy)/std::log10(2);

    a = std::min( std::max(( a >> sdiv ) + bias, 0), 255 );
    r = std::min( std::max(( r >> sdiv ) + bias, 0), 255 );
    g = std::min( std::max(( g >> sdiv ) + bias, 0), 255 );
    b = std::min( std::max(( b >> sdiv ) + bias, 0), 255 );

    c.set( a, r, g, b );
    return c;
}

unsigned char * VImage::QuarterSize( const unsigned char * src, const int width, const int height, const bool srgb )
{
    float table[256];
    if ( srgb )
    {
        for ( int i = 0; i < 256; i++ )
        {
            table[ i ] = SRGBToLinear( i * ( 1.0f / 255.0f ) );
        }
    }

    const int newWidth = std::max( 1, width >> 1 );
    const int newHeight = std::max( 1, height >> 1 );
    unsigned char * out = (unsigned char *)malloc( newWidth * newHeight * 4 );
    unsigned char * out_p = out;
    for ( int y = 0; y < newHeight; y++ )
    {
        const unsigned char * in_p = src + y * 2 * width * 4;
        for ( int x = 0; x < newWidth; x++ )
        {
            for ( int i = 0; i < 4; i++ )
            {
                if ( srgb )
                {
                    const float linear = ( table[ in_p[ i ] ] +
                        table[ in_p[ 4 + i ] ] +
                        table[ in_p[ width * 4 + i ] ] +
                        table[ in_p[ width * 4 + 4 + i ] ] ) * 0.25f;
                    const float gamma = LinearToSRGB( linear );
                    out_p[ i ] = ( unsigned char )ClampInt( ( int )( gamma * 255.0f + 0.5f ), 0, 255 );
                }
                else
                {
                    out_p[ i ] = ( in_p[ i ] +
                        in_p[ 4 + i ] +
                        in_p[ width * 4 + i ] +
                        in_p[ width * 4 + 4 + i ] ) >> 2;
                }
            }
            out_p += 4;
            in_p += 8;
        }
    }
    return out;
}

#pragma pack(1)
struct OVR_PVR_HEADER
{
    vuint32  Version;
    vuint32  Flags;
    vuint64  PixelFormat;
    vuint32  ColorSpace;
    vuint32  ChannelType;
    vuint32  Height;
    vuint32  Width;
    vuint32  Depth;
    vuint32  NumSurfaces;
    vuint32  NumFaces;
    vuint32  MipMapCount;
    vuint32  MetaDataSize;
};
#pragma pack()

void VImage::WritePvrTexture( const char * fileName, const unsigned char * texture, int width, int height )
{
    FILE *f = fopen( fileName, "wb" );
    if ( !f )
    {
        vWarn("Failed to write" << fileName);
        return;
    }

    OVR_PVR_HEADER header = {};
    header.Version = 0x03525650;                // 'PVR' + 0x3
    header.PixelFormat = 578721384203708274llu; // 8888 RGBA
    header.Width = width;
    header.Height = height;
    header.Depth = 1;
    header.NumSurfaces = 1;
    header.NumFaces = 1;
    header.MipMapCount = 1;

    fwrite( &header, 1, sizeof( header ), f );
    fwrite( texture, 1, width * height * 4, f );

    fclose( f );
}

unsigned char * VImage::ScaleRGBA( const unsigned char * src, const int width, const int height, const int newWidth, const int newHeight, const ImageFilter filter )
{
    int footprintMin = 0;
    int footprintMax = 0;
    int offsetX = 0;
    int offsetY = 0;
    switch ( filter )
    {
    case IMAGE_FILTER_NEAREST:
    {
                footprintMin = 0;
                footprintMax = 0;
                offsetX = width;
                offsetY = height;
                break;
    }
    case IMAGE_FILTER_LINEAR:
    {
                footprintMin = 0;
                footprintMax = 1;
                offsetX = width - newWidth;
                offsetY = height - newHeight;
                break;
    }
    case IMAGE_FILTER_CUBIC:
    {
                footprintMin = -1;
                footprintMax = 2;
                offsetX = width - newWidth;
                offsetY = height - newHeight;
                break;
    }
    }

    unsigned char * scaled = ( unsigned char * )malloc( newWidth * newHeight * 4 * sizeof( unsigned char ) );

    float * srcLinear = ( float * )malloc( width * height * 4 * sizeof( float ) );
    float * scaledLinear = ( float * )malloc( newWidth * newHeight * 4 * sizeof( float ) );

    float table[ 256 ];
    for ( int i = 0; i < 256; i++ )
    {
        table[ i ] = SRGBToLinear( i * ( 1.0f / 255.0f ) );
    }

    for ( int y = 0; y < height; y++ )
    {
        for ( int x = 0; x < width; x++ )
        {
            for ( int c = 0; c < 4; c++ )
            {
                srcLinear[ ( y * width + x ) * 4 + c ] = table[ src[ ( y * width + x ) * 4 + c ] ];
            }
        }
    }

    for ( int y = 0; y < newHeight; y++ )
    {
        const int srcY = ( y * height * 2 + offsetY ) / ( newHeight * 2 );
        const float fracY = FracFloat( ( ( float )y * height * 2.0f + offsetY ) / ( newHeight * 2.0f ) );

        float weightsY[ 4 ];
        FilterWeights( fracY, filter, weightsY );

        for ( int x = 0; x < newWidth; x++ )
        {
            const int srcX = ( x * width * 2 + offsetX ) / ( newWidth * 2 );
            const float fracX = FracFloat( ( ( float )x * width * 2.0f + offsetX ) / ( newWidth * 2.0f ) );

            float weightsX[ 4 ];
            FilterWeights( fracX, filter, weightsX );

            float fR = 0.0f;
            float fG = 0.0f;
            float fB = 0.0f;
            float fA = 0.0f;

            for ( int fpY = footprintMin; fpY <= footprintMax; fpY++ )
            {
                const float wY = weightsY[ fpY - footprintMin ];

                for ( int fpX = footprintMin; fpX <= footprintMax; fpX++ )
                {
                    const float wX = weightsX[ fpX - footprintMin ];
                    const float wXY = wX * wY;

                    const int cx = ClampInt( srcX + fpX, 0, width - 1 );
                    const int cy = ClampInt( srcY + fpY, 0, height - 1 );
                    fR += srcLinear[ ( cy * width + cx ) * 4 + 0 ] * wXY;
                    fG += srcLinear[ ( cy * width + cx ) * 4 + 1 ] * wXY;
                    fB += srcLinear[ ( cy * width + cx ) * 4 + 2 ] * wXY;
                    fA += srcLinear[ ( cy * width + cx ) * 4 + 3 ] * wXY;
                }
            }

            scaledLinear[ ( y * newWidth + x ) * 4 + 0 ] = fR;
            scaledLinear[ ( y * newWidth + x ) * 4 + 1 ] = fG;
            scaledLinear[ ( y * newWidth + x ) * 4 + 2 ] = fB;
            scaledLinear[ ( y * newWidth + x ) * 4 + 3 ] = fA;
        }
    }

    for ( int y = 0; y < newHeight; y++ )
    {
        for ( int x = 0; x < newWidth; x++ )
        {
            for ( int c = 0; c < 4; c++ )
            {
                const float gamma = LinearToSRGB( scaledLinear[ ( y * newWidth + x ) * 4 + c ] );
                scaled[ ( y * newWidth + x ) * 4 + c ] = ( unsigned char )ClampInt( ( int )( gamma * 255.0f + 0.5f ), 0, 255 );
            }
        }
    }

    free( scaledLinear );
    free( srcLinear );

    return scaled;
}
}
