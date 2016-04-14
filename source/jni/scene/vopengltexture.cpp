#include "VOpenGLTexture.h"

namespace NervGear{

//! constructor for usual textures
VOpenGLTexture::VOpenGLTexture(VImage* origImage, const VPath& name, void* mipmapData)
    : VTexture(name), ColorFormat(ECF_A8R8G8B8), Image(0), MipImage(0),
    TextureName(0), InternalFormat(GL_RGBA), PixelFormat(GL_BGRA_EXT),
    PixelType(GL_UNSIGNED_BYTE), MipLevelStored(0), MipmapLegacyMode(true),
    IsRenderTarget(false), AutomaticMipmapUpdate(false),
    ReadOnlyLock(false), KeepImage(true)
{
    // TODO::MipMap need to specified
    HasMipMaps = VTexture::getTextureCreationFlag(ETCF_CREATE_MIP_MAPS);
    getImageValues(origImage);

    glGenTextures(1, &TextureName);

    if (ImageSize==TextureSize)
    {
        Image = createImage(ColorFormat, ImageSize);
        origImage->copyTo(Image);
    }
    else
    {
        Image = createImage(ColorFormat, TextureSize);
        // scale texture
        origImage->copyToScaling(Image);
    }
    uploadTexture(true, mipmapData);
    if (!KeepImage)
    {
        Image->drop();
        Image=0;
    }
}


//! constructor for basic setup (only for derived classes)
VOpenGLTexture::VOpenGLTexture(const VPath& name)
    : VTexture(name), ColorFormat(ECF_A8R8G8B8), Image(0), MipImage(0),
    TextureName(0), InternalFormat(GL_RGBA), PixelFormat(GL_BGRA_EXT),
    PixelType(GL_UNSIGNED_BYTE), MipLevelStored(0), HasMipMaps(true),
    MipmapLegacyMode(true), IsRenderTarget(false), AutomaticMipmapUpdate(false),
    ReadOnlyLock(false), KeepImage(true)
{
    #ifdef _DEBUG
    setDebugName("VOpenGLTexture");
    #endif
}


//! destructor
VOpenGLTexture::~VOpenGLTexture()
{
    if (TextureName)
        glDeleteTextures(1, &TextureName);
    if (Image)
        Image->drop();
}


//! Choose best matching color format, based on texture creation flags
ColorFormat VOpenGLTexture::getBestColorFormat(ColorFormat format)
{
    ColorFormat destFormat = ECF_A8R8G8B8;
    switch (format)
    {
        case ECF_A1R5G5B5:
            if (VTexture::getTextureCreationFlag(ETCF_ALWAYS_32_BIT))
                destFormat = ECF_A1R5G5B5;
        break;
        case ECF_R5G6B5:
            if (VTexture::getTextureCreationFlag(ETCF_ALWAYS_32_BIT))
                destFormat = ECF_A1R5G5B5;
        break;
        case ECF_A8R8G8B8:
            if (VTexture::getTextureCreationFlag(ETCF_ALWAYS_16_BIT) ||
                    VTexture::getTextureCreationFlag(ETCF_OPTIMIZED_FOR_SPEED))
                destFormat = ECF_A1R5G5B5;
        break;
        case ECF_R8G8B8:
            if (VTexture::getTextureCreationFlag(ETCF_ALWAYS_16_BIT) ||
                    VTexture::getTextureCreationFlag(ETCF_OPTIMIZED_FOR_SPEED))
                destFormat = ECF_A1R5G5B5;
        default:
        break;
    }
    if (VTexture::getTextureCreationFlag(ETCF_NO_ALPHA_CHANNEL))
    {
        switch (destFormat)
        {
            case ECF_A1R5G5B5:
                destFormat = ECF_R5G6B5;
            break;
            case ECF_A8R8G8B8:
                destFormat = ECF_R8G8B8;
            break;
            default:
            break;
        }
    }
    return destFormat;
}


//! Get opengl values for the GPU texture storage
GLint VOpenGLTexture::getOpenGLFormatAndParametersFromColorFormat(ColorFormat format,
                GLint& filtering,
                GLenum& colorformat,
                GLenum& type)
{
    // default
    filtering = GL_LINEAR;
    colorformat = GL_RGBA;
    type = GL_UNSIGNED_BYTE;
    GLenum internalformat = GL_RGBA;

    switch(format)
    {
        case ECF_A1R5G5B5:
            //OpenGL ES can't support GL_UNSIGNED_SHORT_1_5_5_5_REV;
            //colorformat=GL_BGRA_EXT;
            //type=GL_UNSIGNED_SHORT_1_5_5_5_REV;
            //internalformat =  GL_RGBA;
            break;
        case ECF_R5G6B5:
            colorformat=GL_RGB;
            type=GL_UNSIGNED_SHORT_5_6_5;
            internalformat =  GL_RGB;
            break;
        case ECF_R8G8B8:
            //OpenGL ES can't support GL_BGR;
            //colorformat=GL_BGR;
            //type=GL_UNSIGNED_BYTE;
            //internalformat =  GL_RGB;
            break;
        case ECF_A8R8G8B8:
            //OpenGL ES can't support GL_UNSIGNED_INT_8_8_8_8_REV;
            //colorformat=GL_BGRA_EXT;
            //type=GL_UNSIGNED_INT_8_8_8_8_REV;
            //internalformat =  GL_RGBA;
            break;
        // Floating Point texture formats. Thanks to Patryk "Nadro" Nadrowski.
        case ECF_R16F:
        {
            ColorFormat = ECF_A8R8G8B8;
            internalformat =  GL_RGB8;
        }
            break;
        case ECF_G16R16F:
        {
            ColorFormat = ECF_A8R8G8B8;
            internalformat =  GL_RGB8;
        }
            break;
        case ECF_A16B16G16R16F:
        {
            ColorFormat = ECF_A8R8G8B8;
            internalformat =  GL_RGBA8;
        }
            break;
        case ECF_R32F:
        {
            colorformat = ECF_A8R8G8B8;
            internalformat =  GL_RGB8;
        }
            break;
        case ECF_G32R32F:
        {
            colorformat = ECF_A8R8G8B8;
            internalformat =  GL_RGB8;
        }
            break;
        case ECF_A32B32G32R32F:
        {
            colorformat = ECF_A8R8G8B8;
            internalformat =  GL_RGBA8;
        }
            break;
        default:
        {
            vInfo("Unsupported texture format");
            internalformat =  GL_RGBA8;
        }
    }
    return internalformat;
}


// prepare values ImageSize, TextureSize, and ColorFormat based on image
void VOpenGLTexture::getImageValues(VImage* image)
{
    if (!image)
    {
        vInfo("No image for OpenGL texture.");
        return;
    }

    ImageSize = image->getDimension();

    if ( !ImageSize.Width || !ImageSize.Height)
    {
        vInfo("Invalid size of image for OpenGL Texture.");
        return;
    }

    const float ratio = (float)ImageSize.Width/(float)ImageSize.Height;
    //2048 is the common max texture size for mobile
    if ((ImageSize.Width>2048) && (ratio >= 1.0f))
    {
        ImageSize.Width = 2048;
        ImageSize.Height = (uint)(2048/ratio);
    }
    else if (ImageSize.Height>2048)
    {
        ImageSize.Height = 2048;
        ImageSize.Width = (uint)(2048*ratio);
    }
    TextureSize=ImageSize.getOptimalSize(true);

    ColorFormat = getBestColorFormat(image->getColorFormat());
}


//! copies the the texture into an open gl texture.
void VOpenGLTexture::uploadTexture(bool newTexture, void* mipmapData, uint level)
{
    // check which image needs to be uploaded
    VImage* image = level?MipImage:Image;
    VGlOperation gloperation;
    if (!image)
    {
        vInfo("No image for OpenGL texture to upload");
        return;
    }

    // get correct opengl color data values
    GLenum oldInternalFormat = InternalFormat;
    GLint filtering;
    InternalFormat = getOpenGLFormatAndParametersFromColorFormat(ColorFormat, filtering, PixelFormat, PixelType);
    // make sure we don't change the internal format of existing images
    if (!newTexture)
        InternalFormat=oldInternalFormat;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, static_cast<const VOpenGLTexture*>(this)->getOpenGLTextureName());
    if (gloperation.getEglErrorString())
        vInfo(gloperation.getEglErrorString());

