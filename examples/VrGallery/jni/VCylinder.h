//
// Created by yangkai on 2016/9/14.
//
#pragma once

#include <vglobal.h>
#include <GLES3/gl3.h>

NV_NAMESPACE_BEGIN

class VCylinder {
public:
    VCylinder();
    void setMVP(const GLfloat m[4][4]);
    void draw();
private:
    void createShape();
    void createProgram();
    GLuint m_program;
    GLuint m_vertexArray;
    GLuint m_texture;
    GLint m_MvpLoc;
    GLfloat m_mvp[4][4] =
            {   {1,0,0,0},
                {0,1,0,0},
                {0,0,1,0},
                {0,0,0,1}
            };

    GLuint m_hCount = 500;    //圆等分的份数
    GLuint m_vCount = 10;    //高度等分的份数
};
NV_NAMESPACE_END
