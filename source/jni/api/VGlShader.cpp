#include "VGlShader.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "Android/GlUtils.h"
#include "../core/VLog.h"

NV_NAMESPACE_BEGIN

/*
	Vertex Color
*/

const char * VertexColorVertexShaderSrc =
        "uniform highp mat4 Mvpm;\n"
        "attribute highp vec4 Position;\n"
        "attribute lowp vec4 VertexColor;\n"
        "varying lowp vec4 oColor;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = Mvpm * Position;\n"
        "   oColor = VertexColor;\n"
        "}\n";

const char * VertexColorSkinned1VertexShaderSrc =
        "uniform highp mat4 Mvpm;\n"
        "uniform highp mat4 Joints[" MAX_JOINTS_STRING "];\n"
        "attribute highp vec4 Position;\n"
        "attribute lowp vec4 VertexColor;\n"
        "attribute highp vec4 JointWeights;\n"
        "attribute highp vec4 JointIndices;\n"
        "varying lowp vec4 oColor;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = Mvpm * ( Joints[int(JointIndices.x)] * Position );\n"
        "   oColor = VertexColor;\n"
        "}\n";

const char * VertexColorFragmentShaderSrc =
        "varying lowp vec4 oColor;\n"
        "void main()\n"
        "{\n"
        "   gl_FragColor = oColor;\n"
        "}\n";

/*
	Single Texture
*/

const char * SingleTextureVertexShaderSrc =
        "uniform highp mat4 Mvpm;\n"
        "attribute highp vec4 Position;\n"
        "attribute highp vec2 TexCoord;\n"
        "varying highp vec2 oTexCoord;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = Mvpm * Position;\n"
        "   oTexCoord = TexCoord;\n"
        "}\n";

const char * SingleTextureSkinned1VertexShaderSrc =
        "uniform highp mat4 Mvpm;\n"
        "uniform highp mat4 Joints[" MAX_JOINTS_STRING "];\n"
        "attribute highp vec4 Position;\n"
        "attribute highp vec2 TexCoord;\n"
        "attribute highp vec4 JointWeights;\n"
        "attribute highp vec4 JointIndices;\n"
        "varying highp vec2 oTexCoord;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = Mvpm * ( Joints[int(JointIndices.x)] * Position );\n"
        "   oTexCoord = TexCoord;\n"
        "}\n";

const char * SingleTextureFragmentShaderSrc =
        "uniform sampler2D Texture0;\n"
        "varying highp vec2 oTexCoord;\n"
        "void main()\n"
        "{\n"
        "   gl_FragColor = texture2D( Texture0, oTexCoord );\n"
        "}\n";

/*
	Light Mapped
*/

const char * LightMappedVertexShaderSrc =
        "uniform highp mat4 Mvpm;\n"
        "attribute highp vec4 Position;\n"
        "attribute highp vec2 TexCoord;\n"
        "attribute highp vec2 TexCoord1;\n"
        "varying highp vec2 oTexCoord;\n"
        "varying highp vec2 oTexCoord1;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = Mvpm * Position;\n"
        "   oTexCoord = TexCoord;\n"
        "   oTexCoord1 = TexCoord1;\n"
        "}\n";

const char * LightMappedSkinned1VertexShaderSrc =
        "uniform highp mat4 Mvpm;\n"
        "uniform highp mat4 Joints[" MAX_JOINTS_STRING "];\n"
        "attribute highp vec4 Position;\n"
        "attribute highp vec2 TexCoord;\n"
        "attribute highp vec2 TexCoord1;\n"
        "attribute highp vec4 JointWeights;\n"
        "attribute highp vec4 JointIndices;\n"
        "varying highp vec2 oTexCoord;\n"
        "varying highp vec2 oTexCoord1;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = Mvpm * ( Joints[int(JointIndices.x)] * Position );\n"
        "   oTexCoord = TexCoord;\n"
        "   oTexCoord1 = TexCoord1;\n"
        "}\n";

const char * LightMappedFragmentShaderSrc =
        "uniform sampler2D Texture0;\n"
        "uniform sampler2D Texture1;\n"
        "varying highp vec2 oTexCoord;\n"
        "varying highp vec2 oTexCoord1;\n"
        "void main()\n"
        "{\n"
        "   lowp vec4 diffuse = texture2D( Texture0, oTexCoord );\n"
        "   lowp vec4 emissive = texture2D( Texture1, oTexCoord1 );\n"
        "   gl_FragColor.xyz = diffuse.xyz * emissive.xyz * 1.5;\n"
        "   gl_FragColor.w = diffuse.w;\n"
        "}\n";

