#ifndef VIMAGE_H
#define VIMAGE_H
#include "VReferenceCounted.h"
#include "VImageColor.h"
#include "VRect.h"

namespace  NervGear
{
class VImage : public virtual VReferenceCounted
{
public:

    //! Lock function. Use this to get a pointer to the image data.
    /** After you don't need the pointer anymore, you must call unlock().
    \return Pointer to the image data. What type of data is pointed to
    depends on the color format of the image. For example if the color
    format is ECF_A8R8G8B8, it is of uint. Be sure to call unlock() after
    you don't need the pointer any more. */
    virtual void* lock() = 0;

    //! Unlock function.
    /** Should be called after the pointer received by lock() is not
    needed anymore. */
    virtual void unlock() = 0;

    //! Returns width and height of image data.
    virtual const VDimension<uint>& getDimension() const = 0;

    //! Returns bits per pixel.
    virtual uint getBitsPerPixel() const = 0;

    //! Returns bytes per pixel
    virtual uint getBytesPerPixel() const = 0;

    //! Returns image data size in bytes
    virtual uint getImageDataSizeInBytes() const = 0;

    //! Returns image data size in pixels
    virtual uint getImageDataSizeInPixels() const = 0;

    //! Returns a pixel
    virtual VImageColor getPixel(uint x, uint y) const = 0;

    //! Sets a pixel
    virtual void setPixel(uint x, uint y, const VImageColor &color, bool blend = false ) = 0;

    //! Returns the color format
    virtual ECOLOR_FORMAT getColorFormat() const = 0;

    //! Returns mask for red value of a pixel
    virtual uint getRedMask() const = 0;

    //! Returns mask for green value of a pixel
    virtual uint getGreenMask() const = 0;

    //! Returns mask for blue value of a pixel
    virtual uint getBlueMask() const = 0;

    //! Returns mask for alpha value of a pixel
    virtual uint getAlphaMask() const = 0;

    //! Returns pitch of image
    virtual uint getPitch() const =0;

    //! Copies the image into the target, scaling the image to fit
    virtual void copyToScaling(void* target, uint width, uint height, ECOLOR_FORMAT format=ECF_A8R8G8B8, uint pitch=0) =0;

    //! Copies the image into the target, scaling the image to fit
    virtual void copyToScaling(VImage* target) =0;

    //! copies this surface into another, scaling it to fit, appyling a box filter
    virtual void copyToScalingBoxFilter(VImage* target, int bias = 0, bool blend = false) = 0;

    //! fills the surface with given color
    virtual void fill(const VImageColor &color) =0;

    //! get the amount of Bits per Pixel of the given color format
    static uint getBitsPerPixelFromFormat(const ECOLOR_FORMAT format)
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
    static bool isRenderTargetOnlyFormat(const ECOLOR_FORMAT format)
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

};

class CImage : public VImage
{
public:

    //! constructor from raw image data
    /** \param useForeignMemory: If true, the image will use the data pointer
    directly and own it from now on, which means it will also try to delete [] the
    data when the image will be destructed. If false, the memory will by copied. */
    CImage(ECOLOR_FORMAT format, const VDimension<uint>& size,
        void* data, bool ownForeignMemory=true, bool deleteMemory = true);

    //! constructor for empty image
    CImage(ECOLOR_FORMAT format, const VDimension<uint>& size);

    //! destructor
    virtual ~CImage();

    //! Lock function.
    virtual void* lock()
    {
        return Data;
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

    //! sets a pixel
    virtual void setPixel(uint x, uint y, const VImageColor &color, bool blend = false );

    //! returns the color format
    virtual ECOLOR_FORMAT getColorFormat() const;

    //! returns pitch of image
    virtual uint getPitch() const { return Pitch; }

    //! copies this surface into another, scaling it to fit.
    virtual void copyToScaling(void* target, uint width, uint height, ECOLOR_FORMAT format, uint pitch=0);

    //! copies this surface into another, scaling it to fit.
    virtual void copyToScaling(VImage* target);

    //! copies this surface into another, scaling it to fit, appyling a box filter
    virtual void copyToScalingBoxFilter(VImage* target, int bias = 0, bool blend = false);

    //! fills the surface with given color
    virtual void fill(const VImageColor &color);

private:

    //! assumes format and size has been set and creates the rest
    void initData();

    inline VImageColor getPixelBox ( int x, int y, int fx, int fy, int bias ) const;

    char* Data;
    VDimension<uint> Size;
    uint BytesPerPixel;
    uint Pitch;
    ECOLOR_FORMAT Format;

    bool DeleteMemory;
};


}

#endif // VIMAGE_H

