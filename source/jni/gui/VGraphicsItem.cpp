#include "VGraphicsItem.h"
#include "VArray.h"
#include "VTimer.h"

NV_NAMESPACE_BEGIN

struct VGraphicsItem::Private
{
    VGraphicsItem *parent;
    VArray<VGraphicsItem *> children;
    VRect3f boundingRect;
    VPosf pos;
    bool hasFocus;
    double focusTimestamp;
    double clickElapsedTime;
    bool clicked;

    Private()
        : hasFocus(false)
        , focusTimestamp(0.0)
        , clickElapsedTime(2.0)
        , clicked(false)
    {
    }
};

VGraphicsItem::VGraphicsItem(VGraphicsItem *parent)
    : d(new Private)
{
    d->parent = parent;
    if (parent) {
        parent->addChild(this);
    }
}

VGraphicsItem::~VGraphicsItem()
{
    for (VGraphicsItem *child : d->children) {
        child->setParent(nullptr);
        delete child;
    }
    if (d->parent) {
        d->parent->removeChild(this);
    }
    delete d;
}

VGraphicsItem *VGraphicsItem::parent() const
{
    return d->parent;
}

void VGraphicsItem::addChild(VGraphicsItem *child)
{
    child->setParent(this);
    d->children.append(child);
}

void VGraphicsItem::removeChild(VGraphicsItem *child)
{
    child->setParent(nullptr);
    d->children.removeOne(child);
}

VPosf VGraphicsItem::pos() const
{
    return d->pos;
}

void VGraphicsItem::setPos(const VPosf &pos)
{
    d->pos = pos;
}

VPosf VGraphicsItem::globalPos() const
{
    VPosf pos = d->pos;
    VGraphicsItem *parent = d->parent;
    while (parent) {
        pos += parent->pos();
        parent = parent->parent();
    }
    return pos;
}

VRect3f VGraphicsItem::boundingRect() const
{
    if (d->children.isEmpty()) {
        return d->boundingRect;
    }
    //TODO: calculate the bounding rect of all its children
    return VRect3f();
}

double VGraphicsItem::clickElapsedTime() const
{
    return d->clickElapsedTime;
}

void VGraphicsItem::setClickElapsedTime(double elapsed)
{
    d->clickElapsedTime = elapsed;
}

void VGraphicsItem::setBoundingRect(const VRect3f &rect)
{
    d->boundingRect = rect;
}

void VGraphicsItem::init(void *vg)
{
    for (VGraphicsItem *child : d->children) {
        child->init(vg);
    }
}

void VGraphicsItem::paint(VPainter *painter)
{
    for (VGraphicsItem *child : d->children) {
        child->paint(painter);
    }
}

void VGraphicsItem::setParent(VGraphicsItem *parent)
{
    d->parent = parent;
}

void VGraphicsItem::onFocus()
{
}

void VGraphicsItem::onBlur()
{
}

void VGraphicsItem::onClick()
{
}

void VGraphicsItem::onSensorChanged(const VMatrix4f &mvp)
{
    for (VGraphicsItem *child : d->children) {
        child->onSensorChanged(mvp);
    }

    VRect3f rect = boundingRect();
    VVect3f start = mvp.transform(rect.start);
    VVect3f end = mvp.transform(rect.end);

    bool hovered = start.x <= 0 && start.y <= 0 && end.x >= 0 && end.y >= 0;
    if (hovered) {
        if (!d->hasFocus) {
            d->hasFocus = true;
            d->focusTimestamp = VTimer::Seconds();
            onFocus();
        } else {
            if (!d->clicked) {
                double now = VTimer::Seconds();
                if (now - d->focusTimestamp >= d->clickElapsedTime) {
                    d->clicked = true;
                    onClick();
                }
            }
        }
    } else {
        if (d->hasFocus) {
            d->hasFocus = false;
            d->clicked = false;
            onBlur();
        }
    }
}

NV_NAMESPACE_END
