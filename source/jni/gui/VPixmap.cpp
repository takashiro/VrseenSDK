#include "VPixmap.h"
#include "VPainter.h"
#include "VGlGeometry.h"
#include "VGlShader.h"

NV_NAMESPACE_BEGIN

static const char* VertexShaderSource =
    "uniform highp mat4 Mvpm;\n"
    "attribute vec4 Position;\n"
    "attribute vec2 TexCoord;\n"
    "varying highp vec2 oTexCoord;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = Mvpm * Position;\n"
    "    oTexCoord = vec2(TexCoord.x, TexCoord.y);\n"
    "}\n";

const char * FragmentShaderSource =
    "uniform sampler2D Texture0;\n"
    "varying highp vec2 oTexCoord;\n"
    "void main()\n"
    "{\n"
    "    gl_FragColor = texture2D(Texture0, oTexCoord);\n"
    "}\n";

struct VPixmap::Private
{
    VTexture texture;
    VGlShader shader;
    VGlGeometry geometry;

    Private()
    {
        shader.initShader(VertexShaderSource, FragmentShaderSource);
        geometry.createPlaneQuadGrid(1, 1);
    }

    ~Private()
    {
        geometry.destroy();
        shader.destroy();
    }
};

VPixmap::VPixmap(VGraphicsItem *parent)
    : VGraphicsItem(parent)
{
}

VPixmap::VPixmap(const VTexture &texture, VGraphicsItem *parent)
    : VGraphicsItem(parent)
    , d(new Private)
{
    d->texture = texture;
}

VPixmap::~VPixmap()
{
    delete d;
}

void VPixmap::load(const VTexture &texture)
{
    d->texture = texture;
}

void VPixmap::paint(VPainter *painter)
{
    VGraphicsItem::paint(painter);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, d->texture.id());

    glUseProgram(d->shader.program);
    const VMatrix4f screenMvp = painter->viewMatrix() * transform();
    glUniformMatrix4fv(d->shader.uniformModelViewProMatrix, 1, GL_FALSE, screenMvp.transposed().data());
    d->geometry.drawElements();
}

const VRect3f &VPixmap::rect() const
{
    return boundingRect();
}

void VPixmap::setRect(const VRect3f &rect)
{
    setBoundingRect(rect);
}

NV_NAMESPACE_END
