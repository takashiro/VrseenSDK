#pragma once

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include "android/VOsBuild.h"
#include "VLog.h"

#define __gl2_h_

#include <EGL/egl.h>
#include <EGL/eglext.h>
#ifdef __gl2_h_
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
static const int GL_ES_VERSION = 3;
#else
#include <GLES2/gl2.h>
static const int GL_ES_VERSION = 2;
#endif
#include <GLES2/gl2ext.h>

#define EGL_OPENGL_ES3_BIT_KHR      0x0040

NV_NAMESPACE_BEGIN

class VEglDriver
{
public:


    enum EglEnum
    {
        EGL_GL_COLORSPACE_KHR = 0x309D,
        EGL_GL_COLORSPACE_SRGB_KHR = 0x3089,
        EGL_GL_COLORSPACE_LINEAR_KHR = 0x3089A,
        GL_FRAMEBUFFER_SRGB_EXT = 0x8DB9,
        GL_TEXTURE_SRGB_DECODE_EXT = 0x8A88,
        GL_DECODE_EXT = 0x8A49,
        GL_SKIP_DECODE_EXT = 0x8A4A,
        GL_QUERY_RESULT_EXT = 0x8866,
        GL_TIME_ELAPSED_EXT = 0x88BF,
        GL_TIMESTAMP_EXT = 0x8E28,
        GL_GPU_DISJOINT_EXT = 0x8FBB
    };

    enum GpuType
    {
        GPU_TYPE_ADRENO					= 0x1000,
        GPU_TYPE_ADRENO_330				= 0x1001,
        GPU_TYPE_ADRENO_420				= 0x1002,
        GPU_TYPE_MALI					= 0x2000,
        GPU_TYPE_MALI_T760				= 0x2100,
        GPU_TYPE_MALI_T760_EXYNOS_5433	= 0x2101,
        GPU_TYPE_MALI_T760_EXYNOS_7420	= 0x2102,
        GPU_TYPE_UNKNOWN				= 0xFFFF
    };

    VEglDriver();
    ~VEglDriver();

    ushort eglGetGpuType();

    const char * getGlErrorEnum(const GLenum e);
    EGLConfig eglConfigForConfigID( const GLint configID);
    const char * getEglErrorString();
    bool logErrorsEnum(const char *logTitle);
    void logExtensions();
    bool glIsExtensionString(const char *extension);
    void glFinish();
    void glFlush();
    EGLint glWaitforFlush(int timeout);
    void glDisableFramebuffer(const bool colorBuffer,
                                  const bool depthBuffer);



    void * getExtensionProc( const char * name );
     void updateDisplay();
     void initExtensions();


    EGLConfig chooseColorConfig( const int redBits,
                                 const int greeBits,
                                 const int blueBits,
                                 const int depthBits,
                                 const int samples,
                                 const bool pbuffer );
    void eglInit( const EGLContext shareContext,
                   const int GlEsVersion,
                   const int red, const int green, const int blue,
                   const int depth, const int multisamples,
                   const GLuint contextPriority );


    void glRenderbufferStorageMultisampleIMG(GLenum target,
                                             GLsizei samples,
                                             GLenum internalformat,
                                             GLsizei width,
                                             GLsizei height);
    void glFramebufferTexture2DMultisampleIMG(GLenum target,
                                              GLenum attachment,
                                              GLenum textarget,
                                              GLuint texture,
                                              GLint level,
                                              GLsizei samples);
    EGLSyncKHR eglCreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list);
    EGLBoolean eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync);
    EGLint eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout);
    void glBindVertexArrayOES(GLuint array);
    void glDeleteVertexArraysOES(GLsizei n, const GLuint *arrays);
    void glGenVertexArraysOES(GLsizei n, GLuint *arrays);
    void glStartTilingQCOM(GLuint x, GLuint y, GLuint width, GLuint height, GLbitfield preserveMask);
    void glEndTilingQCOM(GLbitfield preserveMask);
    void glGenQueriesEXT(GLsizei n, GLuint *ids);
    void glDeleteQueriesEXT(GLsizei n, const GLuint *ids);
    void glBeginQueryEXT(GLenum target, GLuint id);
    void glEndQueryEXT(GLenum target);
    void glQueryCounterEXT(GLuint id, GLenum target);
    void glGetQueryObjectivEXT(GLuint id, GLenum pname, GLint *params);
    void glGetQueryObjectui64vEXT(GLuint id, GLenum pname, GLuint64 *params);
    void glGetInteger64v(GLenum pname, GLint64 *params);
    void glBlitFramebuffer(GLint srcX0,
                           GLint srcY0,
                           GLint srcX1,
                           GLint srcY1,
                           GLint dstX0,
                           GLint dstY0,
                           GLint dstX1,
                           GLint dstY1,
                           GLbitfield mask,
                           GLenum filter);
    void  glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum* attachments);


    int	m_glEsVersion;
    ushort	m_gpuType;
    EGLDisplay m_display;
    EGLSurface m_pbufferSurface;
    EGLConfig m_config;
    EGLContext m_context;
    const char * m_extensions;
};
NV_NAMESPACE_END
