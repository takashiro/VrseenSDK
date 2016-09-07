#include "VLoading.h"
#include "VPainter.h"
#include "VGlShader.h"
#include "VGlGeometry.h"
#include "VResource.h"
#include "VTimer.h"

NV_NAMESPACE_BEGIN

static const char* VertexShaderSource =
        "uniform highp mat4 Mvpm;\n"
        "attribute vec4 Position;\n"
        "attribute vec2 TexCoord;\n"
        "varying highp vec2 oTexCoord;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = Mvpm * Position;\n"
        "    oTexCoord = TexCoord;\n"
        "}\n";

static const char * FragmentShaderSource =
        "uniform sampler2D Texture0;\n"
        "varying highp vec2 oTexCoord;\n"
        "void main()\n"
        "{\n"
        "    vec4 color = texture2D(Texture0, oTexCoord);\n"
        "    gl_FragColor = vec4(color.rgb,color.r);\n"
        "}\n";

struct VLoading::Private
{
    VTexture texture;
    VGlShader shader;
    VGlGeometry geometry;

    unsigned  int duration;

    Private()
    {
        shader.initShader(VertexShaderSource, FragmentShaderSource);
        geometry.createPlaneQuadGrid(1, 1);

        duration = 3.0;
    }

    ~Private()
    {
        geometry.destroy();
        shader.destroy();
    }
};


VLoading::VLoading(VGraphicsItem *parent)
        : VPixmap(parent)
        , d(new Private)
{
    setBoundingRect(VRect3f(-0.1,-0.1,0,0.1,0.1,0));
}

VLoading::VLoading(const VTexture &texture, VGraphicsItem *parent)
        : VPixmap(parent)
        , d(new Private)
{
    d->texture = texture;
    setBoundingRect(VRect3f(-0.1,-0.1,0,0.1,0.1,0));
}

VLoading::~VLoading()
{
    delete d;
}

void VLoading::setDuration(unsigned int duration)
{
    d->duration = duration;
}

void VLoading::paint(VPainter *painter)
{
    VGraphicsItem::paint(painter);

    static double start = VTimer::Seconds();
    double end = VTimer::Seconds();
    double duration = end - start;
    if(duration>d->duration) return;

    double rotateAngle = duration /d->duration * M_PI * 2;

    VEglDriver::glPushAttrib();
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(d->shader.program);
    const VMatrix4f screenMvp = transform() * VMatrix4f::RotationZ(-rotateAngle);
    glUniformMatrix4fv(d->shader.uniformModelViewProMatrix, 1, GL_FALSE, screenMvp.transposed().data());
    d->geometry.textureId = d->texture.id();

    if(!d->geometry.textureId)
    {
        VTexture loadingIcon(VResource("res/raw/loading_indicator.png"), VTexture::NoMipmaps);
        d->geometry.textureId = loadingIcon.id();
    }

    d->geometry.drawElements();

    VEglDriver::glPopAttrib();
}

NV_NAMESPACE_END
