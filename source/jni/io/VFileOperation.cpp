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

NV_NAMESPACE_END
