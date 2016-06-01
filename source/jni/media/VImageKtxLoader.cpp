#include "VImageKtxLoader.h"
#include "VLog.h"
#include "VImageColor.h"
#include "VEglDriver.h"
#include "VPath.h"
#include "VIODevice.h"
#include "VMap.h"
#include "VDimension.h"
#include "VImage.h"

namespace NervGear
{

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

static bool GlFormatToTextureFormat( ColorFormat & format, const GLenum glFormat, const GLenum glInternalFormat )
{
    if ( glFormat == GL_RED && glInternalFormat == GL_R8 )
    {
        format = ECF_R;
        return true;
    }
    if ( glFormat == GL_RGB && ( glInternalFormat == GL_RGB || glInternalFormat == GL_SRGB8 ) )
    {
        format = ECF_RGB;
        return true;
    }
    if ( glFormat == GL_RGBA && ( glInternalFormat == GL_RGBA || glInternalFormat == GL_SRGB8_ALPHA8 ) )
    {
        format = ECF_RGBA;
        return true;
    }
    if ( ( glFormat == 0 || glFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ) && glInternalFormat == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT )
    {
        format = ECF_DXT1;
        return true;
    }
    if ( ( glFormat == 0 || glFormat == GL_RGB ) && glInternalFormat == GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG )
    {
        format = ECF_PVR4bRGB;
        return true;
    }
    if ( ( glFormat == 0 || glFormat == GL_RGBA ) && glInternalFormat == GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG )
    {
        format = ECF_PVR4bRGBA;
        return true;
    }
    if ( ( glFormat == 0 || glFormat == GL_RGB ) && ( glInternalFormat == GL_ETC1_RGB8_OES || glInternalFormat == GL_COMPRESSED_SRGB8_ETC2 ) )
    {
        format = ECF_ETC1;
        return true;
    }
    if ( ( glFormat == 0 || glFormat == GL_RGB ) && ( glInternalFormat == GL_COMPRESSED_RGB8_ETC2 || glInternalFormat == GL_COMPRESSED_SRGB8_ETC2 ) )
    {
        format = ECF_ETC2_RGB;
        return true;
    }
    if ( ( glFormat == 0 || glFormat == GL_RGBA ) && ( glInternalFormat == GL_COMPRESSED_RGBA8_ETC2_EAC || glInternalFormat == GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC ) )
    {
        format = ECF_ETC2_RGBA;
        return true;
    }
    if ( ( glFormat == 0 || glFormat == GL_RGB ) && glInternalFormat == GL_ATC_RGB_AMD )
    {
        format = ECF_ATC_RGB;
        return true;
    }
    if ( ( glFormat == 0 || glFormat == GL_RGBA ) && glInternalFormat == GL_ATC_RGBA_EXPLICIT_ALPHA_AMD )
    {
        format = ECF_ATC_RGBA;
        return true;
    }
    return false;
}

    bool VImageKtxLoader::isValid(const VPath& filename) const
    {
        return filename.extension().toLower() == "ktx";
    }

    bool VImageKtxLoader::isValid(VIODevice *file) const
    {
        if (file->size() < (int)sizeof(OVR_KTX_HEADER))
        {
            vWarn("Invalid KTX file");
            return false;
        }
        else
            return true;
    }

    VImage* VImageKtxLoader::load(VIODevice *file) const
    {
        char* buffer = new char[file->size()];
        file->read(buffer, file->size());

        const uchar fileIdentifier[12] =
        {
            171, 75, 84, 88, 32, 49, 49, 187, 13, 10, 26, 10
        };

        const OVR_KTX_HEADER & header = *(OVR_KTX_HEADER *)buffer;
        if ( memcmp( header.identifier, fileIdentifier, sizeof( fileIdentifier ) ) != 0 )
        {
            vInfo("Invalid KTX file");
            return 0;
        }
        // only support little endian
        if ( header.endianness != 0x04030201 )
        {
            vInfo("KTX file has wrong endianess");
            return 0;
        }
        // only support compressed or unsigned byte
        if ( header.glType != 0 && header.glType != GL_UNSIGNED_BYTE )
        {
            vInfo("KTX file has unsupported glType " << header.glType);
            return 0;
        }
        // no support for texture arrays
        if ( header.numberOfArrayElements != 0 )
        {
            vInfo("KTX file has unsupported number of array elements " << header.numberOfArrayElements);
            return 0;
        }
        // derive the texture format from the GL format
        ColorFormat format;
        if ( !GlFormatToTextureFormat( format, header.glFormat, header.glInternalFormat ) )
        {
            vInfo("KTX file has unsupported glFormat " << header.glFormat << ", glInternalFormat " << header.glInternalFormat);
            return 0;
        }
        // skip the key value data
        const uintptr_t startTex = sizeof( OVR_KTX_HEADER ) + header.bytesOfKeyValueData;
        if ( ( startTex < sizeof( OVR_KTX_HEADER ) ) || ( startTex >= static_cast< size_t >( file->size() ) ) )
        {
            vInfo("Invalid KTX header sizes");
            return 0;
        }

        int width = header.pixelWidth;
        int height = header.pixelHeight;

        VMap<VString, VString> info;
        info["mipCount"] = VString::number((int)header.numberOfMipmapLevels);

        if ( header.numberOfFaces == 1 )
        {
            info["numberOfFaces"] = "1";
        }
        else if ( header.numberOfFaces == 6 )
        {
            info["numberOfFaces"] = "6";
        }
        else
        {
            vInfo("KTX file has unsupported number of faces " << header.numberOfFaces);
        }

        VImage* image = new VImage(format, VDimension<uint>(width, height), buffer + startTex, file->size() - startTex, info);
        delete [] buffer;

        return image;
    }






}
