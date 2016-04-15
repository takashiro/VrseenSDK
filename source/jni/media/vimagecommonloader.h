#ifndef VIMAGECOMMONLOADER_H
#define VIMAGECOMMONLOADER_H

#include "VImageLoader.h"


namespace NervGear
{

    class VImageCommonLoader : public VImageLoader
    {
    public:

        VImageCommonLoader();

        virtual ~VImageCommonLoader();

        virtual bool isALoadableFileExtension(const VPath& filename) const;

        virtual bool isALoadableFileFormat(VFile* file) const;

        virtual std::shared_ptr<VImage> loadImage(VFile* file) const;

    };

}

#endif // VIMAGECOMMONLOADER_H

