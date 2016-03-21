#pragma once
#include "vgltypedefine.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include "vglobal.h"
#include "android/VOsBuild.h"
#include "android/LogUtils.h"
#include "VLog.h"

NV_NAMESPACE_BEGIN
class VGlOperation
{
 public:
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

    enum invalidateTarget_t {
        INV_DEFAULT,
        INV_FBO
    };

    enum EglKhrGlColorSpace
    {
        EGL_GL_COLORSPACE_KHR = 0x309D,
        EGL_GL_COLORSPACE_SRGB_KHR = 0x3089,
        EGL_GL_COLORSPACE_LINEAR_KHR = 0x3089A
    };

    enum ExtGLFrameSrgb
    {
        GL_FRAMEBUFFER_SRGB_EXT = 0x8DB9
    };

    enum ExtSrgbDecode
    {
        GL_TEXTURE_SRGB_DECODE_EXT = 0x8A48,
        GL_DECODE_EXT = 0x8A49,
        GL_SKIP_DECODE_EXT = 0x8A4A
    };

    enum QCOMBinningControl
    {
        GL_QUERY_RESULT_EXT = 0x8866,
        GL_TIME_ELAPSED_EXT = 0x88BF,
        GL_TIMESTAMP_EXT = 0x8E28,
        GL_GPU_DISJOINT_EXT = 0x8FBB
    };

    VGlOperation()
        : EGLProtectedConftentExt(0x32c0)
    {

    }

    GpuType EglGetGpuType();
    GpuType EglGetGpuTypeLocal();
    const char * GL_ErrorForEnum(const GLenum e);
    EGLConfig EglConfigForConfigID(const EGLDisplay display, const GLint configID);
    const char * EglErrorString();
    bool GL_CheckErrors(const char *logTitle);
    void GL_FindExtensions();
    bool GL_ExtensionStringPresent(const char *extension, const char *allExtensions);
    void GL_Finish();
    void GL_Flush();
    EGLint GL_FlushSync(int timeout);
    void GL_InvalidateFramebuffer(const invalidateTarget_t isFBO, const bool colorBuffer, const bool depthBuffer);
    void LogStringWords(const char *allExtensions);
    void *GetExtensionProc( const char * name );
    void LogStringWords(const char *allExtensions);

public:
    bool HasEXT_sRGB_texture_decode;
    bool EXT_disjoint_timer_query;
    bool EXT_discard_framebuffer;
    bool EXT_texture_filter_anisotropic;
    bool IMG_multisampled_render_to_texture;
    bool OES_vertex_array_object;
    bool QCOM_tiled_rendering;
    int EGLProtectedConftentExt;

    PFNGLDISCARDFRAMEBUFFEREXTPROC glDiscardFramebufferEXT_;

    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG glRenderbufferStorageMultisampleIMG_;
    PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG glFramebufferTexture2DMultisampleIMG_;

    PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR_;
    PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR_;
    PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR_;
    PFNEGLSIGNALSYNCKHRPROC eglSignalSyncKHR_;
    PFNEGLGETSYNCATTRIBKHRPROC eglGetSyncAttribKHR_;

    PFNGLBINDVERTEXARRAYOESPROC	glBindVertexArrayOES_;
    PFNGLDELETEVERTEXARRAYSOESPROC	glDeleteVertexArraysOES_;
    PFNGLGENVERTEXARRAYSOESPROC	glGenVertexArraysOES_;
    PFNGLISVERTEXARRAYOESPROC	glIsVertexArrayOES_;

    PFNGLSTARTTILINGQCOMPROC	glStartTilingQCOM_;
    PFNGLENDTILINGQCOMPROC		glEndTilingQCOM_;

    PFNGLGENQUERIESEXTPROC glGenQueriesEXT_;
    PFNGLDELETEQUERIESEXTPROC glDeleteQueriesEXT_;
    PFNGLISQUERYEXTPROC glIsQueryEXT_;
    PFNGLBEGINQUERYEXTPROC glBeginQueryEXT_;
    PFNGLENDQUERYEXTPROC glEndQueryEXT_;
    PFNGLQUERYCOUNTEREXTPROC glQueryCounterEXT_;
    PFNGLGETQUERYIVEXTPROC glGetQueryivEXT_;
    PFNGLGETQUERYOBJECTIVEXTPROC glGetQueryObjectivEXT_;
    PFNGLGETQUERYOBJECTUIVEXTPROC glGetQueryObjectuivEXT_;
    PFNGLGETQUERYOBJECTI64VEXTPROC glGetQueryObjecti64vEXT_;
    PFNGLGETQUERYOBJECTUI64VEXTPROC glGetQueryObjectui64vEXT_;
    PFNGLGETINTEGER64VPROC glGetInteger64v_;

    PFNGLBLITFRAMEBUFFER_				glBlitFramebuffer_;
    PFNGLRENDERBUFFERSTORAGEMULTISAMPLE_	glRenderbufferStorageMultisample_;
    PFNGLINVALIDATEFRAMEBUFFER_			glInvalidateFramebuffer_;
    PFNGLMAPBUFFERRANGE_					glMapBufferRange_;
    PFNGLUNMAPBUFFEROESPROC_				glUnmapBuffer_;

};
NV_NAMESPACE_END
