#include "VGui.h"
#include "VGraphicsItem.h"
#include "VRectangle.h"

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

    Private()
        : viewWidth(1024)
        , viewHeight(1024)
    {
        vg = nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
    }

    ~Private()
    {
        nvgDeleteGLES3(vg);
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

void VGui::update()
{
    nvgBeginFrame(d->vg, d->viewWidth, d->viewHeight, 1.0f);
    glClearColor(0, 162.0f / 255, 232.0f / 255, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    d->root.paint(reinterpret_cast<VGuiPainter *>(d->vg));
    nvgEndFrame(d->vg);
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
