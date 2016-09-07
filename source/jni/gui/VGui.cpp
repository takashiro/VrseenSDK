#include "VGui.h"
#include "VGraphicsItem.h"
#include "VPainter.h"
#include "VClickEvent.h"

#define NANOVG_GLES3_IMPLEMENTATION
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include "3rdparty/nanovg/nanovg.h"
#include "3rdparty/nanovg/nanovg_gl.h"


NV_NAMESPACE_BEGIN

struct VGui::Private
{
    VGraphicsItem root;
    NVGcontext *vg;
    int viewWidth;
    int viewHeight;
    VColor backgroundColor;

    Private()
        : viewWidth(1024)
        , viewHeight(1024)
        , backgroundColor(0.0f, 162.0f, 232.0f, 0.0f)
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

void VGui::init()
{
    d->vg = nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
    d->root.init(d->vg);
}

void VGui::prepare()
{
    nvgBeginFrame(d->vg, d->viewWidth, d->viewHeight, 1.0f);
}

void VGui::update(const VMatrix4f &mvp)
{
    d->root.onSensorChanged(mvp);
    VPainter painter;
    painter.setNativeContext(d->vg);
    painter.setViewMatrix(mvp);
    d->root.paint(&painter);
}

void VGui::commit()
{
    nvgEndFrame(d->vg);
}

int VGui::viewWidth() const
{
    return d->viewWidth;
}

void VGui::setViewWidth(int width)
{
    d->viewWidth = width;
}

int VGui::viewHeight() const
{
    return d->viewHeight;
}

void VGui::setViewHeight(int height)
{
    d->viewHeight = height;
}

VColor VGui::backgroundColor() const
{
    return d->backgroundColor;
}

void VGui::setBackgroundColor(const VColor &color)
{
    d->backgroundColor = color;
}

VGraphicsItem *VGui::root() const
{
    return &d->root;
}

void VGui::addItem(VGraphicsItem *item)
{
    d->root.addChild(item);
}

void VGui::onKeyEvent(int keyCode, int repeatCount)
{
    VClickEvent event;
    event.key = keyCode;
    event.repeat = repeatCount;
    d->root.onKeyEvent(event);
}

NV_NAMESPACE_END
