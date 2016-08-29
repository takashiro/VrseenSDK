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
    VColor backgroundColor;

    Private()
        : viewWidth(1024)
        , viewHeight(1024)
        , backgroundColor(0.0f, 162.0f, 232.0f, 0.0f)
    {
        vg = nvgCreateGLES3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);

        int fontId = nvgCreateFont(vg, "sans", "/storage/emulated/0/VRSeen/SDK/fonttype/Roboto-Regular.ttf");
        if (fontId == -1) {
            printf("error\n");
            return ;
        }

        nvgFontSize(vg, 30.0f);
        nvgFontFace(vg, "sans");
        nvgFillColor(vg, nvgRGBA(0,255,0,128));
        nvgStrokeColor(vg, nvgRGBA(0, 255, 128, 128));
        nvgTextAlign(vg, NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
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

VColor VGui::backgroundColor() const
{
    return d->backgroundColor;
}

void VGui::setBackgroundColor(const VColor &color)
{
    d->backgroundColor = color;
}

void VGui::update()
{
    nvgBeginFrame(d->vg, d->viewWidth, d->viewHeight, 1.0f);
    glClearColor(d->backgroundColor.red / 255.0f, d->backgroundColor.green / 255.0f, d->backgroundColor.blue / 255.0f, d->backgroundColor.alpha / 255.0f);
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
