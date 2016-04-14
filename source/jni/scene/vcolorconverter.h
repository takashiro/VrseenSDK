#ifndef VCOLORCONVERTER_H
#define VCOLORCONVERTER_H

#include "VImage.h"


namespace NervGear {

class CColorConverter
{
    public:

    //! converts a monochrome bitmap to A1R5G5B5
    static void convert1BitTo16Bit(const char* in, short* out, int width, int height, int linepad=0, bool flip=false);

    //! converts a 4 bit palettized image to A1R5G5B5
    static void convert4BitTo16Bit(const char* in, short* out, int width, int height, const int* palette, int linepad=0, bool flip=false);

    //! converts a 8 bit palettized image to A1R5G5B5
    static void convert8BitTo16Bit(const char* in, short* out, int width, int height, const int* palette, int linepad=0, bool flip=false);

    //! converts a 8 bit palettized or non palettized image (A8) into R8G8B8
    static void convert8BitTo24Bit(const char* in, char* out, int width, int height, const char* palette, int linepad = 0, bool flip=false);

    //! converts a 8 bit palettized or non palettized image (A8) into A8R8G8B8
    static void convert8BitTo32Bit(const char* in, char* out, int width, int height, const char* palette, int linepad = 0, bool flip=false);

    //! converts R8G8B8 16 bit data to A1R5G5B5 data
    static void convert16BitTo16Bit(const short* in, short* out, int width, int height, int linepad=0, bool flip=false);

    //! copies R8G8B8 24 bit data to 24 data, and flips and
    //! mirrors the image during the process.
    static void convert24BitTo24Bit(const char* in, char* out, int width, int height, int linepad=0, bool flip=false, bool bgr=false);

    //! Resizes the surface to a new size and converts it at the same time
    //! to an A8R8G8B8 format, returning the pointer to the new buffer.
    static void convert16bitToA8R8G8B8andResize(const short* in, int* out, int newWidth, int newHeight, int currentWidth, int currentHeight);

    //! copies X8R8G8B8 32 bit data, and flips and
    //! mirrors the image during the process.
    static void convert32BitTo32Bit(const int* in, int* out, int width, int height, int linepad, bool flip=false);


    //! functions for converting one image format to another efficiently
    //! and hopefully correctly.
    //!
    //! \param sP pointer to source pixel data
    //! \param sN number of source pixels to copy
    //! \param dP pointer to destination data buffer. must be big enough
    //! to hold sN pixels in the output format.
    static void convert_A1R5G5B5toR8G8B8(const void* sP, int sN, void* dP);
    static void convert_A1R5G5B5toB8G8R8(const void* sP, int sN, void* dP);
    static void convert_A1R5G5B5toA8R8G8B8(const void* sP, int sN, void* dP);
    static void convert_A1R5G5B5toA1R5G5B5(const void* sP, int sN, void* dP);
    static void convert_A1R5G5B5toR5G6B5(const void* sP, int sN, void* dP);

    static void convert_A8R8G8B8toR8G8B8(const void* sP, int sN, void* dP);
    static void convert_A8R8G8B8toB8G8R8(const void* sP, int sN, void* dP);
    static void convert_A8R8G8B8toA8R8G8B8(const void* sP, int sN, void* dP);
    static void convert_A8R8G8B8toA1R5G5B5(const void* sP, int sN, void* dP);
    static void convert_A8R8G8B8toR5G6B5(const void* sP, int sN, void* dP);

    static void convert_A8R8G8B8toR3G3B2(const void* sP, int sN, void* dP);
    static void convert_R8G8B8toR8G8B8(const void* sP, int sN, void* dP);
    static void convert_R8G8B8toA8R8G8B8(const void* sP, int sN, void* dP);
    static void convert_R8G8B8toA1R5G5B5(const void* sP, int sN, void* dP);
    static void convert_R8G8B8toR5G6B5(const void* sP, int sN, void* dP);
    static void convert_B8G8R8toA8R8G8B8(const void* sP, int sN, void* dP);
    static void convert_B8G8R8A8toA8R8G8B8(const void* sP, int sN, void* dP);

    static void convert_R5G6B5toR5G6B5(const void* sP, int sN, void* dP);
    static void convert_R5G6B5toR8G8B8(const void* sP, int sN, void* dP);
    static void convert_R5G6B5toB8G8R8(const void* sP, int sN, void* dP);
    static void convert_R5G6B5toA8R8G8B8(const void* sP, int sN, void* dP);
    static void convert_R5G6B5toA1R5G5B5(const void* sP, int sN, void* dP);
    static void convert_viaFormat(const void* sP, ColorFormat sF, int sN,
                void* dP, ColorFormat dF);


};

}
#endif // VCOLORCONVERTER_H

