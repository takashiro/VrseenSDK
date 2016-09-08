#include "VGraphicsItem.h"
#include "VArray.h"
#include "VTimer.h"
#include "VTouchEvent.h"
#include "VKeyEvent.h"

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

    std::function<void()> focusListener;
    std::function<void()> blurListener;
    std::function<void()> stareListener;
    std::function<void(const VTouchEvent &)> touchListener;
    std::function<void(const VKeyEvent &)> keyPressListener;

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

    d->transform = VMatrix4f::Translation(globalPos()) * VMatrix4f::Translation(center) *  VMatrix4f::Scaling(widthScale, heightScale, 1.0f);
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

void VGraphicsItem::setOnFocusListener(const std::function<void()> &listener)
{
    d->focusListener = listener;
}

void VGraphicsItem::setOnBlurListener(const std::function<void()> &listener)
{
    d->blurListener = listener;
}

void VGraphicsItem::setOnStareListener(const std::function<void()> &listener)
{
    d->stareListener = listener;
}

void VGraphicsItem::setOnTouchListener(const std::function<void(const VTouchEvent &)> &listener)
{
    d->touchListener = listener;
}

void VGraphicsItem::setOnKeyPressListener(const std::function<void(const VKeyEvent &)> &listener)
{
    d->keyPressListener = listener;
}

void VGraphicsItem::onFocus()
{
    if (d->focusListener) {
        d->focusListener();
    }
}

void VGraphicsItem::onBlur()
{
    if (d->blurListener) {
        d->blurListener();
    }
}

void VGraphicsItem::onStare()
{
    if (d->stareListener) {
        d->stareListener();
    }
}

void VGraphicsItem::onTouch(const VTouchEvent &event)
{
    if (d->touchListener) {
        d->touchListener(event);
    }
}

void VGraphicsItem::onKeyPress(const VKeyEvent &event)
{
    if (d->keyPressListener) {
        d->keyPressListener(event);
    }
}

void VGraphicsItem::onSensorChanged(const VMatrix4f &mvp)
{
    for (VGraphicsItem *child : d->children) {
        child->onSensorChanged(mvp);
    }

    VMatrix4f pos = isFixed() ? VMatrix4f::Translation(globalPos()):mvp * VMatrix4f::Translation(globalPos());
    VRect3f rect = boundingRect();
    VVect3f start = pos.transform(rect.start);
    VVect3f end = pos.transform(rect.end);

    bool hovered = start.x <= 0 && start.y <= 0 && end.x >= 0 && end.y >= 0 && start.z >= -1 && start.z <= 1;
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

void VGraphicsItem::onTouchEvent(const VTouchEvent &event)
{
    for (VGraphicsItem *child : d->children) {
        child->onTouchEvent(event);
    }
    if (hasFocus()) {
        onTouch(event);
    }
}

void VGraphicsItem::onKeyEvent(const VKeyEvent &event)
{
    for (VGraphicsItem *child : d->children) {
        child->onKeyEvent(event);
    }
    if (hasFocus()) {
        onKeyPress(event);
    }
}

bool VGraphicsItem::isFixed() const
{
    return false;
}

bool VGraphicsItem::needCursor(const VMatrix4f &mvp) const
{
    for (VGraphicsItem *child : d->children) {
        if (child->needCursor(mvp)) return true;
    }

    if(d->focusListener || d->blurListener || d->stareListener || d->clickListener)
    {
        VMatrix4f pos = isFixed() ? VMatrix4f::Translation(globalPos()):mvp * VMatrix4f::Translation(globalPos());
        VRect3f rect = boundingRect();
        VVect3f start = pos.transform(rect.start);
        VVect3f end = pos.transform(rect.end);

        bool bEntered = (start.x>-1&&start.x<1&&start.y>-1&&start.y<1&&start.z>-1&&start.z<1) || (end.x>-1&&end.x<1&&end.y>-1&&end.y<1&&end.z>-1&&end.z<1);
        if(bEntered) return true;
    }

    return false;
}

NV_NAMESPACE_END
