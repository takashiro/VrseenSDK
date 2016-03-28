#pragma once

#include "../vglobal.h"
#include "api/VGlOperation.h"

NV_NAMESPACE_BEGIN

// STRINGIZE is used so program text strings can include lines like:
// "uniform highp mat4 Joints["MAX_JOINTS_STRING"];\n"

#define STRINGIZE( x )			#x
#define STRINGIZE_VALUE( x )	STRINGIZE( x )

#define MAX_JOINTS				16
#define MAX_JOINTS_STRING		STRINGIZE_VALUE( MAX_JOINTS )

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

extern const char * externalFragmentShaderSource;
extern const char * textureFragmentShaderSource;
extern const char * identityVertexShaderSource;
extern const char * untexturedFragmentShaderSource;
extern const char * VertexColorVertexShaderSrc;
extern const char * VertexColorSkinned1VertexShaderSrc;
extern const char * VertexColorFragmentShaderSrc;
extern const char * SingleTextureVertexShaderSrc;
extern const char * SingleTextureSkinned1VertexShaderSrc;
extern const char * SingleTextureFragmentShaderSrc;
extern const char * LightMappedVertexShaderSrc;
extern const char * LightMappedSkinned1VertexShaderSrc;
extern const char * LightMappedFragmentShaderSrc;
extern const char * ReflectionMappedVertexShaderSrc;
extern const char * ReflectionMappedSkinned1VertexShaderSrc;
extern const char * ReflectionMappedFragmentShaderSrc;

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
    GLint   uniformColorTableOffset;	// uniform offset to apply to color table index
    GLint	uniformFadeDirection;		// uniform FadeDirection
    GLint	uniformJoints;			// uniform Joints
private:
    NV_DECLARE_PRIVATE
};

NV_NAMESPACE_END


