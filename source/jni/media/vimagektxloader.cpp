#include "VImageKtxLoader.h"
#include "VLog.h"

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

    bool VImageKtxLoader::isALoadableFileExtension(const VPath& filename) const
    {
        return filename.extension() == "ktx";
    }

    bool VImageKtxLoader::isALoadableFileFormat(VFile *file) const
    {
        if (file->size() < (int)sizeof(OVR_KTX_HEADER))
        {
            vInfo(file->path() << ": Invalid KTX file");
            return false;
        }
    }

    std::shared_ptr<VImage> VImageKtxLoader::loadImage(VFile *file) const
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
            vInfo(file->path() << ": Invalid KTX file");
            return 0;
        }
        // only support little endian
        if ( header.endianness != 0x04030201 )
        {
            vInfo(file->path() << ": KTX file has wrong endianess");
            return 0;
        }
        // only support compressed or unsigned byte
        if ( header.glType != 0 && header.glType != GL_UNSIGNED_BYTE )
        {
            vInfo(file->path() << ": KTX file has unsupported glType " << header.glType);
            return 0;
        }
        // no support for texture arrays
        if ( header.numberOfArrayElements != 0 )
        {
            vInfo(file->path() << ": KTX file has unsupported number of array elements " << header.numberOfArrayElements);
            return 0;
        }
        // derive the texture format from the GL format
        int format = 0;
        if ( !GlFormatToTextureFormat( format, header.glFormat, header.glInternalFormat ) )
        {
            vInfo(file->path() << ": KTX file has unsupported glFormat " << header.glFormat << ", glInternalFormat " << header.glInternalFormat);
            return 0;
        }
        // skip the key value data
        const uintptr_t startTex = sizeof( OVR_KTX_HEADER ) + header.bytesOfKeyValueData;
        if ( ( startTex < sizeof( OVR_KTX_HEADER ) ) || ( startTex >= static_cast< size_t >( bufferLength ) ) )
        {
            vInfo(file->path() << ": Invalid KTX header sizes");
            return 0;
        }

        int width = header.pixelWidth;
        int height = header.pixelHeight;

        const vuint32 mipCount = ( noMipMaps ) ? 1 : std::max( 1u, header.numberOfMipmapLevels );

        if ( header.numberOfFaces == 1 )
        {
            return CreateGlTexture( fileName, format, width, height, buffer + startTex, bufferLength - startTex, mipCount, useSrgbFormat, true );
        }
        else if ( header.numberOfFaces == 6 )
        {
            return CreateGlCubeTexture( fileName, format, width, height, buffer + startTex, bufferLength - startTex, mipCount, useSrgbFormat, true );
        }
        else
        {
            vInfo(file->path() << ": KTX file has unsupported number of faces " << header.numberOfFaces);
        }

        width = 0;
        height = 0;
        return 0;
    }



}
