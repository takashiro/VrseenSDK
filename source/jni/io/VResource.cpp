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
};

VResource::VResource(const VPath &path)
    : d(new Private)
{
    d->path = path;

    const VZipFile &apk = vApp->apkFile();
    d->exists = apk.contains(path);
    if (d->exists) {
        d->data = apk.read(path);
    }
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
