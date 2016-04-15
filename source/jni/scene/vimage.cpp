#include "VImage.h"
#include "VBlit.h"
#include <math.h>
#include <cmath>
#include <cerrno>
#include <cfenv>

namespace  NervGear {

CImage::CImage(ColorFormat format, const VDimension<uint>& size)
:m_data(0), m_size(size), m_format(format), m_length(0), m_info(),DeleteMemory(true)
{
    initData();
}


//! Constructor from raw data
CImage::CImage(ColorFormat format, const VDimension<uint>& size, void* data,
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
CImage::CImage(ColorFormat format, const VDimension<uint>& size, void* data, uint length, VMap<VString, VString> &info)
    :m_data(0), m_size(size), m_format(format), m_length(length), m_info(info), DeleteMemory(true)
{
    m_data = (char *)data;

}


//! assumes format and size has been set and creates the rest
void CImage::initData()
{
#ifdef _DEBUG
    setDebugName("CImage");
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
CImage::~CImage()
{
    if ( DeleteMemory )
        delete [] m_data;
}


//! Returns width and height of image data.
const VDimension<uint>& CImage::getDimension() const
{
    return m_size;
}


//! Returns bits per pixel.
uint CImage::getBitsPerPixel() const
{
    return getBitsPerPixelFromFormat(m_format);
}


//! Returns bytes per pixel
uint CImage::getBytesPerPixel() const
{
    return m_bytesPerPixel;
}


//! Returns image data size in bytes
uint CImage::getImageDataSizeInBytes() const
{
    return m_pitch * m_size.Height;
}


//! Returns image data size in pixels
uint CImage::getImageDataSizeInPixels() const
{
    return m_size.Width * m_size.Height;
}


//! returns mask for red value of a pixel
uint CImage::getRedMask() const
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
uint CImage::getGreenMask() const
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
uint CImage::getBlueMask() const
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
uint CImage::getAlphaMask() const
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

uint CImage::getLength() const
{
    return m_length;
}

//! sets a pixel
void CImage::setPixel(uint x, uint y, const VImageColor &color, bool blend)
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
VImageColor CImage::getPixel(uint x, uint y) const
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
ColorFormat CImage::getColorFormat() const
{
    return m_format;
}


//! copies this surface into another at given position
void CImage::copyTo(VImage* target, const V2Vect<int>& pos)
{
    Blit(BLITTER_TEXTURE, target, 0, &pos, this, 0, 0);
}


//! copies this surface partially into another at given position
void CImage::copyTo(VImage* target, const V2Vect<int>& pos, const VRectangle<int>& sourceRect, const VRectangle<int>* clipRect)
{
    Blit(BLITTER_TEXTURE, target, clipRect, &pos, this, &sourceRect, 0);
}


//! copies this surface into another, scaling it to the target image size
// note: this is very very slow.
void CImage::copyToScaling(void* target, uint width, uint height, ColorFormat format, uint pitch)
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
void CImage::copyToScaling(VImage* target)
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
void CImage::copyToScalingBoxFilter(VImage* target, int bias, bool blend)
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


VMap<VString, VString> CImage::getInfo()
{
    return m_info;
}

//! fills the surface with given color
void CImage::fill(const VImageColor &color)
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
inline VImageColor CImage::getPixelBox( int x, int y, int fx, int fy, int bias ) const
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


}
