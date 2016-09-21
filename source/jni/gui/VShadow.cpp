#include "VShadow.h"
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
                "uniform lowp vec4 UniformColor;\n"
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

struct VShadow::Private
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

VShadow::VShadow(VGraphicsItem *parent)
        : VGraphicsItem(parent)
        , d(new Private)
{
    setRect(VRect3f(VVect3f(-5.0, -5.0, -1), VVect3f(5.0, 5.0, -1)));
    setVisible(false);
}

VShadow::~VShadow()
{
    delete d;
}

VRect3f VShadow::rect() const
{
    return boundingRect();
}

void VShadow::setRect(const VRect3f &rect)
{
    setBoundingRect(rect);
}

VColor VShadow::color() const
{
    return d->color;
}

void VShadow::setColor(const VColor &color)
{
    d->color = color;
}

void VShadow::paint(VPainter *painter)
{
    VGraphicsItem::paint(painter);

    VEglDriver::glPushAttrib();
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(d->shader.program);
    glUniform4f(d->shader.uniformColor, d->color.red / 255.0f, d->color.green / 255.0f, d->color.blue / 255.0f, d->color.alpha / 255.0f);
    const VMatrix4f screenMvp = transform();
    glUniformMatrix4fv(d->shader.uniformModelViewProMatrix, 1, GL_FALSE, screenMvp.transposed().data());
    d->geometry.drawElements();

    VEglDriver::glPopAttrib();
}

bool VShadow::isFixed() const
{
    return  true;
}


NV_NAMESPACE_END
