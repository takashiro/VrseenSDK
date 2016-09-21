#include "VRectangle.h"
#include "VGlShader.h"
#include "VGlGeometry.h"
#include "VMatrix4.h"
#include "VPainter.h"

NV_NAMESPACE_BEGIN

static const char* VertexShaderSource =
    "uniform highp mat4 Mvpm;\n"
    "uniform highp mat4 Texm;\n"
    "attribute vec4 Position;\n"
    "attribute vec2 TexCoord;\n"
    "uniform mediump vec4 UniformColor;\n"
    "varying  highp vec2 oTexCoord;\n"
    "varying  lowp vec4 oColor;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = Mvpm * Position;\n"
    "   oTexCoord = vec2( Texm * vec4(TexCoord,1,1) );\n"
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
    const VMatrix4f screenMvp = painter->viewMatrix() * transform();
    glUniformMatrix4fv(d->shader.uniformModelViewProMatrix, 1, GL_FALSE, screenMvp.transposed().data());
    d->geometry.drawElements();
}

bool VRectangle::isFixed() const
{
    return  true;
}


NV_NAMESPACE_END
