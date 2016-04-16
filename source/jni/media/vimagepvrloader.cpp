#include "VImagePvrLoader.h"
#include "VLog.h"

namespace NervGear{

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

bool VImagePvrLoader::isALoadableFileExtension(const VPath &filename) const
{
    return filename.extension().toLower() == "pvr";
}

bool VImagePvrLoader::isALoadableFileFormat(VFile *file) const
{
    if ( file->size() < ( int )( sizeof( OVR_PVR_HEADER ) ) )
    {
        vInfo("Invalid PVR file");
        return false;
    }
    else
        return true;
}

VImage* VImagePvrLoader::loadImage(VFile *file) const
{
    char* buffer = new char[file->size()];
    file->read(buffer, file->size());


    const OVR_PVR_HEADER & header = *( OVR_PVR_HEADER * )buffer;
    if ( header.Version != 0x03525650 )
    {
        vInfo("Invalid PVR file version");
        return 0;
    }

    ColorFormat format;
    switch ( header.PixelFormat )
    {
        case 2:						format = ECF_PVR4bRGB;	break;
        case 3:						format = ECF_PVR4bRGBA;	break;
        case 6:						format = ECF_ETC1;		break;
        case 22:					format = ECF_ETC2_RGB;	break;
        case 23:					format = ECF_ETC2_RGBA;	break;
        case 578721384203708274ull:	format = ECF_RGBA;		break;
        default:
            vInfo("Unknown PVR texture format ");
            return 0;
    }

    // skip the metadata
    const vuint32 startTex = sizeof( OVR_PVR_HEADER ) + header.MetaDataSize;
    if ( ( startTex < sizeof( OVR_PVR_HEADER ) ) || ( startTex >= static_cast< size_t >( file->size() ) ) )
    {
        vInfo("Invalid PVR header sizes");
        return 0;
    }
    VMap<VString, VString> info;
    info["mipCount"] = VString::number((int)header.MipMapCount);

    int width = header.Width;
    int height = header.Height;

    if ( header.NumFaces == 1 )
    {
        info["NumFaces"] = "1";
    }
    else if ( header.NumFaces == 6 )
    {
        info["NumFaces"] = "6";
    }
    else
    {
        vInfo("PVR file has unsupported number of faces ");
    }

    VImage* image = new VImage(format, VDimension<uint>(width, height), buffer + startTex, file->size() - startTex, info);
    delete [] buffer;

    return image;
}




}
