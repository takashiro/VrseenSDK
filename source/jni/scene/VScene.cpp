#include "VScene.h"

#include "VItem.h"

NV_NAMESPACE_BEGIN

struct VScene::Private
{
    VItem *rootItem;

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

void VScene::update()
{
    d->rootItem->update();
}

NV_NAMESPACE_END