/*
	Reflection Mapped
*/

const char * ReflectionMappedVertexShaderSrc =
        "uniform highp mat4 Mvpm;\n"
        "uniform highp mat4 Modelm;\n"
        "uniform highp mat4 Viewm;\n"
        "attribute highp vec4 Position;\n"
        "attribute highp vec3 Normal;\n"
        "attribute highp vec3 Tangent;\n"
        "attribute highp vec3 Binormal;\n"
        "attribute highp vec2 TexCoord;\n"
        "attribute highp vec2 TexCoord1;\n"
        "varying highp vec3 oEye;\n"
        "varying highp vec3 oNormal;\n"
        "varying highp vec3 oTangent;\n"
        "varying highp vec3 oBinormal;\n"
        "varying highp vec2 oTexCoord;\n"
        "varying highp vec2 oTexCoord1;\n"
        "vec3 multiply( mat4 m, vec3 v )\n"
        "{\n"
        "   return vec3(\n"
        "      m[0].x * v.x + m[1].x * v.y + m[2].x * v.z,\n"
        "      m[0].y * v.x + m[1].y * v.y + m[2].y * v.z,\n"
        "      m[0].z * v.x + m[1].z * v.y + m[2].z * v.z );\n"
        "}\n"
        "vec3 transposeMultiply( mat4 m, vec3 v )\n"
        "{\n"
        "   return vec3(\n"
        "      m[0].x * v.x + m[0].y * v.y + m[0].z * v.z,\n"
        "      m[1].x * v.x + m[1].y * v.y + m[1].z * v.z,\n"
        "      m[2].x * v.x + m[2].y * v.y + m[2].z * v.z );\n"
        "}\n"
        "void main()\n"
        "{\n"
        "   gl_Position = Mvpm * Position;\n"
        "   vec3 eye = transposeMultiply( Viewm, -vec3( Viewm[3] ) );\n"
        "   oEye = eye - vec3( Modelm * Position );\n"
        "   oNormal = multiply( Modelm, Normal );\n"
        "   oTangent = multiply( Modelm, Tangent );\n"
        "   oBinormal = multiply( Modelm, Binormal );\n"
        "   oTexCoord = TexCoord;\n"
        "   oTexCoord1 = TexCoord1;\n"
        "}\n";

const char * ReflectionMappedSkinned1VertexShaderSrc =
        "uniform highp mat4 Mvpm;\n"
        "uniform highp mat4 Modelm;\n"
        "uniform highp mat4 Viewm;\n"
        "uniform highp mat4 Joints[" MAX_JOINTS_STRING "];\n"
        "attribute highp vec4 Position;\n"
        "attribute highp vec3 Normal;\n"
        "attribute highp vec3 Tangent;\n"
        "attribute highp vec3 Binormal;\n"
        "attribute highp vec2 TexCoord;\n"
        "attribute highp vec2 TexCoord1;\n"
        "attribute highp vec4 JointWeights;\n"
        "attribute highp vec4 JointIndices;\n"
        "varying highp vec3 oEye;\n"
        "varying highp vec3 oNormal;\n"
        "varying highp vec3 oTangent;\n"
        "varying highp vec3 oBinormal;\n"
        "varying highp vec2 oTexCoord;\n"
        "varying highp vec2 oTexCoord1;\n"
        "vec3 multiply( mat4 m, vec3 v )\n"
        "{\n"
        "   return vec3(\n"
        "      m[0].x * v.x + m[1].x * v.y + m[2].x * v.z,\n"
        "      m[0].y * v.x + m[1].y * v.y + m[2].y * v.z,\n"
        "      m[0].z * v.x + m[1].z * v.y + m[2].z * v.z );\n"
        "}\n"
        "vec3 transposeMultiply( mat4 m, vec3 v )\n"
        "{\n"
        "   return vec3(\n"
        "      m[0].x * v.x + m[0].y * v.y + m[0].z * v.z,\n"
        "      m[1].x * v.x + m[1].y * v.y + m[1].z * v.z,\n"
        "      m[2].x * v.x + m[2].y * v.y + m[2].z * v.z );\n"
        "}\n"
        "void main()\n"
        "{\n"
        "   gl_Position = Mvpm * ( Joints[int(JointIndices.x)] * Position );\n"
        "   vec3 eye = transposeMultiply( Viewm, -vec3( Viewm[3] ) );\n"
        "   oEye = eye - vec3( Modelm * ( Joints[int(JointIndices.x)] * Position ) );\n"
        "   oNormal = multiply( Modelm, multiply( Joints[int(JointIndices.x)], Normal ) );\n"
        "   oTangent = multiply( Modelm, multiply( Joints[int(JointIndices.x)], Tangent ) );\n"
        "   oBinormal = multiply( Modelm, multiply( Joints[int(JointIndices.x)], Binormal ) );\n"
        "   oTexCoord = TexCoord;\n"
        "   oTexCoord1 = TexCoord1;\n"
        "}\n";

