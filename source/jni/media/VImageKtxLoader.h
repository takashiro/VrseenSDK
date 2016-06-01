#ifndef VIMAGEKTXLOADER_H
#define VIMAGEKTXLOADER_H

#include "VImageLoader.h"

namespace NervGear
{

    class VImageKtxLoader : public VImageLoader
    {
    public:


        virtual bool isValid(const VPath& filename) const;

        virtual bool isValid(VIODevice *file) const;

        virtual VImage* load(VIODevice *file) const;

    };

}

#endif // VIMAGEKTXLOADER_H

