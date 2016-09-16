//
// Created by yangkai on 2016/9/14.
//
#include <cmath>
#include "VCylinder.h"
#include <vglobal.h>
#include <VLog.h>
#include <VTexture.h>
#include <VResource.h>
NV_NAMESPACE_BEGIN
#define PI 3.141593
static GLuint createShader(const char *src,GLuint shaderType){
    GLuint shader = glCreateShader( shaderType );
    glShaderSource( shader, 1, &src, 0 );
    glCompileShader( shader );
    GLint r;
    glGetShaderiv( shader, GL_COMPILE_STATUS, &r );
    if ( r == GL_FALSE )
    {
        vInfo( "Compiling shader: "<<src<<"****** failed ******\n" );
        GLchar msg[4096];
        glGetShaderInfoLog( shader, sizeof( msg ), 0, msg );
        vError( msg );
        return 0;
    }
    return shader;
}
static GLuint _createProgram(const char *ver, const char * fag){
    GLuint vshader = createShader(ver,GL_VERTEX_SHADER);
    GLuint fshader = createShader(fag,GL_FRAGMENT_SHADER);
    GLuint p = glCreateProgram();
    glAttachShader( p, vshader );
    glAttachShader( p, fshader );
    glLinkProgram( p );
    GLint r;
    glGetProgramiv( p, GL_LINK_STATUS, &r );
    if ( r == GL_FALSE )
    {
        GLchar msg[1024];
        glGetProgramInfoLog( p, sizeof( msg ), 0, msg );
        vFatal( "Linking program failed: "<<msg );
    }
    return p;
}
void VCylinder::setMVP(const GLfloat m[4][4]) {
    if(m == nullptr)
        return;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            m_mvp[i][j] = m[j][i];
        }
    }
    glUseProgram(m_program);
    glUniformMatrix4fv(m_MvpLoc, 1, false, m_mvp[0]);
    glUseProgram(0);
}
VCylinder::VCylinder() {
    m_texture =  VTexture(VResource("assets/han_2.jpg"), VTexture::UseSRGB).id();
    createProgram();
    createShape();
    glUseProgram(m_program);

    glUniformMatrix4fv(glGetUniformLocation(m_program,"mvp"), 1, false,m_mvp[0]);
    glUniform1i(glGetUniformLocation(m_program,"tex"),0);
    m_MvpLoc = glGetUniformLocation(m_program, "mvp");
    glUseProgram(0);
}
void VCylinder::draw(){
    glEnable( GL_DEPTH_TEST );
    glDisable( GL_CULL_FACE );
    glUseProgram(m_program);
    glBindVertexArray( m_vertexArray );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, m_texture );
    glDrawElements( GL_TRIANGLES, m_hCount * m_vCount * 6, GL_UNSIGNED_INT , NULL );
    glUseProgram(0);
    glBindVertexArray( 0 );
    glBindTexture( GL_TEXTURE_2D, 0 );
}
void VCylinder::createProgram() {
    m_program = _createProgram(
            "attribute vec4 Position;\n"
            "attribute vec2 TexCoord;\n"
            "uniform mediump mat4 mvp;"
            "varying  highp vec2 oTexCoord;\n"
            "void main()\n"
            "{\n"
            "   gl_Position = mvp*Position;\n"
            "   oTexCoord = TexCoord;\n"
            "}\n",

            "varying highp vec2 oTexCoord;\n"
            "uniform sampler2D tex;\n"
            "void main()\n"
            "{\n"
            "   gl_FragColor = texture2D( tex, oTexCoord);\n"
            "}\n");
}
void VCylinder::createShape(){
    GLuint count = m_hCount * (m_vCount + 1);
    GLfloat *vertex = new GLfloat [count * 4 + count * 2];
    GLfloat *texCor = vertex + count * 4;
    GLuint *indexBuf = new GLuint [m_hCount * m_vCount * 6];
    GLfloat dH = 2*PI / m_hCount;
    GLfloat dV = 1.0 / m_vCount;
    for (GLuint vInd = 0;vInd <= m_vCount;vInd++) {
        for (GLuint hInd = 0;hInd < m_hCount;hInd++) {
            int i = (hInd + vInd * m_hCount);
            vertex[4*i] = sin(dH*hInd);
            vertex[4*i+1] = dV*vInd - 0.5;
            vertex[4*i+2] = cos(dH*hInd);
            vertex[4*i+3] = 1;

            texCor[2*i] = hInd / (GLfloat)m_hCount;
            texCor[2*i + 1] = 1 - vInd / (GLfloat)m_vCount;
        }
    }
    for (GLuint i = 0;i < m_vCount;i++) {
        for (GLuint j = 0;j < m_hCount;j++) {
            int iIndex = 6*(j + i*m_hCount);
            indexBuf[iIndex] = i*m_hCount+j;
            indexBuf[iIndex+1] = (i+1)*m_hCount+j;
            indexBuf[iIndex+2] = (i+1)*m_hCount+((j+1)%m_hCount);
            indexBuf[iIndex+3] = i*m_hCount+j;
            indexBuf[iIndex+4] = i*m_hCount+((j+1)%m_hCount);
            indexBuf[iIndex+5] = (i+1)*m_hCount+((j+1)%m_hCount);
        }
    }
    GLuint vertexBuffer, indBuf;
    glGenVertexArrays( 1, &m_vertexArray );
    glBindVertexArray( m_vertexArray );
    glGenBuffers( 1, &vertexBuffer );
    glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );
    glBufferData( GL_ARRAY_BUFFER, (4*count+2*count)*sizeof( GLfloat ), vertex, GL_STATIC_DRAW );
    glGenBuffers( 1, &indBuf );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indBuf );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, m_hCount * m_vCount * 6 * sizeof( GLuint ), indexBuf, GL_STATIC_DRAW );

    GLuint ind = glGetAttribLocation(m_program,"Position");
    glEnableVertexAttribArray( ind );
    glVertexAttribPointer( ind, 4, GL_FLOAT, false,
                           4*sizeof( GLfloat ), nullptr );
    ind = glGetAttribLocation(m_program,"TexCoord");
    glEnableVertexAttribArray( ind );
    glVertexAttribPointer( ind, 2, GL_FLOAT, false,
                           2*sizeof( GLfloat ), (const GLvoid *)(count * 4 * sizeof(GLfloat)) );
    glBindVertexArray( 0 );
    delete [] vertex;
    delete [] indexBuf;
}
NV_NAMESPACE_END