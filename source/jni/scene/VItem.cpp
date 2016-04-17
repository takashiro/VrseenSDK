#include "VItem.h"

NV_NAMESPACE_BEGIN

struct VItem::Private
{
    VItem *parent;
    VArray<VItem *> children;
    //VPosF pos;
    bool visible;

    Private()
        : parent(nullptr)
        , visible(true)
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

void VItem::update()
{
    paint();
    for (VItem *child : d->children) {
        if (child->isVisible()) {
            child->update();
        }
    }
}

//void VItem::setPos(const VPosF &pos)
//{
//    d->pos = pos;
//}
//
//VPosF &VItem::pos()
//{
//    return d->pos;
//}
//
//VPosF VItem::pos() const
//{
//    return d->pos;
//}
//
//VPosF VItem::globalPos() const
//{
//    VPosF pos = this->pos();
//    VItem *parent = this->parent();
//    while (parent) {
//        pos += parent->pos();
//        parent = parent->parent();
//    }
//    return pos;
//}
//
//void VItem::setX(vreal x)
//{
//    d->pos.x = x;
//}
//
//vreal VItem::x() const
//{
//    return d->pos.x;
//}
//
//void VItem::setY(vreal y)
//{
//    d->pos.y = y;
//}
//
//vreal VItem::y() const
//{
//    return d->pos.y;
//}
//
//void VItem::setZ(vreal z)
//{
//    d->pos.z = z;
//}
//
//vreal VItem::z() const
//{
//    return d->pos.z;
//}

void VItem::setVisible(bool visible)
{
    d->visible = visible;
}

bool VItem::isVisible() const
{
    return d->visible;
}

void VItem::paint()
{
}

NV_NAMESPACE_END
