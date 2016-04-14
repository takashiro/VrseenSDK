#ifndef VBLIT_H
#define VBLIT_H
#include "VTexture.h"
#include "VImage.h"
#include "VBasicmath.h"
#include "VImageColor.h"

namespace NervGear {

// some 2D Defines
struct AbsRectangle
{
    int x0;
    int y0;
    int x1;
    int y1;
};

struct SBlitJob
    {
        AbsRectangle Dest;
        AbsRectangle Source;

        uint argb;

        void * src;
        void * dst;

        int width;
        int height;

        uint srcPitch;
        uint dstPitch;

        uint srcPixelMul;
        uint dstPixelMul;

        bool stretch;
        float x_stretch;
        float y_stretch;

        SBlitJob() : stretch(false) {}
    };
// Blitter Operation
enum eBlitter
{
    BLITTER_INVALID = 0,
    BLITTER_COLOR,
    BLITTER_COLOR_ALPHA,
    BLITTER_TEXTURE,
    BLITTER_TEXTURE_ALPHA_BLEND,
    BLITTER_TEXTURE_ALPHA_COLOR_BLEND
};

typedef void (*tExecuteBlit) ( const SBlitJob * job );


/*!
*/
struct blitterTable
{
    eBlitter operation;
    int destFormat;
    int sourceFormat;
    tExecuteBlit func;
};


static tExecuteBlit getBlitter2( eBlitter operation,const VImage * dest,const VImage * source )
{
    ColorFormat sourceFormat = (ColorFormat) ( source ? source->getColorFormat() : -1 );
    ColorFormat destFormat = (ColorFormat) ( dest ? dest->getColorFormat() : -1 );

    const blitterTable * b = blitTable;

    while ( b->operation != BLITTER_INVALID )
    {
        if ( b->operation == operation )
        {
            if (( b->destFormat == -1 || b->destFormat == destFormat ) &&
                ( b->sourceFormat == -1 || b->sourceFormat == sourceFormat ) )
                    return b->func;
            else
            if ( b->destFormat == -2 && ( sourceFormat == destFormat ) )
                    return b->func;
        }
        b += 1;
    }
    return 0;
}
static const blitterTable blitTable[] =
{
    { BLITTER_TEXTURE, -2, -2, executeBlit_TextureCopy_x_to_x },
    { BLITTER_TEXTURE, ECF_A1R5G5B5, ECF_A8R8G8B8, executeBlit_TextureCopy_32_to_16 },
    { BLITTER_TEXTURE, ECF_A1R5G5B5, ECF_R8G8B8, executeBlit_TextureCopy_24_to_16 },
    { BLITTER_TEXTURE, ECF_A8R8G8B8, ECF_A1R5G5B5, executeBlit_TextureCopy_16_to_32 },
    { BLITTER_TEXTURE, ECF_A8R8G8B8, ECF_R8G8B8, executeBlit_TextureCopy_24_to_32 },
    { BLITTER_TEXTURE, ECF_R8G8B8, ECF_A1R5G5B5, executeBlit_TextureCopy_16_to_24 },
    { BLITTER_TEXTURE, ECF_R8G8B8, ECF_A8R8G8B8, executeBlit_TextureCopy_32_to_24 },
    { BLITTER_TEXTURE_ALPHA_BLEND, ECF_A1R5G5B5, ECF_A1R5G5B5, executeBlit_TextureBlend_16_to_16 },
    { BLITTER_TEXTURE_ALPHA_BLEND, ECF_A8R8G8B8, ECF_A8R8G8B8, executeBlit_TextureBlend_32_to_32 },
    { BLITTER_TEXTURE_ALPHA_COLOR_BLEND, ECF_A1R5G5B5, ECF_A1R5G5B5, executeBlit_TextureBlendColor_16_to_16 },
    { BLITTER_TEXTURE_ALPHA_COLOR_BLEND, ECF_A8R8G8B8, ECF_A8R8G8B8, executeBlit_TextureBlendColor_32_to_32 },
    { BLITTER_COLOR, ECF_A1R5G5B5, -1, executeBlit_Color_16_to_16 },
    { BLITTER_COLOR, ECF_A8R8G8B8, -1, executeBlit_Color_32_to_32 },
    { BLITTER_COLOR_ALPHA, ECF_A1R5G5B5, -1, executeBlit_ColorAlpha_16_to_16 },
    { BLITTER_COLOR_ALPHA, ECF_A8R8G8B8, -1, executeBlit_ColorAlpha_32_to_32 },
    { BLITTER_INVALID, -1, -1, 0 }
};




static int Blit(eBlitter operation,
        VImage * dest,
        const VRect<int> *destClipping,
        const V2Vect<int> *destPos,
        VImage * const source,
        const VRect<int> *sourceClipping,
        uint argb)
{
    tExecuteBlit blitter = getBlitter2( operation, dest, source );
    if ( 0 == blitter )
    {
        return 0;
    }

    // Clipping
    AbsRectangle sourceClip;
    AbsRectangle destClip;
    AbsRectangle v;

    SBlitJob job;

    setClip ( sourceClip, sourceClipping, source, 1 );
    setClip ( destClip, destClipping, dest, 0 );

    v.x0 = destPos ? destPos->x : 0;
    v.y0 = destPos ? destPos->y : 0;
    v.x1 = v.x0 + ( sourceClip.x1 - sourceClip.x0 );
    v.y1 = v.y0 + ( sourceClip.y1 - sourceClip.y0 );

    if ( !intersect( job.Dest, destClip, v ) )
        return 0;

    job.width = job.Dest.x1 - job.Dest.x0;
    job.height = job.Dest.y1 - job.Dest.y0;

    job.Source.x0 = sourceClip.x0 + ( job.Dest.x0 - v.x0 );
    job.Source.x1 = job.Source.x0 + job.width;
    job.Source.y0 = sourceClip.y0 + ( job.Dest.y0 - v.y0 );
    job.Source.y1 = job.Source.y0 + job.height;

    job.argb = argb;

    if ( source )
    {
        job.srcPitch = source->getPitch();
        job.srcPixelMul = source->getBytesPerPixel();
        job.src = (void*) ( (char*) source->lock() + ( job.Source.y0 * job.srcPitch ) + ( job.Source.x0 * job.srcPixelMul ) );
    }
    else
    {
        // use srcPitch for color operation on dest
        job.srcPitch = job.width * dest->getBytesPerPixel();
    }

    job.dstPitch = dest->getPitch();
    job.dstPixelMul = dest->getBytesPerPixel();
    job.dst = (void*) ( (char*) dest->lock() + ( job.Dest.y0 * job.dstPitch ) + ( job.Dest.x0 * job.dstPixelMul ) );

    blitter( &job );

    if ( source )
        source->unlock();

    if ( dest )
        dest->unlock();

    return 1;
}

static void executeBlit_TextureCopy_x_to_x( const SBlitJob * job )
{
    const uint w = job->width;
    const uint h = job->height;
    if (job->stretch)
    {
        const uint *src = static_cast<const uint*>(job->src);
        uint *dst = static_cast<uint*>(job->dst);
        const float wscale = 1.f/job->x_stretch;
        const float hscale = 1.f/job->y_stretch;

        for ( uint dy = 0; dy < h; ++dy )
        {
            const uint src_y = (uint)(dy*hscale);
            src = (uint*) ( (char*) (job->src) + job->srcPitch*src_y );

            for ( uint dx = 0; dx < w; ++dx )
            {
                const uint src_x = (uint)(dx*wscale);
                dst[dx] = src[src_x];
            }
            dst = (uint*) ( (char*) (dst) + job->dstPitch );
        }
    }
    else
    {
        const uint widthPitch = job->width * job->dstPixelMul;
        const void *src = (void*) job->src;
        void *dst = (void*) job->dst;

        for ( uint dy = 0; dy != h; ++dy )
        {
            memcpy( dst, src, widthPitch );

            src = (void*) ( (char*) (src) + job->srcPitch );
            dst = (void*) ( (char*) (dst) + job->dstPitch );
        }
    }
}

/*!
*/
static void executeBlit_TextureCopy_32_to_16( const SBlitJob * job )
{
    const uint w = job->width;
    const uint h = job->height;
    const uint *src = static_cast<const uint*>(job->src);
    ushort *dst = static_cast<ushort*>(job->dst);

    if (job->stretch)
    {
        const float wscale = 1.f/job->x_stretch;
        const float hscale = 1.f/job->y_stretch;

        for ( uint dy = 0; dy < h; ++dy )
        {
            const uint src_y = (uint)(dy*hscale);
            src = (uint*) ( (char*) (job->src) + job->srcPitch*src_y );

            for ( uint dx = 0; dx < w; ++dx )
            {
                const uint src_x = (uint)(dx*wscale);
                //16 bit Blitter depends on pre-multiplied color
                const uint s = PixelLerp32( src[src_x] | 0xFF000000, extractAlpha( src[src_x] ) );
                dst[dx] = A8R8G8B8toA1R5G5B5( s );
            }
            dst = (ushort*) ( (char*) (dst) + job->dstPitch );
        }
    }
    else
    {
        for ( uint dy = 0; dy != h; ++dy )
        {
            for ( uint dx = 0; dx != w; ++dx )
            {
                //16 bit Blitter depends on pre-multiplied color
                const uint s = PixelLerp32( src[dx] | 0xFF000000, extractAlpha( src[dx] ) );
                dst[dx] = A8R8G8B8toA1R5G5B5( s );
            }

            src = (uint*) ( (char*) (src) + job->srcPitch );
            dst = (ushort*) ( (char*) (dst) + job->dstPitch );
        }
    }
}

/*!
*/
static void executeBlit_TextureCopy_24_to_16( const SBlitJob * job )
{
    const uint w = job->width;
    const uint h = job->height;
    const char *src = static_cast<const char*>(job->src);
    ushort *dst = static_cast<ushort*>(job->dst);

    if (job->stretch)
    {
        const float wscale = 3.f/job->x_stretch;
        const float hscale = 1.f/job->y_stretch;

        for ( uint dy = 0; dy < h; ++dy )
        {
            const uint src_y = (uint)(dy*hscale);
            src = (char*)(job->src) + job->srcPitch*src_y;

            for ( uint dx = 0; dx < w; ++dx )
            {
                const char* src_x = src+(uint)(dx*wscale);
                dst[dx] = RGBA16(src_x[0], src_x[1], src_x[2]);
            }
            dst = (ushort*) ( (char*) (dst) + job->dstPitch );
        }
    }
    else
    {
        for ( uint dy = 0; dy != h; ++dy )
        {
            const char* s = src;
            for ( uint dx = 0; dx != w; ++dx )
            {
                dst[dx] = RGBA16(s[0], s[1], s[2]);
                s += 3;
            }

            src = src+job->srcPitch;
            dst = (ushort*) ( (char*) (dst) + job->dstPitch );
        }
    }
}


/*!
*/
static void executeBlit_TextureCopy_16_to_32( const SBlitJob * job )
{
    const uint w = job->width;
    const uint h = job->height;
    const ushort *src = static_cast<const ushort*>(job->src);
    uint *dst = static_cast<uint*>(job->dst);

    if (job->stretch)
    {
        const float wscale = 1.f/job->x_stretch;
        const float hscale = 1.f/job->y_stretch;

        for ( uint dy = 0; dy < h; ++dy )
        {
            const uint src_y = (uint)(dy*hscale);
            src = (ushort*) ( (char*) (job->src) + job->srcPitch*src_y );

            for ( uint dx = 0; dx < w; ++dx )
            {
                const uint src_x = (uint)(dx*wscale);
                dst[dx] = A1R5G5B5toA8R8G8B8(src[src_x]);
            }
            dst = (uint*) ( (char*) (dst) + job->dstPitch );
        }
    }
    else
    {
        for ( uint dy = 0; dy != h; ++dy )
        {
            for ( uint dx = 0; dx != w; ++dx )
            {
                dst[dx] = A1R5G5B5toA8R8G8B8( src[dx] );
            }

            src = (ushort*) ( (char*) (src) + job->srcPitch );
            dst = (uint*) ( (char*) (dst) + job->dstPitch );
        }
    }
}

static void executeBlit_TextureCopy_16_to_24( const SBlitJob * job )
{
    const uint w = job->width;
    const uint h = job->height;
    const ushort *src = static_cast<const ushort*>(job->src);
    char *dst = static_cast<char*>(job->dst);

    if (job->stretch)
    {
        const float wscale = 1.f/job->x_stretch;
        const float hscale = 1.f/job->y_stretch;

        for ( uint dy = 0; dy < h; ++dy )
        {
            const uint src_y = (uint)(dy*hscale);
            src = (ushort*) ( (char*) (job->src) + job->srcPitch*src_y );

            for ( uint dx = 0; dx < w; ++dx )
            {
                const uint src_x = (uint)(dx*wscale);
                uint color = A1R5G5B5toA8R8G8B8(src[src_x]);
                char * writeTo = &dst[dx * 3];
                *writeTo++ = (color >> 16)& 0xFF;
                *writeTo++ = (color >> 8) & 0xFF;
                *writeTo++ = color & 0xFF;
            }
            dst += job->dstPitch;
        }
    }
    else
    {
        for ( uint dy = 0; dy != h; ++dy )
        {
            for ( uint dx = 0; dx != w; ++dx )
            {
                uint color = A1R5G5B5toA8R8G8B8(src[dx]);
                char * writeTo = &dst[dx * 3];
                *writeTo++ = (color >> 16)& 0xFF;
                *writeTo++ = (color >> 8) & 0xFF;
                *writeTo++ = color & 0xFF;
            }

            src = (ushort*) ( (char*) (src) + job->srcPitch );
            dst += job->dstPitch;
        }
    }
}

/*!
*/
static void executeBlit_TextureCopy_24_to_32( const SBlitJob * job )
{
    const uint w = job->width;
    const uint h = job->height;
    const char *src = static_cast<const char*>(job->src);
    uint *dst = static_cast<uint*>(job->dst);

    if (job->stretch)
    {
        const float wscale = 3.f/job->x_stretch;
        const float hscale = 1.f/job->y_stretch;

        for ( uint dy = 0; dy < h; ++dy )
        {
            const uint src_y = (uint)(dy*hscale);
            src = (const char*)job->src+(job->srcPitch*src_y);

            for ( uint dx = 0; dx < w; ++dx )
            {
                const char* s = src+(uint)(dx*wscale);
                dst[dx] = 0xFF000000 | s[0] << 16 | s[1] << 8 | s[2];
            }
            dst = (uint*) ( (char*) (dst) + job->dstPitch );
        }
    }
    else
    {
        for ( int dy = 0; dy != job->height; ++dy )
        {
            const char* s = src;

            for ( int dx = 0; dx != job->width; ++dx )
            {
                dst[dx] = 0xFF000000 | s[0] << 16 | s[1] << 8 | s[2];
                s += 3;
            }

            src = src + job->srcPitch;
            dst = (uint*) ( (char*) (dst) + job->dstPitch );
        }
    }
}

static void executeBlit_TextureCopy_32_to_24( const SBlitJob * job )
{
    const uint w = job->width;
    const uint h = job->height;
    const uint *src = static_cast<const uint*>(job->src);
    char *dst = static_cast<char*>(job->dst);

    if (job->stretch)
    {
        const float wscale = 1.f/job->x_stretch;
        const float hscale = 1.f/job->y_stretch;

        for ( uint dy = 0; dy < h; ++dy )
        {
            const uint src_y = (uint)(dy*hscale);
            src = (uint*) ( (char*) (job->src) + job->srcPitch*src_y);

            for ( uint dx = 0; dx < w; ++dx )
            {
                const uint src_x = src[(uint)(dx*wscale)];
                char * writeTo = &dst[dx * 3];
                *writeTo++ = (src_x >> 16)& 0xFF;
                *writeTo++ = (src_x >> 8) & 0xFF;
                *writeTo++ = src_x & 0xFF;
            }
            dst += job->dstPitch;
        }
    }
    else
    {
        for ( uint dy = 0; dy != h; ++dy )
        {
            for ( uint dx = 0; dx != w; ++dx )
            {
                char * writeTo = &dst[dx * 3];
                *writeTo++ = (src[dx] >> 16)& 0xFF;
                *writeTo++ = (src[dx] >> 8) & 0xFF;
                *writeTo++ = src[dx] & 0xFF;
            }

            src = (uint*) ( (char*) (src) + job->srcPitch );
            dst += job->dstPitch;
        }
    }
}

/*!
*/
static void executeBlit_TextureBlend_16_to_16( const SBlitJob * job )
{
    const uint w = job->width;
    const uint h = job->height;
    const uint rdx = w>>1;

    const uint *src = (uint*) job->src;
    uint *dst = (uint*) job->dst;

    if (job->stretch)
    {
        const float wscale = 1.f/job->x_stretch;
        const float hscale = 1.f/job->y_stretch;
        const uint off = if_c_a_else_b(w&1, (uint)((w-1)*wscale), 0);
        for ( uint dy = 0; dy < h; ++dy )
        {
            const uint src_y = (uint)(dy*hscale);
            src = (uint*) ( (char*) (job->src) + job->srcPitch*src_y );

            for ( uint dx = 0; dx < rdx; ++dx )
            {
                const uint src_x = (uint)(dx*wscale);
                dst[dx] = PixelBlend16_simd( dst[dx], src[src_x] );
            }
            if ( off )
            {
                ((ushort*) dst)[off] = PixelBlend16( ((ushort*) dst)[off], ((ushort*) src)[off] );
            }

            dst = (uint*) ( (char*) (dst) + job->dstPitch );
        }
    }
    else
    {
        const uint off = if_c_a_else_b(w&1, w-1, 0);
        for (uint dy = 0; dy != h; ++dy )
        {
            for (uint dx = 0; dx != rdx; ++dx )
            {
                dst[dx] = PixelBlend16_simd( dst[dx], src[dx] );
            }

            if ( off )
            {
                ((ushort*) dst)[off] = PixelBlend16( ((ushort*) dst)[off], ((ushort*) src)[off] );
            }

            src = (uint*) ( (char*) (src) + job->srcPitch );
            dst = (uint*) ( (char*) (dst) + job->dstPitch );
        }
    }
}

/*!
*/
static void executeBlit_TextureBlend_32_to_32( const SBlitJob * job )
{
    const uint w = job->width;
    const uint h = job->height;
    const uint *src = (uint*) job->src;
    uint *dst = (uint*) job->dst;

    if (job->stretch)
    {
        const float wscale = 1.f/job->x_stretch;
        const float hscale = 1.f/job->y_stretch;
        for ( uint dy = 0; dy < h; ++dy )
        {
            const uint src_y = (uint)(dy*hscale);
            src = (uint*) ( (char*) (job->src) + job->srcPitch*src_y );

            for ( uint dx = 0; dx < w; ++dx )
            {
                const uint src_x = (uint)(dx*wscale);
                dst[dx] = PixelBlend32( dst[dx], src[src_x] );
            }

            dst = (uint*) ( (char*) (dst) + job->dstPitch );
        }
    }
    else
    {
        for ( uint dy = 0; dy != h; ++dy )
        {
            for ( uint dx = 0; dx != w; ++dx )
            {
                dst[dx] = PixelBlend32( dst[dx], src[dx] );
            }
            src = (uint*) ( (char*) (src) + job->srcPitch );
            dst = (uint*) ( (char*) (dst) + job->dstPitch );
        }
    }
}

/*!
*/
static void executeBlit_TextureBlendColor_16_to_16( const SBlitJob * job )
{
    ushort *src = (ushort*) job->src;
    ushort *dst = (ushort*) job->dst;

    ushort blend = A8R8G8B8toA1R5G5B5 ( job->argb );
    for ( int dy = 0; dy != job->height; ++dy )
    {
        for ( int dx = 0; dx != job->width; ++dx )
        {
            if ( 0 == (src[dx] & 0x8000) )
                continue;

            dst[dx] = PixelMul16_2( src[dx], blend );
        }
        src = (ushort*) ( (char*) (src) + job->srcPitch );
        dst = (ushort*) ( (char*) (dst) + job->dstPitch );
    }
}


/*!
*/
static void executeBlit_TextureBlendColor_32_to_32( const SBlitJob * job )
{
    uint *src = (uint*) job->src;
    uint *dst = (uint*) job->dst;

    for ( int dy = 0; dy != job->height; ++dy )
    {
        for ( int dx = 0; dx != job->width; ++dx )
        {
            dst[dx] = PixelBlend32( dst[dx], PixelMul32_2( src[dx], job->argb ) );
        }
        src = (uint*) ( (char*) (src) + job->srcPitch );
        dst = (uint*) ( (char*) (dst) + job->dstPitch );
    }
}

/*!
*/
static void executeBlit_Color_16_to_16( const SBlitJob * job )
{
    const ushort c = A8R8G8B8toA1R5G5B5(job->argb);
    ushort *dst = (ushort*) job->dst;

    for ( int dy = 0; dy != job->height; ++dy )
    {
        memset16(dst, c, job->srcPitch);
        dst = (ushort*) ( (char*) (dst) + job->dstPitch );
    }
}

/*!
*/
static void executeBlit_Color_32_to_32( const SBlitJob * job )
{
    uint *dst = (uint*) job->dst;

    for ( int dy = 0; dy != job->height; ++dy )
    {
        memset32( dst, job->argb, job->srcPitch );
        dst = (uint*) ( (char*) (dst) + job->dstPitch );
    }
}

/*!
*/
static void executeBlit_ColorAlpha_16_to_16( const SBlitJob * job )
{
    ushort *dst = (ushort*) job->dst;

    const ushort alpha = extractAlpha( job->argb ) >> 3;
    if ( 0 == alpha )
        return;
    const uint src = A8R8G8B8toA1R5G5B5( job->argb );

    for ( int dy = 0; dy != job->height; ++dy )
    {
        for ( int dx = 0; dx != job->width; ++dx )
        {
            dst[dx] = 0x8000 | PixelBlend16( dst[dx], src, alpha );
        }
        dst = (ushort*) ( (char*) (dst) + job->dstPitch );
    }
}

/*!
*/
static void executeBlit_ColorAlpha_32_to_32( const SBlitJob * job )
{
    uint *dst = (uint*) job->dst;

    const uint alpha = extractAlpha( job->argb );
    const uint src = job->argb;

    for ( int dy = 0; dy != job->height; ++dy )
    {
        for ( int dx = 0; dx != job->width; ++dx )
        {
            dst[dx] = (job->argb & 0xFF000000 ) | PixelBlend32( dst[dx], src, alpha );
        }
        dst = (uint*) ( (char*) (dst) + job->dstPitch );
    }
}

inline uint PixelLerp32(const uint source, const uint value)
{
    uint srcRB = source & 0x00FF00FF;
    uint srcXG = (source & 0xFF00FF00) >> 8;

    srcRB *= value;
    srcXG *= value;

    srcRB >>= 8;
    //srcXG >>= 8;

    srcXG &= 0xFF00FF00;
    srcRB &= 0x00FF00FF;

    return srcRB | srcXG;
}

static inline uint extractAlpha(const uint c)
{
    return ( c >> 24 ) + ( c >> 31 );
}

inline uint if_c_a_else_b (const int condition, const uint a, const uint b)
{
    return ( ( -condition >> 31 ) & ( a ^ b ) ) ^ b;
}

// 1 - Bit Alpha Blending 16Bit SIMD
inline uint PixelBlend16_simd (const uint c2, const uint c1 )
{
    uint mask = ((c1 & 0x80008000) >> 15 ) + 0x7fff7fff;
    return (c2 & mask ) | ( c1 & ~mask );
}

// 1 - Bit Alpha Blending
inline uint PixelBlend16 (const ushort c2, const ushort c1 )
{
    ushort mask = ((c1 & 0x8000) >> 15 ) + 0x7fff;
    return (c2 & mask ) | ( c1 & ~mask );
}

inline uint PixelBlend32 (const uint c2, const uint c1 )
{
    // alpha test
    uint alpha = c1 & 0xFF000000;

    if ( 0 == alpha )
        return c2;

    if ( 0xFF000000 == alpha )
    {
        return c1;
    }

    alpha >>= 24;

    // add highbit alpha, if ( alpha > 127 ) alpha += 1;
    alpha += ( alpha >> 7);

    uint srcRB = c1 & 0x00FF00FF;
    uint srcXG = c1 & 0x0000FF00;

    uint dstRB = c2 & 0x00FF00FF;
    uint dstXG = c2 & 0x0000FF00;


    uint rb = srcRB - dstRB;
    uint xg = srcXG - dstXG;

    rb *= alpha;
    xg *= alpha;
    rb >>= 8;
    xg >>= 8;

    rb += dstRB;
    xg += dstXG;

    rb &= 0x00FF00FF;
    xg &= 0x0000FF00;

    return (c1 & 0xFF000000) | rb | xg;
}

/*
    Pixel = c0 * (c1/31).
*/
inline ushort PixelMul16_2 (ushort c0, ushort c1)
{
    return	(ushort)(( ( (c0 & 0x7C00) * (c1 & 0x7C00) ) & 0x3E000000 ) >> 15 |
            ( ( (c0 & 0x03E0) * (c1 & 0x03E0) ) & 0x000F8000 ) >> 10 |
            ( ( (c0 & 0x001F) * (c1 & 0x001F) ) & 0x000003E0 ) >> 5  |
            ( c0 & c1 & 0x8000));
}

/*
    Pixel = c0 * (c1/255).
*/
inline ushort PixelMul32_2 ( const uint c0, const uint c1)
{
    return	(( ( (c0 & 0xFF000000) >> 16 ) * ( (c1 & 0xFF000000) >> 16 ) ) & 0xFF000000 ) |
            (( ( (c0 & 0x00FF0000) >> 12 ) * ( (c1 & 0x00FF0000) >> 12 ) ) & 0x00FF0000 ) |
            (( ( (c0 & 0x0000FF00) * (c1 & 0x0000FF00) ) >> 16 ) & 0x0000FF00 ) |
            (( ( (c0 & 0x000000FF) * (c1 & 0x000000FF) ) >> 8  ) & 0x000000FF);
}

//! a more useful memset for pixel
// (standard memset only works with 8-bit values)
inline void memset16(void * dest, const ushort value, uint bytesize)
{
    ushort * d = (ushort*) dest;

    uint i;

    // loops unrolled to reduce the number of increments by factor ~8.
    i = bytesize >> (1 + 3);
    while (i)
    {
        d[0] = value;
        d[1] = value;
        d[2] = value;
        d[3] = value;

        d[4] = value;
        d[5] = value;
        d[6] = value;
        d[7] = value;

        d += 8;
        --i;
    }

    i = (bytesize >> 1 ) & 7;
    while (i)
    {
        d[0] = value;
        ++d;
        --i;
    }
}

//! a more useful memset for pixel
// (standard memset only works with 8-bit values)
inline void memset32(void * dest, const uint value, uint bytesize)
{
    uint * d = (uint*) dest;

    uint i;

    // loops unrolled to reduce the number of increments by factor ~8.
    i = bytesize >> (2 + 3);
    while (i)
    {
        d[0] = value;
        d[1] = value;
        d[2] = value;
        d[3] = value;

        d[4] = value;
        d[5] = value;
        d[6] = value;
        d[7] = value;

        d += 8;
        i -= 1;
    }

    i = (bytesize >> 2 ) & 7;
    while (i)
    {
        d[0] = value;
        d += 1;
        i -= 1;
    }
}

// bounce clipping to texture
inline void setClip ( AbsRectangle &out, const VRect<int> *clip,
                     const VImage * tex, int passnative )
{
    if ( clip && 0 == tex && passnative )
    {
        out.x0 = clip->UpperLeftCorner.X;
        out.x1 = clip->LowerRightCorner.X;
        out.y0 = clip->UpperLeftCorner.Y;
        out.y1 = clip->LowerRightCorner.Y;
        return;
    }

    const int w = tex ? tex->getDimension().Width : 0;
    const int h = tex ? tex->getDimension().Height : 0;
    if ( clip )
    {
        out.x0 = std::min(std::max(clip->UpperLeftCorner.X, 0), w);
        out.x1 = std::min(std::max(clip->LowerRightCorner.X, out.x0), w );
        out.y0 = std::min(std::max(clip->UpperLeftCorner.Y, 0), h );
        out.y1 = std::min(std::max(clip->LowerRightCorner.Y, out.y0), h );
    }
    else
    {
        out.x0 = 0;
        out.y0 = 0;
        out.x1 = w;
        out.y1 = h;
    }
}

//! 2D Intersection test
inline bool intersect ( AbsRectangle &dest, const AbsRectangle& a, const AbsRectangle& b)
{
    dest.x0 = std::max( a.x0, b.x0 );
    dest.y0 = std::max( a.y0, b.y0 );
    dest.x1 = std::min( a.x1, b.x1 );
    dest.y1 = std::min( a.y1, b.y1 );
    return dest.x0 < dest.x1 && dest.y0 < dest.y1;
}

}


#endif // VBLIT_H

