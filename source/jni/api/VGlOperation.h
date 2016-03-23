#pragma once

#include "vgltypedefine.h"
#define EGL_OPENGL_ES3_BIT_KHR      0x0040

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

    enum invalidateTarget_t
    {
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
    {
        extensions = (const char *)glGetString( GL_EXTENSIONS );
        if ( NULL == extensions )
        {
            LOG( "glGetString( GL_EXTENSIONS ) returned NULL" );
        }
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
    void DumpEglConfigs(const EGLDisplay display);
    EGLConfig ChooseColorConfig( const EGLDisplay display, const int redBits,
            const int greeBits, const int blueBits, const int depthBits, const int samples, const bool pbuffer );
    void EglSetup( const EGLContext shareContext,
            const int requestedGlEsVersion,
            const int redBits, const int greenBits, const int blueBits,
            const int depthBits, const int multisamples,
            const GLuint contextPriority );
    void	EglShutdown();

    int			glEsVersion;
    GpuType		gpuType;
    EGLDisplay	display;
    EGLSurface	pbufferSurface;
    EGLConfig	config;
    EGLContext	context;
    const char * extensions;

    static bool HasEXT_sRGB_texture_decode;
    static bool EXT_disjoint_timer_query;
    static bool EXT_discard_framebuffer;
    static bool EXT_texture_filter_anisotropic;
    static bool OES_vertex_array_object;
    static bool QCOM_tiled_rendering;
    static const int EGL_PROTECTED_CONTENT_EXT;

    void glRenderbufferStorageMultisampleIMG(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
    void glFramebufferTexture2DMultisampleIMG(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples);

    static PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR_;
    void eglCreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list);
    static PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR_;
    static PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR_;
    static PFNEGLSIGNALSYNCKHRPROC eglSignalSyncKHR_;
    static PFNEGLGETSYNCATTRIBKHRPROC eglGetSyncAttribKHR_;

    static PFNGLBINDVERTEXARRAYOESPROC	glBindVertexArrayOES_;
    static PFNGLDELETEVERTEXARRAYSOESPROC	glDeleteVertexArraysOES_;
    static PFNGLGENVERTEXARRAYSOESPROC	glGenVertexArraysOES_;
    static PFNGLISVERTEXARRAYOESPROC	glIsVertexArrayOES_;

    static PFNGLSTARTTILINGQCOMPROC	glStartTilingQCOM_;
    static PFNGLENDTILINGQCOMPROC		glEndTilingQCOM_;

    static PFNGLGENQUERIESEXTPROC glGenQueriesEXT_;
    static PFNGLDELETEQUERIESEXTPROC glDeleteQueriesEXT_;
    static PFNGLISQUERYEXTPROC glIsQueryEXT_;
    static PFNGLBEGINQUERYEXTPROC glBeginQueryEXT_;
    static PFNGLENDQUERYEXTPROC glEndQueryEXT_;
    static PFNGLQUERYCOUNTEREXTPROC glQueryCounterEXT_;
    static PFNGLGETQUERYIVEXTPROC glGetQueryivEXT_;
    static PFNGLGETQUERYOBJECTIVEXTPROC glGetQueryObjectivEXT_;
    static PFNGLGETQUERYOBJECTUIVEXTPROC glGetQueryObjectuivEXT_;
    static PFNGLGETQUERYOBJECTI64VEXTPROC glGetQueryObjecti64vEXT_;
    static PFNGLGETQUERYOBJECTUI64VEXTPROC glGetQueryObjectui64vEXT_;
    static PFNGLGETINTEGER64VPROC glGetInteger64v_;

    static PFNGLBLITFRAMEBUFFER_				glBlitFramebuffer_;
    static PFNGLRENDERBUFFERSTORAGEMULTISAMPLE_	glRenderbufferStorageMultisample_;
    static PFNGLINVALIDATEFRAMEBUFFER_			glInvalidateFramebuffer_;
    static PFNGLMAPBUFFERRANGE_					glMapBufferRange_;
    static PFNGLUNMAPBUFFEROESPROC_				glUnmapBuffer_;

};
NV_NAMESPACE_END
