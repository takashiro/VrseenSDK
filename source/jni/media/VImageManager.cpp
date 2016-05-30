#include "VImageManager.h"

#include "VFile.h"
#include "VImageCommonLoader.h"
#include "VImageKtxLoader.h"
#include "VImagePvrLoader.h"

NV_NAMESPACE_BEGIN

struct VImageManager::Private
{
    VArray<VImageLoader *> loaders;
};

VImageManager::VImageManager()
    : d(new Private)
{
    d->loaders.append(new VImageCommonLoader);
    d->loaders.append(new VImageKtxLoader);
    d->loaders.append(new VImagePvrLoader);
}

VImageManager::~VImageManager()
{
    for (VImageLoader *loader : d->loaders) {
        delete loader;
    }
    delete d;
}

VImage *VImageManager::loadImage(const VPath &fileName) const
{
    VFile file(fileName, VIODevice::ReadOnly);
    if (!file.exists()) {
        vInfo("Error File doesn't exist!");
        return 0;
    }

    for (VImageLoader *loader : d->loaders) {
        if (loader->isALoadableFileExtension(fileName)) {
            file.seek(0);
            return loader->loadImage(&file);
        }
    }

    vWarn("Failed to load image!");
    return 0;
}

NV_NAMESPACE_END
