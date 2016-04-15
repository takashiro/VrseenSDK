#ifndef VIMAGEPVRLOADER_H
#define VIMAGEPVRLOADER_H


#include "VImageLoader.h"

namespace NervGear
{

    class VImagePvrLoader : public VImageLoader
    {
    public:

        VImagePvrLoader();

        virtual ~VImagePvrLoader();

        virtual bool isALoadableFileExtension(const VPath& filename) const;

        virtual bool isALoadableFileFormat(VFile* file) const;

        virtual std::shared_ptr<VImage> loadImage(VFile* file) const;

    };

}
#endif // VIMAGEPVRLOADER_H

