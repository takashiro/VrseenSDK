#define  GFILE_CXX

#include "VFileOperation.h"
#include "VLog.h"

#include <math.h>

NV_NAMESPACE_BEGIN

static int FError ()
{
    if (errno == ENOENT) {
        return VAbstractFile::FileNotFound;
    } else if (errno == EACCES || errno == EPERM) {
        return VAbstractFile::AccessError;
    } else if (errno == ENOSPC) {
        return VAbstractFile::iskFullError;
    } else {
        return VAbstractFile::IOError;
    }
}

VAbstractFile *VOpenFile(const VString& path, int flags)
{
    return new VFileOperation(path, flags);
}

VFileOperation::VFileOperation(const VString& fileName, int flags)
  : m_fileName(fileName)
  , m_openFlag(flags)
{
    fileInit();
}

VFileOperation::VFileOperation(const char* fileName, int flags)
  : m_fileName(fileName)
  , m_openFlag(flags)
{
    fileInit();
}

void VFileOperation::fileInit()
{
    //openmode default is in and binary;
     std::ios_base::openmode openMode = std::ios_base::in | std::ios_base::binary;

    if (m_openFlag & Open_Truncate) {
        if(m_openFlag & Open_Read) {
            openMode = std::ios_base::trunc
                    | std::ios_base::in
                    | std::ios_base::out
                    | std::ios_base::binary;
        } else {
            openMode = std::ios_base::trunc
                    | std::ios_base::out
                    | std::ios_base::binary;
        }
    } else if (m_openFlag & Open_Create) {
        if (m_openFlag & Open_Read) {
            openMode = std::ios_base::app
                    | std::ios_base::in
                    | std::ios_base::binary;
        } else {
            openMode = std::ios_base::app
                    | std::ios_base::binary;
        }
    } else if (m_openFlag & Open_Write) {
        openMode = std::ios_base::in
                | std::ios_base::out
                | std::ios_base::binary;
    }

    open(m_fileName.toUtf8().data(), openMode);

    m_opened = is_open();

    // Set error code
    if (!m_opened) {
        m_errorCode = FError();
    } else {
        if(m_openFlag & Open_Read) {
            seekg(0);
        } else {
            seekp(0);
        }
        m_errorCode = 0;
    }
    m_lastOp = 0;
}

const std::string VFileOperation::filePath()
{
    return m_fileName.toStdString();
}

bool    VFileOperation::isOpened()
{
    return m_opened;
}

bool    VFileOperation::isWritable()
{
    return isOpened() && (m_openFlag & Open_Write);
}

int     VFileOperation::tell()
{
    int position;

    if (m_openFlag & Open_Read) {
        position = tellg();
    } else {
        position = tellp();
    }

    if (position < 0) {
        m_errorCode = FError();
    }
    return position;
}

long long  VFileOperation::tell64()
{
    long long position;

    if (m_openFlag & Open_Read) {
        position = tellg();
    } else {
        position = tellp();
    }

    if (position < 0) {
        m_errorCode = FError();
    }
    return position;
}

int     VFileOperation::length()
{
    int position = tell();
    if (position >= 0) {
        seek(0, std::ios_base::end);
        int len = tell();
        seek(position, std::ios_base::beg);
        return len;
    }
    return -1;
}

long long VFileOperation::length64()
{
    long long position = tell64();
    if (position >= 0) {
        seek64(0, std::ios_base::end);
        long long len = tell64();
        seek64(position, std::ios_base::beg);
        return len;
    }
    return -1;
}

int     VFileOperation::errorCode()
{
    return m_errorCode;
}

int     VFileOperation::write(const uchar *buffer, int byteNum)
{
    if (m_lastOp && m_lastOp != Open_Write) {
        bufferFlush();
    }
    m_lastOp = Open_Write;
    write(buffer, byteNum);

    if (!good()) {
        m_errorCode = FError();
        return 0;
    }

    return byteNum;
}

int     VFileOperation::read(uchar *buffer, int byteNum)
{
    if (m_lastOp && m_lastOp != Open_Read) {
        bufferFlush();
    }
    m_lastOp = Open_Read;

    read(buffer, byteNum);
    if (!good()) {
        m_errorCode = FError();
        return 0;
    }

    return byteNum;
}

// Seeks ahead to skip bytes
int     VFileOperation::skipBytes(int byteNum)
{
    long long oldPosition    = tell64();
    long long newPosition = seek64(byteNum, std::ios_base::cur);

    // Return -1 for major error
    if ((oldPosition==-1) || (newPosition==-1)) {
        return -1;
    }

    m_errorCode =((newPosition - oldPosition) < byteNum) ? errno : 0;
    return static_cast<int>(newPosition - oldPosition);
}

// Return # of bytes till EOF
int     VFileOperation::bytesAvailable()
{
    long long position    = tell64();
    long long endPosition = length64();

    // Return -1 for major error
    if ((position==-1) || (endPosition==-1)) {
        m_errorCode = FError();
        return 0;
    }
    else {
        m_errorCode = 0;
    }

    return static_cast<int>(endPosition - position);
}

// Flush file contents
bool    VFileOperation::bufferFlush()
{
    flush();
    return good();
}

int     VFileOperation::seek(int offset, std::ios_base::seekdir startPos)
{

    if (startPos == std::ios_base::beg && offset == tell()) {
        return tell();
    }

    if(m_openFlag & Open_Read) {
        seekg(offset, startPos);
    } else {
        seekp(offset, startPos);
    }

    if (!good()) {
        return -1;
    }

    return reinterpret_cast<int>(tell());
}

long long  VFileOperation::seek64(long long offset,std::ios_base::seekdir startPos)
{
    return seek(static_cast<int>(offset),startPos);
}

int VFileOperation::copyStream(VAbstractFile *fstream, int num)
{
    uchar temp[0x4000];
    int size = 0;
    int tempRead;
    int readNum;
    int tempWrite;

    while (num) {
        tempRead = (num > int(sizeof(temp))) ? int(sizeof(temp)) : num;

        readNum = fstream->read(temp, tempRead);
        tempWrite = 0;
        if (readNum > 0) {
            tempWrite = write(temp, readNum);
        }

        size += tempWrite;
        num -= tempWrite;
        if (tempWrite < tempRead){
            break;
        }
    }
    return size;
}


bool VFileOperation::close()
{
    close();
    bool isCloseRet = good();

    if (!isCloseRet) {
        m_errorCode = FError();
        return false;
    } else {
        m_opened = 0;
        m_errorCode = 0;
    }
    return true;
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

inline float FracFloat( const float x )
{
    return x - floorf( x );
}

unsigned char * VFileOperation::ScaleImageRGBA( const unsigned char * src, const int width, const int height, const int newWidth, const int newHeight, const ImageFilter filter )
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


NV_NAMESPACE_END

