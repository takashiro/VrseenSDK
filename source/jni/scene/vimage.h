#ifndef VIMAGE_H
#define VIMAGE_H

#include "VImageColor.h"
#include "VColorConverter.h"
#include "VRect.h"
#include "VJson.h"

namespace  NervGear
{
enum ImageFilter
{
    IMAGE_FILTER_NEAREST,
    IMAGE_FILTER_LINEAR,
    IMAGE_FILTER_CUBIC
};
class VImage
{
public:

    VImage(ColorFormat format, const VDimension<uint>& size,
        void* data, bool ownForeignMemory=true, bool deleteMemory = true);

    //! constructor for empty image
    VImage(ColorFormat format, const VDimension<uint>& size);

    VImage(ColorFormat format, const VDimension<uint>& size, void* data, uint length, VMap<VString, VString> &info);

    //! destructor
    virtual ~VImage();

    //! Lock function.
    virtual void* lock()
    {
        return m_data;
    }

    //! Unlock function.
    virtual void unlock() {}

    //! Returns width and height of image data.
    virtual const VDimension<uint>& getDimension() const;

    //! Returns bits per pixel.
    virtual uint getBitsPerPixel() const;

    //! Returns bytes per pixel
    virtual uint getBytesPerPixel() const;

    //! Returns image data size in bytes
    virtual uint getImageDataSizeInBytes() const;

    //! Returns image data size in pixels
    virtual uint getImageDataSizeInPixels() const;

    //! returns mask for red value of a pixel
    virtual uint getRedMask() const;

    //! returns mask for green value of a pixel
    virtual uint getGreenMask() const;

    //! returns mask for blue value of a pixel
    virtual uint getBlueMask() const;

    //! returns mask for alpha value of a pixel
    virtual uint getAlphaMask() const;

    //! returns a pixel
    virtual VImageColor getPixel(uint x, uint y) const;

    virtual uint getLength() const;

    //! sets a pixel
    virtual void setPixel(uint x, uint y, const VImageColor &color, bool blend = false );

    //! returns the color format
    virtual ColorFormat getColorFormat() const;

    //! returns pitch of image
    virtual uint getPitch() const { return m_pitch; }

    //! copies this surface into another, scaling it to fit.
    virtual void copyToScaling(void* target, uint width, uint height, ColorFormat format, uint pitch=0);

    //! copies this surface into another, scaling it to fit.
    virtual void copyToScaling(VImage* target);

    //! copies this surface into another
    virtual void copyTo(VImage* target, const V2Vect<int>& pos=V2Vect<int>(0,0));

    //! copies this surface into another
    virtual void copyTo(VImage* target, const V2Vect<int>& pos, const VRectangle<int>& sourceRect, const VRectangle<int>* clipRect=0);

    //! copies this surface into another, scaling it to fit, appyling a box filter
    virtual void copyToScalingBoxFilter(VImage* target, int bias = 0, bool blend = false);

    //! fills the surface with given color
    virtual void fill(const VImageColor &color);

    virtual VMap<VString, VString> getInfo();

    static uint getBitsPerPixelFromFormat(const ColorFormat format)
    {
        switch(format)
        {
        case ECF_A1R5G5B5:
            return 16;
        case ECF_R5G6B5:
            return 16;
        case ECF_R8G8B8:
            return 24;
        case ECF_A8R8G8B8:
            return 32;
        case ECF_R16F:
            return 16;
        case ECF_G16R16F:
            return 32;
        case ECF_A16B16G16R16F:
            return 64;
        case ECF_R32F:
            return 32;
        case ECF_G32R32F:
            return 64;
        case ECF_A32B32G32R32F:
            return 128;
        default:
            return 0;
        }
    }

    //! test if the color format is only viable for RenderTarget textures
    /** Since we don't have support for e.g. floating point VImage formats
    one should test if the color format can be used for arbitrary usage, or
    if it is restricted to RTTs. */
    static bool isRenderTargetOnlyFormat(const ColorFormat format)
    {
        switch(format)
        {
            case ECF_A1R5G5B5:
            case ECF_R5G6B5:
            case ECF_R8G8B8:
            case ECF_A8R8G8B8:
                return false;
            default:
                return true;
        }
    }

    static unsigned char * QuarterSize( const unsigned char * src, const int width, const int height, const bool srgb );
    static void WritePvrTexture( const char * fileName, const unsigned char * texture, int width, int height );
    static unsigned char * ScaleRGBA( const unsigned char * src, const int width, const int height, const int newWidth, const int newHeight, const ImageFilter filter );
private:

    //! assumes format and size has been set and creates the rest
    void initData();

    inline VImageColor getPixelBox ( int x, int y, int fx, int fy, int bias ) const;

    char* m_data;
    VDimension<uint> m_size;
    uint m_bytesPerPixel;
    uint m_pitch;
    ColorFormat m_format;
    uint m_length;
    VMap<VString, VString> m_info;

    bool DeleteMemory;
};

    //! get the amount of Bits per Pixel of the given color format



}

#endif // VIMAGE_H

