#include "VGui.h"
#include "VGraphicsItem.h"
#include "VPainter.h"

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
    VRectangle *rect = new VRectangle;
    rect->setRect(VRect3f(0.026997f, 0.427766f, -3.253125f, 5.973003f, 4.391771f, -3.253125f));
    rect->setColor(VColor(0, 255, 0));
    addItem(rect);
}

VGui::~VGui()
{
    delete d;
}

void VGui::update(const VMatrix4f &mvp)
{
    VPainter painter;
    painter.setViewMatrix(mvp);
    d->root.paint(&painter);
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

