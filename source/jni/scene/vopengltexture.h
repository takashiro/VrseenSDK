#ifndef VOPENGLTEXTURE_H
#define VOPENGLTEXTURE_H


#include "VGlOperation.h"
#include "VTexture.h"
#include "VImage.h"

namespace NervGear{

//! OpenGL texture.
class VOpenGLTexture : public VTexture
{
public:

    //! constructor
    VOpenGLTexture(VImage* surface, const VPath& name, void* mipmapData=0);

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
    std::shared_ptr<VImage> Image;
    std::shared_ptr<VImage> MipImage;

    GLuint TextureName;
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
