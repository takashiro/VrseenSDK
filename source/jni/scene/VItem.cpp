#include "VItem.h"

NV_NAMESPACE_BEGIN

struct VItem::Private
{
    VItem *parent;
    VArray<VItem *> children;

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
    if (d->children.contains(item)) {
        d->children.removeOne(item);
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

void VItem::paint()
{
}

NV_NAMESPACE_END
