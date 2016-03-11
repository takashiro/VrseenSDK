#include "VGlShader.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "Android/GlUtils.h"
#include "../core/VLog.h"

NV_NAMESPACE_BEGIN

struct VGlShader::Private
{
    static bool CompileShader( const GLuint shader, const char * src );
};

VGlShader::VGlShader()
{

}

VGlShader::~VGlShader()
{
    destroy();
}

GLuint VGlShader::createShader(GLuint shaderType, const char *src)
{
    GLuint shader = glCreateShader( shaderType );
    if ( !d->CompileShader(shaderType, src ) )
    {
        vFatal( "Failed to compile shader,type:"<<shaderType);
    }
    return  shader;
}

GLuint VGlShader::createProgram(GLuint vertexShader, GLuint fragmentShader)
{
    this->vertexShader = vertexShader;
    this->fragmentShader = fragmentShader;
    program = glCreateProgram();
    glAttachShader( program, vertexShader );
    glAttachShader( program, fragmentShader );

    // set attributes before linking
    glBindAttribLocation( program, SHADER_ATTRIBUTE_LOCATION_POSITION,		"aPosition" );
    glBindAttribLocation( program, SHADER_ATTRIBUTE_LOCATION_NORMAL,			"aNormal" );
    glBindAttribLocation( program, SHADER_ATTRIBUTE_LOCATION_TANGENT,			"aTangent" );
    glBindAttribLocation( program, SHADER_ATTRIBUTE_LOCATION_BINORMAL,		"aBinormal" );
    glBindAttribLocation( program, SHADER_ATTRIBUTE_LOCATION_COLOR,			"aVertexColor" );
    glBindAttribLocation( program, SHADER_ATTRIBUTE_LOCATION_UV0,				"aTextureCoord" );
    glBindAttribLocation( program, SHADER_ATTRIBUTE_LOCATION_UV1,				"aTextureCoord1" );

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
    uniformModelViewProMatrix = glGetUniformLocation( program, "uMVPMatrix" );
    uniformModelMatrix = glGetUniformLocation( program, "uMatrix" );
    uniformViewMatrix = glGetUniformLocation( program, "uVMatrix" );
    uniformProjectionMatrix = glGetUniformLocation( program, "uPMatrix" );
    uniformColor = glGetUniformLocation( program, "uColor" );
    uniformTexMatrix = glGetUniformLocation( program, "uTMatrix" );
    uniformTexMatrix2 = glGetUniformLocation( program, "uTMatrix2" );
    uniformTexMatrix3 = glGetUniformLocation( program, "uTMatrix3" );
    uniformTexMatrix4 = glGetUniformLocation( program, "uTMatrix4" );
    uniformTexMatrix5 = glGetUniformLocation( program, "uTMatrix5" );
    uniformTexClamp = glGetUniformLocation( program, "TexClamp" );
    uniformRotateScale = glGetUniformLocation( program, "RotateScale" );

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
    createProgram(vertexShader,fragmentShader);

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