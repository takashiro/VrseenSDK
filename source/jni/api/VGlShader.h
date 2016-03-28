#pragma once

#include "../vglobal.h"
#include "api/VGlOperation.h"

NV_NAMESPACE_BEGIN


enum ShaderAttributeLocation
{
    LOCATION_POSITION		= 0,
    LOCATION_NORMAL		    = 1,
    LOCATION_TANGENT		= 2,
    LOCATION_BINORMAL		= 3,
    LOCATION_COLOR			= 4,
    LOCATION_UV0			= 5,
    LOCATION_UV1			= 6,
    LOCATION_JOINT_INDICES	= 7,
    LOCATION_JOINT_WEIGHTS	= 8,
    LOCATION_FONT_PARMS	    = 9
};

class VGlShader
{
public:
    VGlShader():
        program( 0 ),
        vertexShader( 0 ),
        fragmentShader( 0 ),
        uniformModelViewProMatrix( -1 ),
        uniformModelMatrix( -1 ),
        uniformViewMatrix( -1 ),
        uniformProjectionMatrix( -1 ),
        uniformColor( -1 ),
        uniformTexMatrix( -1 ),
        uniformTexMatrix2( -1 ),
        uniformTexMatrix3( -1 ),
        uniformTexMatrix4( -1 ),
        uniformTexMatrix5( -1 ),
        uniformColorTableOffset( -1 ),
        uniformFadeDirection( -1 ),
        uniformJoints( -1 ) {};
    VGlShader(const char * vertexSrc, const char * fragmentSrc);
    ~VGlShader();
    GLuint createShader(GLuint shaderType, const char* src);
    GLuint createProgram(GLuint vertexShader, GLuint fragmentShader);
    GLuint initShader (const char * vertexSrc, const char * fragmentSrc);
    void destroy();

   static const char * getAdditionalFragmentShaderSource();
   static const char * getAdditionalVertexShaderSource();

   static const char * getUntextureMvpVertexShaderSource();
   static const char * getUntextureInverseColorVertexShaderSource();
   static const char * getUntexturedFragmentShaderSource();
   static const char * getUniformColorVertexShaderSource();

   static const char * getVertexColorVertexShaderSource();
   static const char * getVertexColorSkVertexShaderSource();
   static const char * getVertexColorFragmentShaderSource();


   static const char * getSingleTextureVertexShaderSource();
   static const char * getSingleTextureSkVertexShaderSource();
   static const char * getSingleTextureFragmentShaderSource();

   static const char * getLightMappedVertexShaderSource();
   static const char * getLightMappedSkVertexShaderSource();
   static const char * getLightMappedFragmentShaderSource();


   static const char * getReflectionMappedVertexShaderSource();
   static const char * getReflectionMappedSkVertexShaderSource();
   static const char * getReflectionMappedFragmentShaderSource();

    GLuint	program;
    GLuint	vertexShader;
    GLuint	fragmentShader; 
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
    GLint   uniformColorTableOffset;	// uniform offset
    GLint	uniformFadeDirection;		// uniform FadeDirection
    GLint	uniformJoints;			// uniform Joints
private:
    NV_DECLARE_PRIVATE
};

NV_NAMESPACE_END


