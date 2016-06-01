#ifndef VIMAGEPVRLOADER_H
#define VIMAGEPVRLOADER_H


#include "VImageLoader.h"

namespace NervGear
{

    class VImagePvrLoader : public VImageLoader
    {
    public:


        virtual bool isValid(const VPath& filename) const;

        virtual bool isValid(VIODevice *file) const;

        virtual VImage* load(VIODevice *file) const;

    };

}
#endif // VIMAGEPVRLOADER_H

