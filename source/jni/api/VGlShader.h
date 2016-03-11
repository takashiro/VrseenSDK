#pragma once

#include "../vglobal.h"
#include "Android/GlUtils.h"

NV_NAMESPACE_BEGIN

enum ShaderAttributeLocation
{
    SHADER_ATTRIBUTE_LOCATION_POSITION		= 0,
    SHADER_ATTRIBUTE_LOCATION_NORMAL		= 1,
    SHADER_ATTRIBUTE_LOCATION_TANGENT		= 2,
    SHADER_ATTRIBUTE_LOCATION_BINORMAL		= 3,
    SHADER_ATTRIBUTE_LOCATION_COLOR			= 4,
    SHADER_ATTRIBUTE_LOCATION_UV0			= 5,
    SHADER_ATTRIBUTE_LOCATION_UV1			= 6
};

class VGlShader
{
public:
    VGlShader();
    ~VGlShader();
    GLuint createShader(GLuint shaderType, const char* src);
    GLuint createProgram(GLuint vertexShader, GLuint fragmentShader);
    GLuint initShader (const char * vertexSrc, const char * fragmentSrc);
    void destroy();

    GLuint	program;
    GLuint	vertexShader;
    GLuint	fragmentShader;

    // Uniforms that aren't found will have a -1 value
    GLint	uniformModelViewProMatrix;	    // ModelViewProMatrix
    GLint	uniformModelMatrix;				// uniform ModelMatrix
    GLint	uniformViewMatrix;				// uniform ViewMatrix
    GLint	uniformProjectionMatrix;		// uniform ProjectionMatrix
    GLint	uniformColor;				// uniform  Color
    GLint	uniformTexMatrix;				// uniform TexMatrix
    GLint	uniformTexMatrix2;				// uniform TexMatrix2
    GLint	uniformTexMatrix3;				// uniform TexMatrix3
    GLint	uniformTexMatrix4;				// uniform TexMatrix4
    GLint	uniformTexMatrix5;				// uniform TexMatrix5
    GLint	uniformTexClamp;			// uniform TexClamp
    GLint	uniformRotateScale;		// uniform RotateScale
private:
    NV_DECLARE_PRIVATE
};

NV_NAMESPACE_END


