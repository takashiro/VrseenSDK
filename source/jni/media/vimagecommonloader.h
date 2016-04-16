#ifndef VIMAGECOMMONLOADER_H
#define VIMAGECOMMONLOADER_H

#include "VImageLoader.h"


namespace NervGear
{

    class VImageCommonLoader : public VImageLoader
    {
    public:


        virtual bool isALoadableFileExtension(const VPath& filename) const;

        virtual bool isALoadableFileFormat(VFile* file) const;

        virtual VImage* loadImage(VFile* file) const;

    };

}

#endif // VIMAGECOMMONLOADER_H

