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
    VMatrix4f transform;

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

const VRect3f &VGraphicsItem::boundingRect() const
{
    return d->boundingRect;
}

double VGraphicsItem::clickElapsedTime() const
{
    return d->clickElapsedTime;
}

void VGraphicsItem::setClickElapsedTime(double elapsed)
{
    d->clickElapsedTime = elapsed;
}

const VMatrix4f &VGraphicsItem::transform() const
{
    return d->transform;
}

void VGraphicsItem::setBoundingRect(const VRect3f &rect)
{
    d->boundingRect = rect;

    const VVect3f size = rect.size();
    const VVect3f center = rect.start + size * 0.5f;
    const float	 screenHeight = size.y;
    const float screenWidth = std::max(size.x, size.z);
    float widthScale;
    float heightScale;
    float aspect = size.x / size.y;
    if (screenWidth / screenHeight > aspect) {
        // screen is wider than movie, clamp size to height
        heightScale = screenHeight * 0.5f;
        widthScale = heightScale * aspect;
    } else {
        // screen is taller than movie, clamp size to width
        widthScale = screenWidth * 0.5f;
        heightScale = widthScale / aspect;
    }

    const float rotateAngle = (size.x > size.z) ? 0.0f : (float) M_PI * 0.5f;

    d->transform = VMatrix4f::Translation(center) * VMatrix4f::RotationY(rotateAngle) * VMatrix4f::Scaling(widthScale, heightScale, 1.0f);
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
