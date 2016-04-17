#include "VImageCommonLoader.h"
#include "3rdParty/stb/stb_image.h"

namespace NervGear {

    bool VImageCommonLoader::isALoadableFileExtension(const VPath &filename) const
    {
        const VString ext = filename.extension().toLower();

        if (    ext == "jpg" || ext == "tga" ||
                ext == "png" || ext == "bmp" ||
                ext == "psd" || ext == "gif" ||
                ext == "hdr" || ext == "pic" )
            return true;
        else
            return false;

    }

    bool VImageCommonLoader::isALoadableFileFormat(VFile *file) const
    {
         if (!file)
             return false;
         else
             return true;

    }

    VImage* VImageCommonLoader::loadImage(VFile *file) const
    {
        char* buffer = new char[file->size()];
        file->read(buffer, file->size());
        int comp;
        int width = 0;
        int height = 0;
        stbi_uc * image = stbi_load_from_memory( (unsigned char *)buffer, file->size(), &width, &height, &comp, 4 );
        VImage* m_image = new VImage(ECF_RGBA, VDimension<uint>(width, height), image, file->size());

        delete [] buffer;
        return m_image;
    }

}
