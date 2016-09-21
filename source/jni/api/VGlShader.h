#pragma once

#include "vglobal.h"
#include "VEglDriver.h"

NV_NAMESPACE_BEGIN


enum VertexLocation
{

    VERTEX_POSITION		= 0,
    VERTEX_NORMAL		= 1,
    VERTEX_TANGENT		= 2,
    VERTEX_BINORMAL		= 3,
    VERTEX_COLOR		= 4,
    VERTEX_UVC0			= 5,
    VERTEX_UVC1			= 6,
    JOINT_INDICES	= 7,
    JOINT_WEIGHTS	= 8,
    FONT_PARMS	= 9

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
        uniformJoints( -1 ) ,
        useMultiview(false){};
    VGlShader(const char * vertexSrc, const char * fragmentSrc):
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
        uniformJoints( -1 ) ,
        useMultiview(false)
    {
        initShader(vertexSrc,fragmentSrc);
    }
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
   static const char * getUniformTextureProgramShaderSource();
   static const char * getUniformSingleTextureProgramShaderSource();
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

   static const char * getDoubletextureTransparentColorVertexShaderSource();
   static const char * getHighlightPragmentShaderSource();
   static const char * getHighlightColorVertexShaderSource();
   static const char * getTexturedMvpVertexShaderSource();

   static const char * getCubeMapPanoProgramShaderSource();
   static const char * getCubeMapPanoVertexShaderSource();

   static  const char * getPanoProgramShaderSource();
  static   const char * getPanoVertexShaderSource();


   static const char * getFadedPanoVertexShaderSource();
   static const char * getFadedPanoProgramShaderSource();
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

    bool useMultiview;
private:
    NV_DECLARE_PRIVATE
};

NV_NAMESPACE_END


