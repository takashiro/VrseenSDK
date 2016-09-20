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
"attribute vec4 position;\n"
"attribute vec4 inputImpostorSpaceCoordinate;\n"
"varying mediump vec2 impostorSpaceCoordinate;\n"
"varying mediump vec3 normalizedViewCoordinate;\n"
"uniform mat4 modelViewProjMatrix;\n"
"uniform mediump mat4 orthographicMatrix;\n"
"uniform mediump float sphereRadius;\n"

"void main()\n"
"{\n"
    "vec4 transformedPosition;\n"
    "transformedPosition = modelViewProjMatrix * position;\n"
    "impostorSpaceCoordinate = inputImpostorSpaceCoordinate.xy;\n"

    "transformedPosition.xy = transformedPosition.xy + inputImpostorSpaceCoordinate.xy * vec2(sphereRadius);\n"
    "transformedPosition = transformedPosition * orthographicMatrix;\n"

    "normalizedViewCoordinate = (transformedPosition.xyz + 1.0) / 2.0;\n"
    "gl_Position = transformedPosition;\n"
"}";

static const char *FragmentShaderSource =
        "precision mediump float;\n"

        "uniform vec3 lightPosition;\n"
        "uniform vec3 sphereColor;\n"
        "uniform mediump float sphereRadius;\n"
        "uniform sampler2D depthTexture;\n"

        "varying mediump vec2 impostorSpaceCoordinate;\n"
        "varying mediump vec3 normalizedViewCoordinate;\n"

        "const mediump vec3 oneVector = vec3(1.0, 1.0, 1.0);\n"

        "void main()\n"
        "{\n"
        "float distanceFromCenter = length(impostorSpaceCoordinate);\n"

        "if (distanceFromCenter > 1.0)\n"
        "{\n"
            "discard;\n"
        "}\n"

        "float normalizedDepth = sqrt(1.0 - distanceFromCenter * distanceFromCenter);\n"

        "float depthOfFragment = sphereRadius * 0.5 * normalizedDepth;\n"
        "float currentDepthValue = (normalizedViewCoordinate.z - depthOfFragment - 0.0025);\n"
        "vec3 normal = vec3(impostorSpaceCoordinate, normalizedDepth);\n"
        "vec3 finalSphereColor = sphereColor;\n"

        "float lightingIntensity = 0.3 + 0.7 * clamp(dot(lightPosition, normal), 0.0, 1.0);\n"
        "finalSphereColor *= lightingIntensity;\n"

        "lightingIntensity  = clamp(dot(lightPosition, normal), 0.0, 1.0);\n"
        "lightingIntensity  = pow(lightingIntensity, 60.0);\n"
        "finalSphereColor += vec3(0.4, 0.4, 0.4) * lightingIntensity;\n"

        "gl_FragColor = vec4(finalSphereColor, 1.0);\n"
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