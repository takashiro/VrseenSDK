#ifndef VIMAGEKTXLOADER_H
#define VIMAGEKTXLOADER_H

#include "VImageLoader.h"

namespace NervGear
{

    class VImageKtxLoader : public VImageLoader
    {
    public:

        VImageKtxLoader();

        virtual ~VImageKtxLoader();

        virtual bool isALoadableFileExtension(const VPath& filename) const;

        virtual bool isALoadableFileFormat(VFile* file) const;

        virtual VImage* loadImage(VFile* file) const;

    };

}

#endif // VIMAGEKTXLOADER_H

