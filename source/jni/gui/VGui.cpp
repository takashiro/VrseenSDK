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

    Private()
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

void VGui::update()
{
    nvgBeginFrame(d->vg, 1024, 1024, 1.0f);
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

