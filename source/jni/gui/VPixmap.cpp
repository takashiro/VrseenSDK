#include <scene/VEyeItem.h>
#include <App.h>
#include "VPixmap.h"
#include "VPainter.h"
#include "VGlGeometry.h"
#include "VGlShader.h"

NV_NAMESPACE_BEGIN

static const char* VertexShaderSource =
    "uniform highp mat4 Mvpm[NUM_VIEWS];\n"
    "attribute vec4 Position;\n"
    "attribute vec2 TexCoord;\n"
    "varying highp vec2 oTexCoord;\n"
    "void main()\n"
    "{\n"
    "    gl_Position = Mvpm[VIEW_ID] * Position;\n"
    "    oTexCoord = TexCoord;\n"
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
        shader.useMultiview = true;
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
    , d(new Private)
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

    glUseProgram(d->shader.program);

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

    d->geometry.textureId = d->texture.id();
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
