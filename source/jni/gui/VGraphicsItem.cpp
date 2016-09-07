#include "VGraphicsItem.h"
#include "VArray.h"
#include "VTimer.h"
#include "VClickEvent.h"

NV_NAMESPACE_BEGIN

struct VGraphicsItem::Private
{
    VGraphicsItem *parent;
    VArray<VGraphicsItem *> children;
    VRect3f boundingRect;
    VVect3f pos;
    bool hasFocus;
    double focusTimestamp;
    double stareElapsedTime;
    bool clicked;
    VMatrix4f transform;
    bool visible;

    Private()
        : hasFocus(false)
        , focusTimestamp(0.0)
        , stareElapsedTime(2.0)
        , clicked(false)
        , visible(true)
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

bool VGraphicsItem::isVisible() const
{
    return d->visible;
}

void VGraphicsItem::setVisible(bool visible)
{
    d->visible = visible;
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

const VVect3f &VGraphicsItem::pos() const
{
    return d->pos;
}

void VGraphicsItem::setPos(const VVect3f &pos)
{
    d->pos = pos;
    updateTransform();
}

VVect3f VGraphicsItem::globalPos() const
{
    VVect3f pos = d->pos;
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

double VGraphicsItem::stareElapsedTime() const
{
    return d->stareElapsedTime;
}

void VGraphicsItem::setStareElapsedTime(double elapsed)
{
    d->stareElapsedTime = elapsed;
}

const VMatrix4f &VGraphicsItem::transform() const
{
    return d->transform;
}

void VGraphicsItem::updateTransform()
{
    for (VGraphicsItem *child : d->children) {
        child->updateTransform();
    }

    const VRect3f &rect = boundingRect();
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

    d->transform = VMatrix4f::Translation(globalPos()) * VMatrix4f::Translation(center) * VMatrix4f::RotationY(rotateAngle) * VMatrix4f::Scaling(widthScale, heightScale, 1.0f);
}

void VGraphicsItem::setBoundingRect(const VRect3f &rect)
{
    d->boundingRect = rect;
    updateTransform();
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
        if (child->d->visible) {
            child->paint(painter);
        }
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

void VGraphicsItem::onStare()
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
                if (now - d->focusTimestamp >= d->stareElapsedTime) {
                    d->clicked = true;
                    onStare();
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

bool VGraphicsItem::hasFocus() const
{
    return d->hasFocus;
}

void VGraphicsItem::onClick(const VClickEvent &event)
{
    NV_UNUSED(event);
}

void VGraphicsItem::onKeyEvent(const VClickEvent &event)
{
    for (VGraphicsItem *child : d->children) {
        child->onKeyEvent(event);
    }
    if (hasFocus()) {
        onClick(event);
    }
}

NV_NAMESPACE_END
