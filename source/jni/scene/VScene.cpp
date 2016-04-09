#include "VScene.h"

#include "VItem.h"

NV_NAMESPACE_BEGIN

struct VScene::Private
{
    VItem *rootItem;
    VColor color;

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

NV_NAMESPACE_END
