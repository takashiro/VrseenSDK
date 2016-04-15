#include "VOpenGLTexture.h"
#include "3rdParty/stb/stb_image.h"
#include "VImageCommonLoader.h"
#include "VImageKtxLoader.h"
#include "VImagePvrLoader.h"

namespace NervGear{

//! constructor for usual textures
VOpenGLTexture::VOpenGLTexture(VImage* origImage, const VPath& name, void* mipmapData)
    : VTexture(name), ColorFormat(ECF_A8R8G8B8), Image(0), MipImage(0),
    TextureName(0), TargetType(0), InternalFormat(GL_RGBA), PixelFormat(GL_BGRA_EXT),
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

//! constructor for extra textures
VOpenGLTexture::VOpenGLTexture(VImage* origImage, const VPath& name, const TextureFlags_t & flags)
    : VTexture(name), m_ColorFormat(origImage->getColorFormat()), Image(0), MipImage(0),
    TextureName(0), TargetType(0), InternalFormat(GL_RGBA), PixelFormat(GL_BGRA_EXT),
    PixelType(GL_UNSIGNED_BYTE), MipLevelStored(0), MipmapLegacyMode(true),
    IsRenderTarget(false), AutomaticMipmapUpdate(false),
    ReadOnlyLock(false), KeepImage(true)
{
    const VString ext = name.extension().toLower();
    bool UseSRGB = flags & TEXTUREFLAG_USE_SRGB;
    HasMipMaps = !(flags & TEXTUREFLAG_NO_MIPMAPS);
    VOpenGLTexture texture;
    int width = 0;
    int height = 0;
    int mipCount = 0;
    if ( name == NULL || origImage == NULL)
    {
        // can't load anything from an empty buffer
    }
    else if (	ext == "jpg" || ext == "tga" ||
                ext == "png" || ext == "bmp" ||
                ext == "psd" || ext == "gif" ||
                ext == "hdr" || ext == "pic" )
    {
        if ( origImage != NULL )
        {
            const size_t dataSize = GetTextureSize( origImage->getColorFormat(), origImage->getDimension().Width, origImage->getDimension().Height );
            texture = CreateGlTexture(origImage->getColorFormat(), width, height, origImage->lock(), dataSize,
                    1 /* one mip level */, flags & TEXTUREFLAG_USE_SRGB, false );
            if ( HasMipMaps )
            {
                glBindTexture( GL_TEXTURE_2D, texture.TextureName );
                glGenerateMipmap( GL_TEXTURE_2D );
                glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
            }
        }
    }
    else if ( ext == "pvr" )
    {
        if (HasMipMaps)
            mipCount = origImage->getInfo()["mipCount"].toInt();
        if (origImage->getInfo()["NumFaces"] == "1")
            texture = CreateGlTexture(origImage->getColorFormat(), width, height, origImage->lock(), origImage->getLength(), mipCount, UseSRGB, false );
        else if(origImage->getInfo()["NumFaces"] == "6")
            texture = CreateGlCubeTexture(origImage->getColorFormat(), width, height, origImage->lock(), origImage->getLength(), mipCount, UseSRGB, false );
    }
    else if ( ext == "ktx" )
    {
        if (HasMipMaps)
            mipCount = origImage->getInfo()["mipCount"].toInt();
        if (origImage->getInfo()["numberOfFaces"] == "1")
            texture = CreateGlTexture(origImage->getColorFormat(), width, height, origImage->lock(), origImage->getLength(), mipCount, UseSRGB, false );
        else if (origImage->getInfo()["numberOfFaces"] == "6")
            texture = CreateGlCubeTexture(origImage->getColorFormat(), width, height, origImage->lock(), origImage->getLength(), mipCount, UseSRGB, false );
    }
    else if ( ext == "pkm" )
    {
        vInfo("PKM format not supported");
    }
    else
    {
        vInfo("unsupported file extension " << ext.toCString());
    }

    // Create a default texture if the load failed
    if ( texture.TextureName == 0 )
    {
        vWarn("Failed to load ");
        if ( ( flags & TEXTUREFLAG_NO_DEFAULT ) == 0 )
        {
            static uint8_t defaultTexture[8 * 8 * 3] =
            {
                    255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255,
                    255,255,255,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64, 255,255,255,
                    255,255,255,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64, 255,255,255,
                    255,255,255,  64, 64, 64,  64, 64, 64, 255,255,255, 255,255,255,  64, 64, 64,  64, 64, 64, 255,255,255,
                    255,255,255,  64, 64, 64,  64, 64, 64, 255,255,255, 255,255,255,  64, 64, 64,  64, 64, 64, 255,255,255,
                    255,255,255,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64, 255,255,255,
                    255,255,255,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64,  64, 64, 64, 255,255,255,
                    255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255, 255,255,255
            };
            texture = LoadRGBTextureFromMemory( defaultTexture, 8, 8, flags & TEXTUREFLAG_USE_SRGB );
        }
    }

    return texture;


}

//! constructor for simple GLTexture
VOpenGLTexture::VOpenGLTexture()
    : VTexture(0), ColorFormat(ECF_A8R8G8B8), Image(0), MipImage(0),
    TextureName(0), TargetType(0), InternalFormat(GL_RGBA), PixelFormat(GL_BGRA_EXT),
    PixelType(GL_UNSIGNED_BYTE), MipLevelStored(0), HasMipMaps(true),
    MipmapLegacyMode(true), IsRenderTarget(false), AutomaticMipmapUpdate(false),
    ReadOnlyLock(false), KeepImage(true)
{

}

//! constructor for basic setup (only for derived classes)
VOpenGLTexture::VOpenGLTexture(const VPath& name)
    : VTexture(name), ColorFormat(ECF_A8R8G8B8), Image(0), MipImage(0),
    TextureName(0), TargetType(0), InternalFormat(GL_RGBA), PixelFormat(GL_BGRA_EXT),
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

static int32_t GetTextureSize( const int format, const int w, const int h )
{
    switch ( format & ECF_TypeMask )
    {
        case ECF_R:            return w*h;
        case ECF_RGB:          return w*h*3;
        case ECF_RGBA:         return w*h*4;
        case ECF_ATC_RGB:
        case ECF_ETC1:
        case ECF_ETC2_RGB:
        case ECF_DXT1:
        {
            int bw = (w+3)/4, bh = (h+3)/4;
            return bw * bh * 8;
        }
        case ECF_ATC_RGBA:
        case ECF_ETC2_RGBA:
        case ECF_DXT3:
        case ECF_DXT5:
        {
            int bw = (w+3)/4, bh = (h+3)/4;
            return bw * bh * 16;
        }
        case ECF_PVR4bRGB:
        case ECF_PVR4bRGBA:
        {
            unsigned int width = (unsigned int)w;
            unsigned int height = (unsigned int)h;
            unsigned int min_width = 8;
            unsigned int min_height = 8;

            // pad the dimensions
            width = width + ((-1*width) % min_width);
            height = height + ((-1*height) % min_height);
            unsigned int depth = 1;

            unsigned int bpp = 4;
            unsigned int bits = bpp * width * height * depth;
            return (int)(bits / 8);
        }
        case ECF_ASTC_4x4:
        {
            int blocksWide = ( w + 3 ) / 4;
            int blocksHigh = ( h + 3 ) / 4;
            return blocksWide * blocksHigh * 16;
        }
        case ECF_ASTC_6x6:
        {
            int blocksWide = ( w + 5 ) / 6;
            int blocksHigh = ( h + 5 ) / 6;
            return blocksWide * blocksHigh * 16;
        }
        default:
        {
            vAssert( false );
            break;
        }
    }
    return 0;
}

static VOpenGLTexture CreateGlTexture(const int format, const int width, const int height,
                        const void * data, const size_t dataSize,
                        const int mipcount, const bool useSrgbFormat, const bool imageSizeStored )
{
    VOpenGLTexture texture;
    texture.TargetType = GL_TEXTURE_2D;
    VGlOperation glOperation;
    // vInfo("CreateGLTexture(): format " << NameForTextureFormat( static_cast< TextureFormat >( format ) ));

    GLenum glFormat;
    GLenum glInternalFormat;
    if ( !TextureFormatToGlFormat( format, useSrgbFormat, glFormat, glInternalFormat ) )
    {
        return texture;
    }

    if ( mipcount <= 0 )
    {
        vInfo(": Invalid mip count " << mipcount);
        return texture;
    }

    // larger than this would require mipSize below to be a larger type
    if ( width <= 0 || width > 32768 || height <= 0 || height > 32768 )
    {
        vInfo(": Invalid texture size (" << width << "x" << height << ")");
        return texture;
    }

    glGenTextures( 1, &texture.TextureName );
    glBindTexture( GL_TEXTURE_2D, texture.TextureName );

    const unsigned char * level = (const unsigned char*)data;
    const unsigned char * endOfBuffer = level + dataSize;

    int w = width;
    int h = height;
    for ( int i = 0; i < mipcount; i++ )
    {
        int32_t mipSize = GetTextureSize( format, w, h );
        if ( imageSizeStored )
        {
            mipSize = *(const size_t *)level;

            level += 4;
            if ( level > endOfBuffer )
            {
                vInfo(": Image data exceeds buffer size");
                glBindTexture( GL_TEXTURE_2D, 0 );
                return texture;
            }
        }

        if ( mipSize <= 0 || mipSize > endOfBuffer - level )
        {
            vInfo(": Mip level " << i << " exceeds buffer size (" << mipSize << " > " << (endOfBuffer - level) << ")");
            glBindTexture( GL_TEXTURE_2D, 0 );
            return texture;
        }

        if ( format & ECF_Compressed )
        {
            glCompressedTexImage2D( GL_TEXTURE_2D, i, glInternalFormat, w, h, 0, mipSize, level );
            glOperation.logErrorsEnum( "ECF_Compressed" );
        }
        else
        {
            glTexImage2D( GL_TEXTURE_2D, i, glInternalFormat, w, h, 0, glFormat, GL_UNSIGNED_BYTE, level );
        }

        level += mipSize;
        if ( imageSizeStored )
        {
            level += 3 - ( ( mipSize + 3 ) % 4 );
            if ( level > endOfBuffer )
            {
                vInfo(": Image data exceeds buffer size");
                glBindTexture( GL_TEXTURE_2D, 0 );
                return texture;
            }
        }

        w >>= 1;
        h >>= 1;
        if ( w < 1 ) { w = 1; }
        if ( h < 1 ) { h = 1; }
    }

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    // Surfaces look pretty terrible without trilinear filtering
    if ( mipcount <= 1 )
    {
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    }
    else
    {
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    }
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    glOperation.logErrorsEnum( "Texture load" );

    glBindTexture( GL_TEXTURE_2D, 0 );

    return texture;

}

static VOpenGLTexture CreateGlCubeTexture(const int format, const int width, const int height,
                        const void * data, const size_t dataSize,
                        const int mipcount, const bool useSrgbFormat, const bool imageSizeStored )
{
    VOpenGLTexture texture;
    texture.TargetType = GL_TEXTURE_CUBE_MAP;
    VGlOperation glOperation;
    assert( width == height );

    if ( mipcount <= 0 )
    {
        vInfo("Invalid mip count " << mipcount);
        return texture;
    }

    // larger than this would require mipSize below to be a larger type
    if ( width <= 0 || width > 32768 || height <= 0 || height > 32768 )
    {
        vInfo("Invalid texture size (" << width << "x" << height << ")");
        return texture;
    }

    GLenum glFormat;
    GLenum glInternalFormat;
    if ( !TextureFormatToGlFormat( format, useSrgbFormat, glFormat, glInternalFormat ) )
    {
        return texture;
    }

    glGenTextures( 1, &texture.TextureName );
    glBindTexture( GL_TEXTURE_CUBE_MAP, texture.TextureName );

    const unsigned char * level = (const unsigned char*)data;
    const unsigned char * endOfBuffer = level + dataSize;

    for ( int i = 0; i < mipcount; i++ )
    {
        const int w = width >> i;
        int32_t mipSize = GetTextureSize( format, w, w );
        if ( imageSizeStored )
        {
            mipSize = *(const size_t *)level;
            level += 4;
            if ( level > endOfBuffer )
            {
                vInfo("Image data exceeds buffer size");
                glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );
                return texture;
            }
        }

        for ( int side = 0; side < 6; side++ )
        {
            if ( mipSize <= 0 || mipSize > endOfBuffer - level )
            {
                vInfo("Mip level " << i << " exceeds buffer size (" << mipSize << " > " << endOfBuffer - level << ")");
                glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );
                return texture;
            }

            if ( format & Texture_Compressed )
            {
                glCompressedTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, i, glInternalFormat, w, w, 0, mipSize, level );
            }
            else
            {
                glTexImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, i, glInternalFormat, w, w, 0, glFormat, GL_UNSIGNED_BYTE, level );
            }

            level += mipSize;
            if ( imageSizeStored )
            {
                level += 3 - ( ( mipSize + 3 ) % 4 );
                if ( level > endOfBuffer )
                {
                    vInfo("Image data exceeds buffer size");
                    glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );
                    return texture;
                }
            }
        }
    }

    // Surfaces look pretty terrible without trilinear filtering
    if ( mipcount <= 1 )
    {
        glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    }
    else
    {
        glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    }
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    glOperation.logErrorsEnum( "Texture load" );

    glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );

    return texture;
}

