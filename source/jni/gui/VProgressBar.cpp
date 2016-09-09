#include <gui/VPixmap.h>
#include <core/VPath.h>
#include "VProgressBar.h"
#include "VRectangle.h"
#include "VPainter.h"
#include "VTexture.h"
#include "VResource.h"

NV_NAMESPACE_BEGIN

struct VProgressBar::Private
{
    VPixmap *bar;
    VPixmap *tag;
    VPixmap *inc;
};

VProgressBar::VProgressBar(VGraphicsItem *parent)
        : VGraphicsItem(parent)
        , d(new Private)
{
    d->bar = new VPixmap;
    d->tag = new VPixmap;
    d->inc = new VPixmap;
    addChild(d->bar);
    addChild(d->tag);
    addChild(d->inc);
}

VProgressBar::~VProgressBar()
{
    delete[](d->bar);
    delete[](d->tag);
    delete[](d->inc);
}

void VProgressBar::change(float ratio)
{
    VPixmap * tag = d->tag;
    VPixmap * bar = d->bar;
    VPixmap * inc = d->inc;

    if (tag == nullptr || bar == nullptr)
        return;

    float barlength = bar->boundingRect().end.x - bar->boundingRect().start.x;


    tag->setRect(VRect3f(VVect3f(bar->boundingRect().start.x + ratio * barlength, tag->boundingRect().start.y, tag->boundingRect().start.z), VVect3f(bar->boundingRect().start.x + barlength / 20 +  ratio * barlength, tag->boundingRect().end.y, tag->boundingRect().end.z)));
    inc->setRect(VRect3f(bar->boundingRect().start, VVect3f(bar->boundingRect().start.x + ratio * barlength, inc->boundingRect().end.y, inc->boundingRect().end.z)));
}

void VProgressBar::setRect(const VRect3f &rect3f)
{
    VVect3f midstart(rect3f.start.x, (rect3f.start.y + rect3f.end.y) / 2 - (rect3f.end.y - rect3f.start.y) / 5, rect3f.start.z + 0.1);
    VVect3f midend(rect3f.end.x, (rect3f.start.y + rect3f.end.y) / 2 + (rect3f.end.y - rect3f.start.y) / 5, rect3f.start.z + 0.1);

    d->bar->setRect(VRect3f(midstart, midend));
    d->tag->setRect(VRect3f(VVect3f(midstart.x, (rect3f.start.y + rect3f.end.y) / 2 - (rect3f.end.y - rect3f.start.y) / 3, rect3f.start.z + 0.15), VVect3f(midstart.x + (midend.x - midstart.x) / 20, (rect3f.start.y + rect3f.end.y) / 2 + (rect3f.end.y - rect3f.start.y) / 3, rect3f.start.z + 0.15 )));
    d->inc->setRect(VRect3f(midstart, VVect3f(midstart.x, midend.y, rect3f.start.z + 0.11)));

    setBoundingRect(rect3f);
}

void VProgressBar::setBarImage(const VTexture &tex)
{
    d->bar->load(VTexture(tex));
}

void VProgressBar::setTagImage(const VTexture &tex)
{
    d->tag->load(VTexture(tex));
}

void VProgressBar::setIncImage(const VTexture &tex)
{
    d->inc->load(VTexture(tex));
}

VPixmap * VProgressBar::getBar()
{
    return d->bar;
}

VPixmap * VProgressBar::getTag()
{
    return d->tag;
}

VPixmap * VProgressBar::getInc()
{
    return d->inc;
}


NV_NAMESPACE_END
