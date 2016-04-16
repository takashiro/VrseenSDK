#ifndef VOPENGLTEXTURE_H
#define VOPENGLTEXTURE_H

#include "VGlOperation.h"
#include "VTexture.h"
#include "VImage.h"
#include "VFlags.h"

namespace NervGear{


#define GL_COMPRESSED_RGBA_ASTC_4x4_KHR            0x93B0
#define GL_COMPRESSED_RGBA_ASTC_5x4_KHR            0x93B1
#define GL_COMPRESSED_RGBA_ASTC_5x5_KHR            0x93B2
#define GL_COMPRESSED_RGBA_ASTC_6x5_KHR            0x93B3
#define GL_COMPRESSED_RGBA_ASTC_6x6_KHR            0x93B4
#define GL_COMPRESSED_RGBA_ASTC_8x5_KHR            0x93B5
#define GL_COMPRESSED_RGBA_ASTC_8x6_KHR            0x93B6
#define GL_COMPRESSED_RGBA_ASTC_8x8_KHR            0x93B7
#define GL_COMPRESSED_RGBA_ASTC_10x5_KHR           0x93B8
#define GL_COMPRESSED_RGBA_ASTC_10x6_KHR           0x93B9
#define GL_COMPRESSED_RGBA_ASTC_10x8_KHR           0x93BA
#define GL_COMPRESSED_RGBA_ASTC_10x10_KHR          0x93BB
#define GL_COMPRESSED_RGBA_ASTC_12x10_KHR          0x93BC
#define GL_COMPRESSED_RGBA_ASTC_12x12_KHR          0x93BD
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR    0x93D0
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR    0x93D1
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR    0x93D2
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR    0x93D3
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR    0x93D4
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR    0x93D5
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR    0x93D6
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR    0x93D7
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR   0x93D8
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR   0x93D9
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR   0x93DA
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR  0x93DB
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR  0x93DC
#define GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR  0x93DD

enum eTextureFlags
{
    // Normally, a failure to load will create an 8x8 default texture, but
    // if you want to take explicit action, setting this flag will cause
    // it to return 0 for the texId.
    TEXTUREFLAG_NO_DEFAULT,
    // Use GL_SRGB8 / GL_SRGB8_ALPHA8 / GL_COMPRESSED_SRGB8_ETC2 formats instead
    // of GL_RGB / GL_RGBA / GL_ETC1_RGB8_OES
    TEXTUREFLAG_USE_SRGB,
    // No mip maps are loaded or generated when this flag is specified.
    TEXTUREFLAG_NO_MIPMAPS
};

typedef VFlags<eTextureFlags> TextureFlags_t;

//! OpenGL texture.
class VOpenGLTexture : public VTexture
{
public:

    //! constructor
    VOpenGLTexture(VImage* surface, const VPath& name, void* mipmapData=0);

    VOpenGLTexture();

    VOpenGLTexture(VImage* origImage, const VPath& name, const TextureFlags_t & flags);

    //! destructor
    virtual ~VOpenGLTexture();

    //! lock function
    virtual void* lock(E_TEXTURE_LOCK_MODE mode=ETLM_READ_WRITE, uint mipmapLevel=0);

    //! unlock function
    virtual void unlock();

    //! Returns original size of the texture (image).
    virtual const VDimension<uint>& getOriginalSize() const;

    //! Returns size of the texture.
    virtual const VDimension<uint>& getSize() const;

    //! returns driver type of texture (=the driver, that created it)
    //virtual E_DRIVER_TYPE getDriverType() const;

    //! returns color format of texture
    virtual ColorFormat getColorFormat() const;

    //! returns pitch of texture (in bytes)
    virtual uint getPitch() const;

    //! return open gl texture name
    GLuint getOpenGLTextureName() const;

    //! return whether this texture has mipmaps
    virtual bool hasMipMaps() const;

    //! Regenerates the mip map levels of the texture.
    /** Useful after locking and modifying the texture
    \param mipmapData Pointer to raw mipmap data, including all necessary mip levels, in the same format as the main texture image. If not set the mipmaps are derived from the main image. */
    virtual void regenerateMipMapLevels(void* mipmapData=0);

    //! Is it a render target?
    virtual bool isRenderTarget() const;

    //! Is it a FrameBufferObject?
    virtual bool isFrameBufferObject() const;

    //! Bind RenderTargetTexture
    virtual void bindRTT();

    //! Unbind RenderTargetTexture
    virtual void unbindRTT();

    //! sets whether this texture is intended to be used as a render target.
    void setIsRenderTarget(bool isTarget);

    GLuint getTextureName();

    GLuint getTargetType();

    void setTextureName(GLuint name);

    void setTargetType(GLuint type);



protected:

    //! protected constructor with basic setup, no GL texture name created, for derived classes
    VOpenGLTexture(const VPath& name);

    //! get the desired color format based on texture creation flags and the input format.
    ColorFormat getBestColorFormat(ColorFormat format);

    //! Get the OpenGL color format parameters based on the given Irrlicht color format
    GLint getOpenGLFormatAndParametersFromColorFormat(
        ColorFormat format, GLint& filtering, GLenum& colorformat, GLenum& type);

    //! get important numbers of the image and hw texture
    void getImageValues(VImage* image);

    //! copies the texture into an OpenGL texture.
    /** \param newTexture True if method is called for a newly created texture for the first time. Otherwise call with false to improve memory handling.
    \param mipmapData Pointer to raw mipmap data, including all necessary mip levels, in the same format as the main texture image.
    \param mipLevel If set to non-zero, only that specific miplevel is updated, using the MipImage member. */
    void uploadTexture(bool newTexture=false, void* mipmapData=0, uint mipLevel=0);

    VDimension<uint> ImageSize;
    VDimension<uint> TextureSize;
    ColorFormat m_ColorFormat;
    //COpenGLDriver* Driver;
    VImage* Image;
    VImage* MipImage;
    GLuint TextureName;
    GLuint TargetType;



    GLint InternalFormat;
    GLenum PixelFormat;
    GLenum PixelType;

    char MipLevelStored;
    bool HasMipMaps;
    bool MipmapLegacyMode;
    bool IsRenderTarget;
    bool AutomaticMipmapUpdate;
    bool ReadOnlyLock;
    bool KeepImage;
};


}

#endif
