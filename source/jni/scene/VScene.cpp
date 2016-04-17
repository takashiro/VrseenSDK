#include "VScene.h"

#include "VItem.h"
#include "VEyeItem.h"

NV_NAMESPACE_BEGIN

struct VScene::Private
{
    VItem *rootItem;
    VColor color;

    VArray<VItem*> eyeItemList;

    Private()
        : rootItem(new VItem)
    {
    }

    ~Private()
    {
        delete rootItem;
    }
};

VScene::VScene()
    : d(new Private)
{
}

VScene::~VScene()
{
    delete d;
}

void VScene::add(VItem *item)
{
    d->rootItem->addChild(item);
}

void VScene::remove(VItem *item)
{
    d->rootItem->removeChild(item);
}

const VColor &VScene::backgroundColor() const
{
    return d->color;
}

void VScene::setBackgroundColor(const VColor &color)
{
    d->color = color;
}

void VScene::update()
{
    d->rootItem->update();
}

VItem* VScene::addEyeItem(VItem *parent)
{
    if(!parent) parent = d->rootItem;
    VEyeItem* eyeItem = new VEyeItem();
    parent->addChild(eyeItem);
    d->eyeItemList.push_back(eyeItem);

    return eyeItem;
}

VArray<VItem*> VScene::getEyeItemList()
{
    return d->eyeItemList;
}

NV_NAMESPACE_END
