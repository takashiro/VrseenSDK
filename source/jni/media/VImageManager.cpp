#include "VImageManager.h"
#include "VIODevice.h"

namespace NervGear {

VImageManager::VImageManager()
{
    ImageLoaderList.push_back(new VImageCommonLoader());
    ImageLoaderList.push_back(new VImageKtxLoader());
    ImageLoaderList.push_back(new VImagePvrLoader());
}

VImageManager::~VImageManager()
{
    for (int i = 0; i < (int)ImageLoaderList.size(); i++)
    {
        delete ImageLoaderList[i];
    }
    ImageLoaderList.clear();
}

VImage* VImageManager::loadImage(const VPath &filename) const
{
    VFile* file =  new VFile(filename, VIODevice::ReadOnly);
    if (!file)
    {
        vInfo("Error File doesn't exist!");
        return 0;
    }

    for (int i = 0; i < (int)ImageLoaderList.size(); i++)
    {
        if (ImageLoaderList[i]->isALoadableFileExtension(filename))
        {
            file->seek(0);
            return ImageLoaderList[i]->loadImage(file);
        }
    }

    delete file;

    vInfo("Failed to load image!");

    return 0;





}



}
