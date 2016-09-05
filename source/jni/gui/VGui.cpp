#include "VGui.h"
#include "VGraphicsItem.h"
#include "VRectangle.h"

NV_NAMESPACE_BEGIN

struct VGui::Private
{
    VGraphicsItem root;

    Private()
    {
    }

    ~Private()
    {
    }
};

VGui::VGui()
    : d(new Private)
{
}

VGui::~VGui()
{
    delete d;
}

void VGui::update()
{
    d->root.paint(nullptr);
}

VGraphicsItem *VGui::root() const
{
    return &d->root;
}

void VGui::addItem(VGraphicsItem *item)
{
    d->root.addChild(item);
}

NV_NAMESPACE_END

