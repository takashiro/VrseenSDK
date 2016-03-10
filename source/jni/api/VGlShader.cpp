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


GLuint VGlShader::initShader(const char *vertexSrc, const char *fragmentSrc)
{
    vertexShader = glCreateShader( GL_VERTEX_SHADER );
    if ( !d->CompileShader(vertexShader, vertexSrc ) )
    {
        vFatal( "Failed to compile vertex shader" );
    }
    fragmentShader = glCreateShader( GL_FRAGMENT_SHADER );
    if ( !d->CompileShader(fragmentShader, fragmentSrc ) )
    {
        vFatal( "Failed to compile fragment shader" );
    }

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