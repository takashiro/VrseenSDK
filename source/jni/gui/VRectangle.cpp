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

struct VRectangle::Private
{
    VRect3f rect;
    VColor color;
    VGlShader shader;
    VGlGeometry geometry;
    VMatrix4f transform;

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
    return d->rect;
}

void VRectangle::setRect(const VRect3f &rect)
{
    d->rect = rect;

    const VVect3f size = rect.size();
    const VVect3f center = rect.start + size * 0.5f;
    const float	 screenHeight = size.y;
    const float screenWidth = std::max(size.x, size.z);
    float widthScale;
    float heightScale;
    float aspect = 1.0f;
    if (screenWidth / screenHeight > aspect) {
        // screen is wider than movie, clamp size to height
        heightScale = screenHeight * 0.5f;
        widthScale = heightScale * aspect;
    } else {
        // screen is taller than movie, clamp size to width
        widthScale = screenWidth * 0.5f;
        heightScale = widthScale / aspect;
    }

    const float rotateAngle = ( size.x > size.z ) ? 0.0f : (float) M_PI * 0.5f;

    d->transform = VMatrix4f::Translation(center) * VMatrix4f::RotationY(rotateAngle) * VMatrix4f::Scaling(widthScale, heightScale, 1.0f);
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
    glUseProgram(d->shader.program);
    glUniform4f(d->shader.uniformColor, d->color.red / 255.0f, d->color.green / 255.0f, d->color.blue / 255.0f, d->color.alpha / 255.0f);
    const VMatrix4f screenMvp = painter->viewMatrix() * d->transform;
    glUniformMatrix4fv(d->shader.uniformModelViewProMatrix, 1, GL_FALSE, screenMvp.transposed().data());
    d->geometry.drawElements();
}

NV_NAMESPACE_END
