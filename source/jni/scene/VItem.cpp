#include "VItem.h"

NV_NAMESPACE_BEGIN

struct VItem::Private
{
    VItem *parent;
    VArray<VItem *> children;
    VPosF pos;

    Private()
        : parent(nullptr)
    {
    }
};

VItem::VItem(VItem *parent)
    : d(new Private)
{
    setParent(parent);
}

VItem::~VItem()
{
    for (VItem *child : d->children) {
        delete child;
    }
    if (d->parent) {
        d->parent->removeChild(this);
    }
    delete d;
}

void VItem::addChild(VItem *item)
{
    if (!d->children.contains(item)) {
        d->children.append(item);
        item->d->parent = this;
    }
}

void VItem::removeChild(VItem *item)
{
    int i = d->children.indexOf(item);
    if (i > 0) {
        d->children.removeAt(i);
        item->d->parent = nullptr;
    }
}

const VArray<VItem *> &VItem::children() const
{
    return d->children;
}

VItem *VItem::parent() const
{
    return d->parent;
}

void VItem::setParent(VItem *item)
{
    if (d->parent == item)
        return;

    if (d->parent) {
        d->parent->removeChild(this);
    }
    if (item) {
        item->addChild(this);
    }
}

void VItem::paintAll()
{
    paint();
    for (VItem *child : d->children) {
        child->paintAll();
    }
}

void VItem::setPos(const VPosF &pos)
{
    d->pos = pos;
}

VPosF &VItem::pos()
{
    return d->pos;
}

VPosF VItem::pos() const
{
    return d->pos;
}

void VItem::setX(vreal x)
{
    d->pos.x = x;
}

vreal VItem::x() const
{
    return d->pos.x;
}

void VItem::setY(vreal y)
{
    d->pos.y = y;
}

vreal VItem::y() const
{
    return d->pos.y;
}

void VItem::setZ(vreal z)
{
    d->pos.z = z;
}

vreal VItem::z() const
{
    return d->pos.z;
}

void VItem::paint()
{
}

NV_NAMESPACE_END