const char * ReflectionMappedFragmentShaderSrc =
        "uniform sampler2D Texture0;\n"
        "uniform sampler2D Texture1;\n"
        "uniform sampler2D Texture2;\n"
        "uniform sampler2D Texture3;\n"
        "uniform samplerCube Texture4;\n"
        "varying highp vec3 oEye;\n"
        "varying highp vec3 oNormal;\n"
        "varying highp vec3 oTangent;\n"
        "varying highp vec3 oBinormal;\n"
        "varying highp vec2 oTexCoord;\n"
        "varying highp vec2 oTexCoord1;\n"
        "void main()\n"
        "{\n"
        "   mediump vec3 normal = texture2D( Texture2, oTexCoord ).xyz * 2.0 - 1.0;\n"
        "   mediump vec3 surfaceNormal = normal.x * oTangent + normal.y * oBinormal + normal.z * oNormal;\n"
        "   mediump vec3 eyeDir = normalize( oEye.xyz );\n"
        "   mediump vec3 reflectionDir = dot( eyeDir, surfaceNormal ) * 2.0 * surfaceNormal - eyeDir;\n"
        "   lowp vec3 specular = texture2D( Texture3, oTexCoord ).xyz * textureCube( Texture4, reflectionDir ).xyz;\n"
        "   lowp vec4 diffuse = texture2D( Texture0, oTexCoord );\n"
        "   lowp vec4 emissive = texture2D( Texture1, oTexCoord1 );\n"
        "	gl_FragColor.xyz = diffuse.xyz * emissive.xyz * 1.5 + specular;\n"
        "   gl_FragColor.w = diffuse.w;\n"
        "}\n";

/*
	Misc
*/

const char * identityVertexShaderSource =
        "attribute vec4 Position;\n"
        "attribute vec4 VertexColor;\n"
        "attribute vec2 TexCoord;\n"
        "uniform mediump vec4 UniformColor;\n"
        "varying highp vec2 oTexCoord;\n"
        "varying lowp vec4 oColor;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = Position;\n"
        "	oTexCoord = TexCoord;\n"
        "   oColor = VertexColor * UniformColor;\n"
        "}\n";

const char * externalFragmentShaderSource =
        "#extension GL_OES_EGL_image_external : require\n"
        "uniform samplerExternalOES Texture0;\n"
        "uniform lowp vec4 ColorBias;\n"
        "varying highp vec2 oTexCoord;\n"
        "varying lowp vec4 oColor;\n"
        "void main()\n"
        "{\n"
        "	gl_FragColor = ColorBias + oColor * texture2D( Texture0, oTexCoord );\n"
        "}\n";

// Same as externalFragmentShaderSource, but with a normal texture
// sampler instead of a GL_OES_EGL_image_external
const char * textureFragmentShaderSource =
        "uniform sampler2D Texture0;\n"
        "uniform lowp vec4 ColorBias;\n"
        "varying highp vec2 oTexCoord;\n"
        "varying lowp vec4 oColor;\n"
        "void main()\n"
        "{\n"
        "	gl_FragColor = ColorBias + oColor * texture2D( Texture0, oTexCoord );\n"
        "}\n";

const char * untexturedFragmentShaderSource =
        "varying lowp vec4 oColor;\n"
        "void main()\n"
        "{\n"
        "	gl_FragColor = oColor;\n"
        "}\n";

struct VGlShader::Private
{
    static bool CompileShader( const GLuint shader, const char * src );
};

VGlShader::~VGlShader()
{

}

GLuint VGlShader::createShader(GLuint shaderType, const char *src)
{
    GLuint shader = glCreateShader( shaderType );
    if ( !d->CompileShader(shader, src ) )
    {
        vFatal( "Failed to compile shader,type:"<<shaderType);
    }
    return  shader;
}

