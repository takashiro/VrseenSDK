#include "VGuiText.h"
#include "VString.h"
#include "3rdparty/nanovg/nanovg.h"

NV_NAMESPACE_BEGIN

struct VGuiText::Private
{
    VString text;
    VColor color;
    int fontType;

};

VGuiText::VGuiText(VGraphicsItem* parent)
    : VGraphicsItem(parent)
    ,d(new Private())
{

}

VGuiText::~VGuiText()
{
    delete d;
}

VColor VGuiText::textColor() const
{
    return d->color;
}

void VGuiText::setTextColor(const VColor &color)
{
    d->color = color;
}

VString VGuiText::textValue() const
{
    return d->text;
}

void VGuiText::setTextValue(const VString &text)
{
    d->text = text;
}

void VGuiText::paint(VGuiPainter *painter)
{
    NVGcontext *vg = reinterpret_cast<NVGcontext *>(painter);
    nvgBeginPath(vg);
    nvgText(vg, 50.0f, 50.0, static_cast<const char *>(d->text.c_str()), nullptr);
    nvgFillColor(vg, nvgRGBA(d->color.red, d->color.green, d->color.blue, d->color.alpha));
    nvgFill(vg);
}
NV_NAMESPACE_END

