#pragma once

#include "VFlags.h"
#include "VString.h"

NV_NAMESPACE_BEGIN

class VFile;
class VResource;

class VTexture
{
public:
    enum Flag
    {
        // Normally, a failure to load will create an 8x8 default texture, but
        // if you want to take explicit action, setting this flag will cause
        // it to return 0 for the texId.

        NoDefault,
        // Use GL_SRGB8 / GL_SRGB8_ALPHA8 / GL_COMPRESSED_SRGB8_ETC2 formats instead
        // of GL_RGB / GL_RGBA / GL_ETC1_RGB8_OES
        UseSRGB,

        // No mip maps are loaded or generated when this flag is specified.
        NoMipmaps
    };
    typedef VFlags<Flag> Flags;

    VTexture();
    VTexture(uint id);
    VTexture(uint id, uint target);

    VTexture(const VTexture &source);
    VTexture(VTexture &&source);

    VTexture(VFile &file, const Flags &flags = NoDefault);
    VTexture(const VResource &resource, const Flags &flags = NoDefault);

    VTexture(const VString &format, const VByteArray &data, const Flags &flags = NoDefault);

    ~VTexture();

    VTexture &operator=(const VTexture &source);
    VTexture &operator=(VTexture &&source);

    operator uint() const { return id(); }
    const uint &id() const;
    const uint &target() const;

    int width() const;
    int height() const;

private:
    NV_DECLARE_PRIVATE
};

// Allocates a GPU texture and uploads the raw data.
VTexture LoadRGBATextureFromMemory(const uchar * texture, const int width, const int height, const bool useSrgbFormat);
VTexture LoadRGBTextureFromMemory(const uchar *texture, const int width, const int height, const bool useSrgbFormat);
VTexture LoadRTextureFromMemory(const uchar * texture, const int width, const int height);
VTexture LoadASTCTextureFromMemory(const uchar * buffer, uint bufferSize, const int numPlanes);

void		MakeTextureClamped(const VTexture &texid);
void		MakeTextureLodClamped(const VTexture &texId, int maxLod );
void		MakeTextureTrilinear(const VTexture &texture);
void		MakeTextureLinear(const VTexture &texture);
void		MakeTextureAniso(const VTexture &texId, float maxAniso);
void		BuildTextureMipmaps(const VTexture &texture);

// FileName's extension determines the file type, but the data is taken from an
// already loaded buffer.
//
// The stb_image file formats are supported:
// .jpg .tga .png .bmp .psd .gif .hdr .pic
//
// Limited support for the PVR and KTX container formats.
//
// If TEXTUREFLAG_NO_DEFAULT, no default texture will be created.
// Otherwise a default square texture will be created on any failure.
//
// Uncompressed image formats will have mipmaps generated and trilinear filtering set.
VTexture	LoadTextureFromBuffer( const char * fileName, const void* buffer, uint length,
                const VTexture::Flags & flags, int & width, int & height );

unsigned char * LoadPVRBuffer( const char * fileName, int & width, int & height );

NV_NAMESPACE_END
