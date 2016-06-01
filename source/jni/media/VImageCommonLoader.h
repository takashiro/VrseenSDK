#ifndef VIMAGECOMMONLOADER_H
#define VIMAGECOMMONLOADER_H

#include "VImageLoader.h"


namespace NervGear
{

    class VImageCommonLoader : public VImageLoader
    {
    public:


        virtual bool isValid(const VPath &fileName) const;

        virtual bool isValid(VIODevice *file) const;

        virtual VImage *load(VIODevice *file) const;

    };

}

#endif // VIMAGECOMMONLOADER_H

