#include <scene/VEyeItem.h>
#include "VCursor.h"
#include "VPainter.h"
#include "VGlShader.h"
#include "VGlGeometry.h"
#include "VResource.h"

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

static const char * FragmentShaderSource =
        "uniform sampler2D Texture0;\n"
        "varying highp vec2 oTexCoord;\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = texture2D(Texture0, oTexCoord);\n"
        "}\n";

struct VCursor::Private
{
    VTexture texture;
    VGlShader shader;
    VGlGeometry geometry;
    VTexture cursorIcon;

    Private()
            : cursorIcon(VResource("res/raw/gaze_cursor_cross.tga"), VTexture::NoMipmaps)
    {
        shader.useMultiview = true;
        shader.initShader(VertexShaderSource, FragmentShaderSource);
        geometry.createPlaneQuadGrid(1, 1);
    }

    ~Private()
    {
        geometry.destroy();
        shader.destroy();
        if(texture.id()!=cursorIcon.id()) cursorIcon.destroy();
    }
};


VCursor::VCursor(VGraphicsItem *parent)
        : VPixmap(parent)
        , d(new Private)
{
    setBoundingRect(VRect3f(-0.02f, -0.02f, 0, 0.02 ,0.02, 0));
}

VCursor::VCursor(const VTexture &texture, VGraphicsItem *parent)
        : VPixmap(parent)
        , d(new Private)
{
    d->texture = texture;
    setBoundingRect(VRect3f(-0.02f, -0.02f, 0, 0.02, 0.02, 0));
}

VCursor::~VCursor()
{
    delete d;
}

void VCursor::paint(VPainter *painter)
{
    VGraphicsItem::paint(painter);

    VEglDriver::glPushAttrib();
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(d->shader.program);

    const VMatrix4f screenMvp = transform();
    if(VEyeItem::settings.useMultiview)
    {
        VMatrix4f modelViewProMatrix[2];
        modelViewProMatrix[0] = screenMvp;
        modelViewProMatrix[1] = screenMvp;
        glUniformMatrix4fv(d->shader.uniformModelViewProMatrix, 2, GL_TRUE, modelViewProMatrix[0].data());
    }
    else glUniformMatrix4fv(d->shader.uniformModelViewProMatrix, 1, GL_FALSE, screenMvp.transposed().data());

    d->geometry.textureId = d->texture.id();
    if(!d->geometry.textureId) {
        d->geometry.textureId = d->cursorIcon.id();
    }

    d->geometry.drawElements();

    VEglDriver::glPopAttrib();
}

NV_NAMESPACE_END
