#define  GFILE_CXX

#include "VFileOperation.h"
#include "VLog.h"

#include <math.h>

NV_NAMESPACE_BEGIN

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

void VFileOperation::Write32BitPvrTexture( const char * fileName, const unsigned char * texture, int width, int height )
{
    FILE *f = fopen( fileName, "wb" );
    if ( !f )
    {
        vWarn("Failed to write" << fileName);
        return;
    }

    OVR_PVR_HEADER header = {};
    header.Version = 0x03525650;                // 'PVR' + 0x3
    header.PixelFormat = 578721384203708274ull; // 8888 RGBA
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

inline int AbsInt( const int x )
{
    const int mask = x >> ( sizeof( int )* 8 - 1 );
    return ( x + mask ) ^ mask;
}

inline int ClampInt( const int x, const int min, const int max )
{
    return min + ( ( AbsInt( x - min ) - AbsInt( x - min - max ) + max ) >> 1 );
}

unsigned char * VFileOperation::QuarterImageSize( const unsigned char * src, const int width, const int height, const bool srgb )
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

NV_NAMESPACE_END
