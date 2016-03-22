#pragma once

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include "android/VOsBuild.h"
#include "android/LogUtils.h"
#include "VLog.h"
#include "vglobal.h"
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

#define GL_BINNING_CONTROL_HINT_QCOM           0x8FB0

#define GL_CPU_OPTIMIZED_QCOM                  0x8FB1
#define GL_GPU_OPTIMIZED_QCOM                  0x8FB2
#define GL_RENDER_DIRECT_TO_FRAMEBUFFER_QCOM   0x8FB3
#define GL_DONT_CARE                           0x1100

typedef void (GL_APIENTRYP PFNGLGENQUERIESEXTPROC) (GLsizei n, GLuint *ids);
typedef void (GL_APIENTRYP PFNGLDELETEQUERIESEXTPROC) (GLsizei n, const GLuint *ids);
typedef GLboolean (GL_APIENTRYP PFNGLISQUERYEXTPROC) (GLuint id);
typedef void (GL_APIENTRYP PFNGLBEGINQUERYEXTPROC) (GLenum target, GLuint id);
typedef void (GL_APIENTRYP PFNGLENDQUERYEXTPROC) (GLenum target);
typedef void (GL_APIENTRYP PFNGLQUERYCOUNTEREXTPROC) (GLuint id, GLenum target);
typedef void (GL_APIENTRYP PFNGLGETQUERYIVEXTPROC) (GLenum target, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTIVEXTPROC) (GLuint id, GLenum pname, GLint *params);
typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTUIVEXTPROC) (GLuint id, GLenum pname, GLuint *params);
typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTI64VEXTPROC) (GLuint id, GLenum pname, GLint64 *params);
typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTUI64VEXTPROC) (GLuint id, GLenum pname, GLuint64 *params);
typedef void (GL_APIENTRYP PFNGLGETINTEGER64VPROC) (GLenum pname, GLint64 *params);

typedef void (GL_APIENTRYP PFNGLBLITFRAMEBUFFER_) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
typedef void (GL_APIENTRYP PFNGLRENDERBUFFERSTORAGEMULTISAMPLE_) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
typedef void (GL_APIENTRYP PFNGLINVALIDATEFRAMEBUFFER_) (GLenum target, GLsizei numAttachments, const GLenum* attachments);
typedef GLvoid* (GL_APIENTRYP PFNGLMAPBUFFERRANGE_) (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
typedef GLboolean (GL_APIENTRYP PFNGLUNMAPBUFFEROESPROC_) (GLenum target);

NV_NAMESPACE_BEGIN
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

struct eglSetup_t
{
        int			glEsVersion;		// 2 or 3
        GpuType		gpuType;
        EGLDisplay	display;
        EGLSurface	pbufferSurface;		// use to make context current when we don't have window surfaces
        EGLConfig	config;
        EGLContext	context;
};
NV_NAMESPACE_END
