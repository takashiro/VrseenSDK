#include "VImageManager.h"

#include "VFile.h"
#include "VZipFile.h"
#include "VBuffer.h"
#include "VImageCommonLoader.h"
#include "VImageKtxLoader.h"
#include "VImagePvrLoader.h"
#include "VLog.h"

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

VImage *VImageManager::loadImage(const VPath &filePath) const
{
    VIODevice *device = nullptr;

    VFile *file = new VFile(filePath, VIODevice::ReadOnly);
    if (!file->exists()) {
        delete file;

        const VZipFile &apk = VZipFile::CurrentApkFile();
        if (apk.contains(filePath)) {
            VBuffer *buffer = new VBuffer;
            if (apk.read(filePath, buffer)) {
                device = buffer;
            } else {
                delete buffer;
            }
        }
    } else {
        device = file;
    }

    if (device) {
        for (VImageLoader *loader : d->loaders) {
            if (loader->isValid(filePath)) {
                device->seek(0);
                return loader->load(device);
            }
        }
        delete device;
    }

    vWarn("Failed to load image!");
    return 0;
}

NV_NAMESPACE_END
