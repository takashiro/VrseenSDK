#include <App.h>
#include "VRectangle.h"
#include "VGlShader.h"
#include "VGlGeometry.h"
#include "VMatrix4.h"
#include "VPainter.h"

NV_NAMESPACE_BEGIN

static const char* VertexShaderSource =
    "uniform highp mat4 Mvpm[NUM_VIEWS];\n"
    "attribute vec4 Position;\n"
    "uniform mediump vec4 UniformColor;\n"
    "varying  lowp vec4 oColor;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = Mvpm[VIEW_ID] * Position;\n"
    "   oColor = UniformColor;\n"
    "}\n";

static const char *FragmentShaderSource =
    "uniform mediump vec4 UniformColor;\n"
    "void main()\n"
    "{\n"
    "	gl_FragColor = UniformColor;\n"
    "}\n";

struct VRectangle::Private
{
    VColor color;
    VGlShader shader;
    VGlGeometry geometry;

    Private()
    {
        shader.useMultiview = true;
        shader.initShader(VertexShaderSource, FragmentShaderSource);
        geometry.createPlaneQuadGrid(1, 1);
    }

    ~Private()
    {
        shader.destroy();
        geometry.destroy();
    }
};

VRectangle::VRectangle(VGraphicsItem *parent)
    : VGraphicsItem(parent)
    , d(new Private)
{
}

VRectangle::~VRectangle()
{
    delete d;
}

VRect3f VRectangle::rect() const
{
    return boundingRect();
}

void VRectangle::setRect(const VRect3f &rect)
{
    setBoundingRect(rect);
}

VColor VRectangle::color() const
{
    return d->color;
}

void VRectangle::setColor(const VColor &color)
{
    d->color = color;
}

void VRectangle::paint(VPainter *painter)
{
    VGraphicsItem::paint(painter);

    glUseProgram(d->shader.program);
    glUniform4f(d->shader.uniformColor, d->color.red / 255.0f, d->color.green / 255.0f, d->color.blue / 255.0f, d->color.alpha / 255.0f);

    if(VEyeItem::settings.useMultiview)
    {
        VMatrix4f modelViewProMatrix[2];
        modelViewProMatrix[0] = vApp->getModelViewProMatrix(0)* transform();
        modelViewProMatrix[1] = vApp->getModelViewProMatrix(1)* transform();
        glUniformMatrix4fv(d->shader.uniformModelViewProMatrix, 2, GL_TRUE, modelViewProMatrix[0].data());
    }
    else
    {
        const VMatrix4f screenMvp = painter->viewMatrix() * transform();
        glUniformMatrix4fv(d->shader.uniformModelViewProMatrix, 1, GL_FALSE, screenMvp.transposed().data());
    }

    d->geometry.drawElements();
}

bool VRectangle::isFixed() const
{
    return  true;
}


NV_NAMESPACE_END
