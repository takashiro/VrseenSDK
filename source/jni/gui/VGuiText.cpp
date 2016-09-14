#include "VGuiText.h"
#include "VString.h"
#include "VPainter.h"
#include "3rdparty/nanovg/nanovg.h"
#include "VVect2.h"
#include "VResource.h"

NV_NAMESPACE_BEGIN

struct VGuiText::Private
{
    VString text;
    VColor color;
//    int fontId;

    VVect2f pos;
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


//void VGuiText::init()
//{
//    //load font data from apk package, only applied to  PanoPhoto
//    VResource *resLoad = new VResource("res/raw/roboto_regular.ttf");
//    uchar *fontData = const_cast<uchar *>(reinterpret_cast<const uchar *>(resLoad->data().c_str()));
//    int dataLength = resLoad->length();
//
//    NVGcontext *vg = static_cast<NVGcontext *>(painter);
//   // d->fontId = nvgCreateFont(vg, "sans", "/storage/emulated/0/VRSeen/SDK/fonttype/Roboto-Regular.ttf");
//    d->fontId = nvgCreateFontMem(vg, "sans", fontData, dataLength, 0);
//    if (d->fontId == -1) {
//        return ;
//    }
//}


void VGuiText::paint(VPainter *painter)
{
    NVGcontext *vg = static_cast<NVGcontext *>(painter->nativeContext());

    nvgFontSize(vg, 40.0f);
    nvgFontFace(vg, "sans");
    nvgTextAlign(vg, NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);

    nvgFontBlur(vg,2);
    nvgFillColor(vg, nvgRGBA(d->color.red, d->color.green, d->color.blue, d->color.alpha));
    nvgStrokeColor(vg, nvgRGBA(255, 0, 128, 128));

    //cannot make the last parament to be nullptr, otherwise this function can not work as it want to be
    nvgText(vg, d->pos.x ,d->pos.y, reinterpret_cast<const char*>(d->text.c_str()), reinterpret_cast<const char *>(d->text.c_str() + d->text.length()));
}

void VGuiText::setPos(float x, float y)
{
    d->pos.x = x;
    d->pos.y = y;
}


NV_NAMESPACE_END

