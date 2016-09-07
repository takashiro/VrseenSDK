#include "VTileButton.h"
#include "VRectangle.h"
#include "VPixmap.h"

NV_NAMESPACE_BEGIN

struct VTileButton::Private
{
    VRectangle *background;
    VPixmap *image;

    Private(VTileButton *button)
        : background(new VRectangle)
        , image(new VPixmap)
    {
        background->setVisible(false);
        background->setColor(VColor(0, 162, 232));
        button->addChild(background);
        button->addChild(image);
    }
};

VTileButton::VTileButton(VGraphicsItem *parent)
    : VGraphicsItem(parent)
    , d(new Private(this))
{
}

VTileButton::~VTileButton()
{
    delete d;
}

void VTileButton::setRect(const VRect3f &rect)
{
    setBoundingRect(rect);
    const float borderWidth = 0.02f;
    d->background->setRect(rect);
    VVect3f center = rect.center();
    VVect3f size = rect.size();
    VRect3f inner;
    inner.start.x = center.x - size.x * (0.5f - borderWidth);
    inner.start.y = center.y - size.y * (0.5f - borderWidth);
    inner.start.z = rect.start.z + 0.001f;
    inner.end.x = center.x + size.x * (0.5f - borderWidth);
    inner.end.y = center.y + size.y * (0.5f - borderWidth);
    inner.end.z = rect.end.z + 0.001f;
    d->image->setRect(inner);
}

void VTileButton::setImage(const VTexture &image)
{
    d->image->load(image);
}

void VTileButton::setBackgroundColor(const VColor &color)
{
    d->background->setColor(color);
}

void VTileButton::onFocus()
{
    d->background->setVisible(true);
}

void VTileButton::onBlur()
{
    d->background->setVisible(false);
}

NV_NAMESPACE_END
