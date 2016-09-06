#include "VPainter.h"

NV_NAMESPACE_BEGIN

struct VPainter::Private
{
    void *nativeContext;
    VMatrix4f viewMatrix;
};

VPainter::VPainter()
    : d(new Private)
{
}

VPainter::~VPainter()
{
    delete d;
}

void *VPainter::nativeContext() const
{
    return d->nativeContext;
}

void VPainter::setNativeContext(void *context)
{
    d->nativeContext = context;
}

const VMatrix4f &VPainter::viewMatrix() const
{
    return d->viewMatrix;
}

void VPainter::setViewMatrix(const VMatrix4f &viewMatrix)
{
    d->viewMatrix = viewMatrix;
}

NV_NAMESPACE_END
