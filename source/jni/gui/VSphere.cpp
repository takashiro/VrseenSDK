//
// Created by Gin on 2016/9/20.
//

#include "VSphere.h"
#include "VGlShader.h"
#include "VGlGeometry.h"
#include "VMatrix4.h"
#include "VPainter.h"

NV_NAMESPACE_BEGIN

static const char* VertexShaderSource =
                "uniform mat4 Mvpm;\n"
                "uniform highp mat4 Texm;\n"
                "attribute vec4 Position;\n"
                "attribute vec4 VertexColor;\n"
                "attribute vec2 TexCoord;\n"
                "uniform mediump vec4 UniformColor;\n"
                "varying  lowp vec4 oColor;\n"
                "varying highp vec2 oTexCoord;\n"
                "void main()\n"
                "{\n"
                "   gl_Position = Mvpm * Position;\n"
                "	oTexCoord = vec2( Texm * vec4(TexCoord,1,1) );\n"
                "   oColor =    UniformColor;\n"
                "}\n";

static const char *FragmentShaderSource =
                "uniform mediump vec4 UniformColor;\n"
                "void main()\n"
                "{\n"
                "	gl_FragColor = UniformColor;\n"
                "}\n";

struct VSphere::Private
{
    VColor color;
    VGlShader shader;
    VGlGeometry geometry;

    Private()
    {
        shader.initShader(VertexShaderSource, FragmentShaderSource);
        geometry.createSphere(1, 1);
    }

    ~Private()
    {
        shader.destroy();
        geometry.destroy();
    }
};

VSphere::VSphere(VGraphicsItem *parent)
        : VGraphicsItem(parent)
        , d(new Private)
{

}

VSphere::~VSphere()
{
    delete d;
}

VRect3f VSphere::rect() const
{
    return boundingRect();
}

void VSphere::setRect(const VRect3f &rect)
{
    setBoundingRect(rect);
}

VColor VSphere::color() const
{
    return d->color;
}

void VSphere::setColor(const VColor &color)
{
    d->color = color;
}

void VSphere::paint(VPainter *painter)
{

    VGraphicsItem::paint(painter);

    glUseProgram(d->shader.program);
    glUniform4f(d->shader.uniformColor, d->color.red / 255.0f, d->color.green / 255.0f, d->color.blue / 255.0f, d->color.alpha / 255.0f);
    const VMatrix4f screenMvp = painter->viewMatrix() * transform();
    glUniformMatrix4fv(d->shader.uniformModelViewProMatrix, 1, GL_FALSE, screenMvp.transposed().data());
    d->geometry.drawElements();
}

bool VSphere::isFixed() const
{
    return  true;
}



NV_NAMESPACE_END