    // mipmap handling for main texture
    if (!level && newTexture)
    {

        // Either generate manually due to missing capability
        // or use predefined mipmap data
        AutomaticMipmapUpdate=false;
        regenerateMipMapLevels(mipmapData);

        if (HasMipMaps) // might have changed in regenerateMipMapLevels
        {
        // enable bilinear mipmap filter
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
        else
        {
        // enable bilinear filter without mipmaps
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
   }

    // now get image data and upload to GPU
    void* source = image->lock();
    if (newTexture)
        glTexImage2D(GL_TEXTURE_2D, level, InternalFormat, image->getDimension().Width,
            image->getDimension().Height, 0, PixelFormat, PixelType, source);
    else
        glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, image->getDimension().Width,
            image->getDimension().Height, PixelFormat, PixelType, source);
    image->unlock();


    if (gloperation.getEglErrorString())
        vInfo(gloperation.getEglErrorString());
}


//! lock function
void* VOpenGLTexture::lock(E_TEXTURE_LOCK_MODE mode, uint mipmapLevel)
{
    // store info about which image is locked
    VImage* image = (mipmapLevel==0)?Image:MipImage;
    ReadOnlyLock |= (mode==ETLM_READ_ONLY);
    MipLevelStored = mipmapLevel;
    if (!ReadOnlyLock && mipmapLevel)
    {
        AutomaticMipmapUpdate=false;
    }
    // if data not available or might have changed on GPU download it
    if (!image || IsRenderTarget)
    {
        // prepare the data storage if necessary
        if (!image)
        {
            if (mipmapLevel)
            {
                uint i=0;
                uint width = TextureSize.Width;
                uint height = TextureSize.Height;
                do
                {
                    if (width>1)
                        width>>=1;
                    if (height>1)
                        height>>=1;
                    ++i;
                }
                while (i != mipmapLevel);
                MipImage = image = createImage(ECF_A8R8G8B8, VDimensionu(width,height));
            }
            else
                Image = image = createImage(ECF_A8R8G8B8, ImageSize);
            ColorFormat = ECF_A8R8G8B8;
        }
        if (!image)
            return 0;

        if (mode != ETLM_WRITE_ONLY)
        {
            char* pixels = static_cast<char*>(image->lock());
            if (!pixels)
                return 0;

            // we need to keep the correct texture bound later on
            GLint tmpTexture;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &tmpTexture);
            glBindFramebuffer(GL_FRAMEBUFFER, TextureName);

            // we need to flip textures vertical
            // however, it seems that this does not hold for mipmap
            // textures, for unknown reasons.


            // download GPU data as ARGB8 to pixels;
            glReadPixels(0, 0, image->getDimension().Width/pow(2, mipmapLevel), image->getDimension().Height/pow(2, mipmapLevel), GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            if (!mipmapLevel)
            {
                    // opengl images are horizontally flipped, so we have to fix that here.
                    const int pitch=image->getPitch();
                    char* p2 = pixels + (image->getDimension().Height - 1) * pitch;
                    char* tmpBuffer = new char[pitch];
                    for (uint i=0; i < image->getDimension().Height; i += 2)
                    {
                        memcpy(tmpBuffer, pixels, pitch);
                        memcpy(pixels, p2, pitch);
                        memcpy(p2, tmpBuffer, pitch);
                        pixels += pitch;
                        p2 -= pitch;
                    }
                    delete [] tmpBuffer;

            }
            image->unlock();
        }
    }
    return image->lock();
}


//! unlock function
void VOpenGLTexture::unlock()
{
    // test if miplevel or main texture was locked
    VImage* image = MipImage?MipImage:Image;
    if (!image)
        return;
    // unlock image to see changes
    image->unlock();
    // copy texture data to GPU
    if (!ReadOnlyLock)
        uploadTexture(false, 0, MipLevelStored);
    ReadOnlyLock = false;
    // cleanup local image
    if (MipImage)
    {
        MipImage->drop();
        MipImage=0;
    }
    else if (!KeepImage)
    {
        Image->drop();
        Image=0;
    }
    // update information
    if (Image)
        ColorFormat=Image->getColorFormat();
    else
        ColorFormat=ECF_A8R8G8B8;
}


//! Returns size of the original image.
const VDimension<uint>& VOpenGLTexture::getOriginalSize() const
{
    return ImageSize;
}


//! Returns size of the texture.
const VDimension<uint>& VOpenGLTexture::getSize() const
{
    return TextureSize;
}


//! returns color format of texture
ColorFormat VOpenGLTexture::getColorFormat() const
{
    return ColorFormat;
}


//! returns pitch of texture (in bytes)
uint VOpenGLTexture::getPitch() const
{
    if (Image)
        return Image->getPitch();
    else
        return 0;
}


//! return open gl texture name
GLuint VOpenGLTexture::getOpenGLTextureName() const
{
    return TextureName;
}


//! Returns whether this texture has mipmaps
bool VOpenGLTexture::hasMipMaps() const
{
    return HasMipMaps;
}


//! Regenerates the mip map levels of the texture. Useful after locking and
//! modifying the texture
void VOpenGLTexture::regenerateMipMapLevels(void* mipmapData)
{
    if (AutomaticMipmapUpdate || !HasMipMaps || !Image)
        return;
    if ((Image->getDimension().Width==1) && (Image->getDimension().Height==1))
        return;

    // Manually create mipmaps or use prepared version
    uint width=Image->getDimension().Width;
    uint height=Image->getDimension().Height;
    uint i=0;
    char* target = static_cast<char*>(mipmapData);
    do
    {
        if (width>1)
            width>>=1;
        if (height>1)
            height>>=1;
        ++i;
        if (!target)
            target = new char[width*height*Image->getBytesPerPixel()];
        // create scaled version if no mipdata available
        if (!mipmapData)
            Image->copyToScaling(target, width, height, Image->getColorFormat());
        glTexImage2D(GL_TEXTURE_2D, i, InternalFormat, width, height,
                0, PixelFormat, PixelType, target);
        // get next prepared mipmap data if available
        if (mipmapData)
        {
            mipmapData = static_cast<char*>(mipmapData)+width*height*Image->getBytesPerPixel();
            target = static_cast<char*>(mipmapData);
        }
    }
    while (width!=1 || height!=1);
    // cleanup
    if (!mipmapData)
        delete [] target;
}


bool VOpenGLTexture::isRenderTarget() const
{
    return IsRenderTarget;
}


void VOpenGLTexture::setIsRenderTarget(bool isTarget)
{
    IsRenderTarget = isTarget;
}


bool VOpenGLTexture::isFrameBufferObject() const
{
    return false;
}


//! Bind Render Target Texture
void VOpenGLTexture::bindRTT()
{
}


//! Unbind Render Target Texture
void VOpenGLTexture::unbindRTT()
{
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, static_cast<const VOpenGLTexture*>(this)->getOpenGLTextureName());

    // Copy Our ViewPort To The Texture
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, getSize().Width, getSize().Height);
}



static VImage* createImage(ColorFormat format, const VDimension<uint>& size)
{
    if (VImage::isRenderTargetOnlyFormat(format))
    {
        vInfo("Could not create IImage, format only supported for render target textures.");
        return 0;
    }
    return new CImage(format, size);
}

}
