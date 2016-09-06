#include "VPainter.h"

NV_NAMESPACE_BEGIN

struct VPainter::Private
{
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

const VMatrix4f &VPainter::viewMatrix() const
{
    return d->viewMatrix;
}

void VPainter::setViewMatrix(const VMatrix4f &viewMatrix)
{
    d->viewMatrix = viewMatrix;
}

NV_NAMESPACE_END