GLuint VGlShader::createProgram(GLuint vertexShader, GLuint fragmentShader)
{
    program = glCreateProgram();
    glAttachShader( program, vertexShader );
    glAttachShader( program, fragmentShader );

    // set attributes before linking
    glBindAttribLocation( program, SHADER_ATTRIBUTE_LOCATION_POSITION,		"Position" );
    glBindAttribLocation( program, SHADER_ATTRIBUTE_LOCATION_NORMAL,			"Normal" );
    glBindAttribLocation( program, SHADER_ATTRIBUTE_LOCATION_TANGENT,			"Tangent" );
    glBindAttribLocation( program, SHADER_ATTRIBUTE_LOCATION_BINORMAL,		"Binormal" );
    glBindAttribLocation( program, SHADER_ATTRIBUTE_LOCATION_COLOR,			"VertexColor" );
    glBindAttribLocation( program, SHADER_ATTRIBUTE_LOCATION_UV0,				"TexCoord" );
    glBindAttribLocation( program, SHADER_ATTRIBUTE_LOCATION_UV1,				"TexCoord1" );
    glBindAttribLocation( program, SHADER_ATTRIBUTE_LOCATION_JOINT_WEIGHTS,	"JointWeights" );
    glBindAttribLocation( program, SHADER_ATTRIBUTE_LOCATION_JOINT_INDICES,	"JointIndices" );
    glBindAttribLocation( program, SHADER_ATTRIBUTE_LOCATION_FONT_PARMS,		"FontParms" );


    // link and error check
    glLinkProgram( program );
    GLint r;
    glGetProgramiv( program, GL_LINK_STATUS, &r );
    if ( r == GL_FALSE )
    {
        GLchar msg[1024];
        glGetProgramInfoLog( program, sizeof( msg ), 0, msg );
        vFatal( "Linking program failed: "<<msg );
    }
    uniformModelViewProMatrix = glGetUniformLocation( program, "Mvpm" );
    uniformModelMatrix = glGetUniformLocation( program, "Modelm" );
    uniformViewMatrix = glGetUniformLocation( program, "Viewm" );
    uniformProjectionMatrix = glGetUniformLocation( program, "Projectionm" );
    uniformColor = glGetUniformLocation( program, "UniformColor" );
    uniformTexMatrix = glGetUniformLocation( program, "Texm" );
    uniformTexMatrix2 = glGetUniformLocation( program, "Texm2" );
    uniformTexMatrix3 = glGetUniformLocation( program, "Texm3" );
    uniformTexMatrix4 = glGetUniformLocation( program, "Texm4" );
    uniformTexMatrix5 = glGetUniformLocation( program, "Texm5" );
    uniformTexClamp = glGetUniformLocation( program, "TexClamp" );
    uniformRotateScale = glGetUniformLocation( program, "RotateScale" );
    uniformJoints = glGetUniformLocation(program, "Joints" );
    uniformColorTableOffset = glGetUniformLocation( program, "ColorTableOffset" );
    uniformFadeDirection = glGetUniformLocation(program, "UniformFadeDirection" );

    glUseProgram( program );

    // texture and image_external bindings
    for ( int i = 0; i < 8; i++ )
    {
        char name[32];
        sprintf( name, "Texture%i", i );
        const GLint uTex = glGetUniformLocation( program, name );
        if ( uTex != -1 )
        {
            glUniform1i( uTex, i );
        }
    }

    return  program;
}

GLuint VGlShader::initShader(const char *vertexSrc, const char *fragmentSrc)
{
    vertexShader = createShader( GL_VERTEX_SHADER ,vertexSrc);
    fragmentShader = createShader( GL_FRAGMENT_SHADER ,fragmentSrc);
    program = createProgram(vertexShader,fragmentShader);

    glUseProgram( 0 );

    return  program;
}

void  VGlShader::destroy() 
{
    if ( program != 0 )
    {
        glDeleteProgram( program );
    }
    if ( vertexShader != 0 )
    {
        glDeleteShader( vertexShader );
    }
    if ( fragmentShader != 0 )
    {
        glDeleteShader( fragmentShader );
    }
    program = 0;
    vertexShader = 0;
    fragmentShader = 0;
}

bool VGlShader::Private::CompileShader( const GLuint shader, const char * src )
{
    glShaderSource( shader, 1, &src, 0 );
    glCompileShader( shader );

    GLint r;
    glGetShaderiv( shader, GL_COMPILE_STATUS, &r );
    if ( r == GL_FALSE )
    {
        vInfo( "Compiling shader: "<<src<<"****** failed ******\n" );
        GLchar msg[4096];
        glGetShaderInfoLog( shader, sizeof( msg ), 0, msg );
        vInfo( msg );
        return false;
    }
    return true;
}


NV_NAMESPACE_END