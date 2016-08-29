#include "VRectangle.h"

#define NANOVG_GLES3_IMPLEMENTATION
#include "3rdparty/nanovg/nanovg.h"

NV_NAMESPACE_BEGIN

struct VRectangle::Private
{
    VRect3f rect;
    VColor color;
};

VRectangle::VRectangle(VGraphicsItem *parent)
    : VGraphicsItem(parent)
    , d(new Private)
{
}

VRectangle::~VRectangle()
{
    delete d;
}

VRect3f VRectangle::rect() const
{
    return d->rect;
}

void VRectangle::setRect(const VRect3f &rect)
{
    d->rect = rect;
}

VColor VRectangle::color() const
{
    return d->color;
}

void VRectangle::setColor(const VColor &color)
{
    d->color = color;
}

void VRectangle::paint(VGuiPainter *painter)
{
    VPosf pos = globalPos();

    NVGcontext *vg = reinterpret_cast<NVGcontext *>(painter);
    nvgBeginPath(vg);
    nvgRect(vg, pos.x + d->rect.start.x, pos.y + d->rect.start.y, d->rect.end.x, d->rect.end.y);
    nvgFillColor(vg, nvgRGBA(d->color.red, d->color.green, d->color.blue, d->color.alpha));
    nvgFill(vg);
}

NV_NAMESPACE_END
