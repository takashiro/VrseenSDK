#include "VResource.h"
#include "VByteArray.h"
#include "VZipFile.h"
#include "VPath.h"

#include "App.h"

NV_NAMESPACE_BEGIN

struct VResource::Private
{
    VPath path;
    bool exists;
    VByteArray data;

    void load()
    {
        const VZipFile &apk = vApp->apkFile();
        exists = apk.contains(path);
        if (exists) {
            data = apk.read(path);
        }
    }
};

VResource::VResource(const VPath &path)
    : d(new Private)
{
    d->path = path;
    d->load();
}

VResource::VResource(const char *path)
    : d(new Private)
{
    d->path = path;
    d->load();
}

VResource::~VResource()
{
    delete d;
}

bool VResource::exists() const
{
    return d->exists;
}

const VPath &VResource::path() const
{
    return d->path;
}

VByteArray VResource::data() const
{
    return d->data;
}

uint VResource::size() const
{
    return d->data.size();
}

int VResource::length() const
{
    return (int) d->data.length();
}

NV_NAMESPACE_END