static bool TextureFormatToGlFormat( const int format, const bool useSrgbFormat, GLenum & glFormat, GLenum & glInternalFormat )
{
    switch ( format & ECF_TypeMask )
    {
        case ECF_RGB:
        {
            glFormat = GL_RGB;
            if ( useSrgbFormat )
            {
                glInternalFormat = GL_SRGB8;
//				vInfo("GL texture format is GL_RGB / GL_SRGB8");
            }
            else
            {
                glInternalFormat = GL_RGB;
//				vInfo("GL texture format is GL_RGB / GL_RGB");
            }
            return true;
        }
        case ECF_RGBA:
        {
            glFormat = GL_RGBA;
            if ( useSrgbFormat )
            {
                glInternalFormat = GL_SRGB8_ALPHA8;
//				vInfo("GL texture format is GL_RGBA / GL_SRGB8_ALPHA8");
            }
            else
            {
                glInternalFormat = GL_RGBA;
//				vInfo("GL texture format is GL_RGBA / GL_RGBA");
            }
            return true;
        }
        case ECF_R:
        {
            glInternalFormat = GL_R8;
            glFormat = GL_RED;
//			vInfo("GL texture format is GL_R8");
            return true;
        }
        case ECF_DXT1:
        {
            glFormat = glInternalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
//			vInfo("GL texture format is GL_COMPRESSED_RGBA_S3TC_DXT1_EXT");
            return true;
        }
    // unsupported on OpenGL ES:
    //    case Texture_DXT3:  glFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT; break;
    //    case Texture_DXT5:  glFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT; break;
        case ECF_PVR4bRGB:
        {
            glFormat = GL_RGB;
            glInternalFormat = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
//			vInfo("GL texture format is GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG");
            return true;
        }
        case ECF_PVR4bRGBA:
        {
            glFormat = GL_RGBA;
            glInternalFormat = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
//			vInfo("GL texture format is GL_RGBA / GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG");
            return true;
        }
        case ECF_ETC1:
        {
            glFormat = GL_RGB;
            if ( useSrgbFormat )
            {
                // Note that ETC2 is backwards compatible with ETC1.
                glInternalFormat = GL_COMPRESSED_SRGB8_ETC2;
//				vInfo("GL texture format is GL_RGB / GL_COMPRESSED_SRGB8_ETC2");
            }
            else
            {
                glInternalFormat = GL_ETC1_RGB8_OES;
//				vInfo("GL texture format is GL_RGB / GL_ETC1_RGB8_OES");
            }
            return true;
        }
        case ECF_ETC2_RGB:
        {
            glFormat = GL_RGB;
            if ( useSrgbFormat )
            {
                glInternalFormat = GL_COMPRESSED_SRGB8_ETC2;
//				vInfo("GL texture format is GL_RGB / GL_COMPRESSED_SRGB8_ETC2");
            }
            else
            {
                glInternalFormat = GL_COMPRESSED_RGB8_ETC2;
//				vInfo("GL texture format is GL_RGB / GL_COMPRESSED_RGB8_ETC2");
            }
            return true;
        }
        case ECF_ETC2_RGBA:
        {
            glFormat = GL_RGBA;
            if ( useSrgbFormat )
            {
                glInternalFormat = GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC;
//				vInfo("GL texture format is GL_RGBA / GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC");
            }
            else
            {
                glInternalFormat = GL_COMPRESSED_RGBA8_ETC2_EAC;
//				vInfo("GL texture format is GL_RGBA / GL_COMPRESSED_RGBA8_ETC2_EAC");
            }
            return true;
        }
        case ECF_ASTC_4x4:
        {
            glFormat = GL_RGBA;
            glInternalFormat = GL_COMPRESSED_RGBA_ASTC_4x4_KHR;
            return true;
        }
        case ECF_ASTC_6x6:
        {
            glFormat = GL_RGBA;
            glInternalFormat = GL_COMPRESSED_RGBA_ASTC_6x6_KHR;
            return true;
        }
        case ECF_ATC_RGB:
        {
            glFormat = GL_RGB;
            glInternalFormat = GL_ATC_RGB_AMD;
//			vInfo("GL texture format is GL_RGB / GL_ATC_RGB_AMD");
            return true;
        }
        case ECF_ATC_RGBA:
        {
            glFormat = GL_RGBA;
            glInternalFormat = GL_ATC_RGBA_EXPLICIT_ALPHA_AMD;
//			vInfo("GL texture format is GL_RGBA / GL_ATC_RGBA_EXPLICIT_ALPHA_AMD");
            return true;
        }
    }
    return false;
}

VOpenGLTexture LoadRGBTextureFromMemory( const uint8_t * texture, const int width, const int height, const bool useSrgbFormat )
{
    const size_t dataSize = GetTextureSize( ECF_RGB, width, height );
    return CreateGlTexture(ECF_RGB, width, height, texture, dataSize, 1, useSrgbFormat, false );
}

}
