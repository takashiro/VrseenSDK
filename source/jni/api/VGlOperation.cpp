#include "VGlOperation.h"

NV_NAMESPACE_BEGIN



 ushort VGlOperation::eglGetGpuType()
{

    GpuType gpuType;
    const char * glRendererString = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    if (strstr(glRendererString, "Adreno (TM) 420")) {
        gpuType = GPU_TYPE_ADRENO_420;
    } else if (strstr(glRendererString, "Adreno (TM) 330")) {
        gpuType = GPU_TYPE_ADRENO_330;
    } else if (strstr(glRendererString, "Adreno")) {
        gpuType = GPU_TYPE_ADRENO;
    } else if (strstr(glRendererString, "Mali-T760")) {
        const VString &hardware = VOsBuild::getString(VOsBuild::Hardware);
        if (hardware == "universal5433") {
            gpuType = GPU_TYPE_MALI_T760_EXYNOS_5433;
        } else if (hardware == "samsungexynos7420") {
            gpuType = GPU_TYPE_MALI_T760_EXYNOS_7420;
        } else {
            gpuType = GPU_TYPE_MALI_T760;
        }
    } else if (strstr(glRendererString, "Mali")) {
        gpuType = GPU_TYPE_MALI;
    } else {
        gpuType = GPU_TYPE_UNKNOWN;
    }

    //VInfo("SoC:" << VOsBuild::getString(VOsBuild::Hardware));
    //VInfo("EglGetGpuType:" << gpuType);

    return gpuType;
}


VGlOperation::VGlOperation()
{
    extensions = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
    if (NULL == extensions) {
        vInfo("glGetString( GL_EXTENSIONS ) returned NULL");
    }
}

EGLConfig VGlOperation::eglConfigForConfigID(const EGLDisplay display, const GLint configID)
{
    static const int MAX_CONFIGS = 1024;
    EGLConfig configs[MAX_CONFIGS];
    EGLint numConfigs = 0;

    if (EGL_FALSE == eglGetConfigs(display,
                                   configs,
                                   MAX_CONFIGS,
                                   &numConfigs)) {
        //vWarn("eglGetConfigs() failed");
        return NULL;
    }

    for (int i = 0; i < numConfigs; i++) {
        EGLint	value = 0;

        eglGetConfigAttrib(display, configs[i], EGL_CONFIG_ID, &value);
        if (value == configID) {
            return configs[i];
        }
    }

    return NULL;
}
const char *VGlOperation::getEglErrorString()
{
    const EGLint err = eglGetError();
    switch(err) {
    case EGL_SUCCESS:
        return "EGL_SUCCESS";
    case EGL_NOT_INITIALIZED:
        return "EGL_NOT_INITIALIZED";
    case EGL_BAD_ACCESS:
        return "EGL_BAD_ACCESS";
    case EGL_BAD_ALLOC:
        return "EGL_BAD_ALLOC";
    case EGL_BAD_ATTRIBUTE:
        return "EGL_BAD_ATTRIBUTE";
    case EGL_BAD_CONTEXT:
        return "EGL_BAD_CONTEXT";
    case EGL_BAD_CONFIG:
        return "EGL_BAD_CONFIG";
    case EGL_BAD_CURRENT_SURFACE:
        return "EGL_BAD_CURRENT_SURFACE";
    case EGL_BAD_DISPLAY:
        return "EGL_BAD_DISPLAY";
    case EGL_BAD_SURFACE:
        return "EGL_BAD_SURFACE";
    case EGL_BAD_MATCH:
        return "EGL_BAD_MATCH";
    case EGL_BAD_PARAMETER:
        return "EGL_BAD_PARAMETER";
    case EGL_BAD_NATIVE_PIXMAP:
        return "EGL_BAD_NATIVE_PIXMAP";
    case EGL_BAD_NATIVE_WINDOW:
        return "EGL_BAD_NATIVE_WINDOW";
    case EGL_CONTEXT_LOST:
        return "EGL_CONTEXT_LOST";
    default:
        return "Unknown egl error code";
    }
}

const char * VGlOperation::getGlErrorEnum(const GLenum e)
{
    switch(e) {
    case GL_NO_ERROR:
        return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    default:
        return "Unknown gl error code";
    }
}

bool VGlOperation::logErrorsEnum(const char *logTitle)
{
    bool hadError = false;

    do {
        GLenum err = glGetError();
        if (err == GL_NO_ERROR) {
            break;
        }
        hadError = true;
        vWarn(logTitle << "GL Error:" << getGlErrorEnum(err));
        if (err == GL_OUT_OF_MEMORY) {
            vFatal("GL_OUT_OF_MEMORY");
        }
    } while (1);
    return hadError;
}

EGLint VGlOperation::glWaitforFlush(int timeout)
{

    EGLDisplay eglDisplay = eglGetCurrentDisplay();

    const EGLSyncKHR sync = eglCreateSyncKHR(eglDisplay, EGL_SYNC_FENCE_KHR, NULL);
    if (sync == EGL_NO_SYNC_KHR) {
        return EGL_FALSE;
    }

    const EGLint wait = eglClientWaitSyncKHR(eglDisplay, sync, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, timeout);

    eglDestroySyncKHR(eglDisplay, sync);

    return wait;
}

void * VGlOperation::getExtensionProc( const char * name )
{
    void * ptr = reinterpret_cast<void*>(eglGetProcAddress(name));
    if (!ptr) {
        //vInfo("NOT FOUND:" << name);
    }
    return ptr;
}

void VGlOperation::logExtensions()
{
    const char * extensions = (const char *)glGetString( GL_EXTENSIONS );
    if (NULL == extensions) {
        //vInfo("glGetString( GL_EXTENSIONS ) returned NULL");
        return;
    }


    vInfo("GL_EXTENSIONS:");


//    const bool es3 = (strncmp((const char *)glGetString(GL_VERSION), "OpenGL ES 3", 11) == 0);
    //vInfo("es3 =" << es3 ? "TRUE" : "FALSE");

    GLint MaxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTextureSize);
    //vInfo("GL_MAX_TEXTURE_SIZE =" << MaxTextureSize);

    GLint MaxVertexUniformVectors = 0;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &MaxVertexUniformVectors);
    //vInfo("GL_MAX_VERTEX_UNIFORM_VECTORS =" << MaxVertexUniformVectors);

    GLint MaxFragmentUniformVectors = 0;
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &MaxFragmentUniformVectors);
    //vInfo("GL_MAX_FRAGMENT_UNIFORM_VECTORS =" << MaxFragmentUniformVectors);
}

bool VGlOperation::glIsExtensionString(const char *extension, const char *allExtensions)
{
    if (strstr(allExtensions, extension)) {
        //vInfo("Found:" << extension);
        return true;
    } else {
        //vInfo("Not found:" << extension);
        return false;
    }
}

void VGlOperation::glFinish()
{
    const EGLint wait = glWaitforFlush(100000000);
    if (wait == EGL_TIMEOUT_EXPIRED_KHR) {
        //vInfo("EGL_TIMEOUT_EXPIRED_KHR");
    }

    if ( wait == EGL_FALSE ) {
        //vInfo("eglClientWaitSyncKHR returned EGL_FALSE");
    }
}

void VGlOperation::glFlush()
{
    const EGLint wait = glWaitforFlush(0);
    if (wait == EGL_FALSE) {
        //vInfo("eglClientWaitSyncKHR returned EGL_FALSE");
    }
}

void VGlOperation::glDisableFramebuffer(const bool colorBuffer, const bool depthBuffer)
{
    const int offset = static_cast<int>(!colorBuffer);
    const int count = static_cast<int>(colorBuffer) + (static_cast<int>(depthBuffer))*2;

    const GLenum fboAttachments[3] = {GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    glInvalidateFramebuffer(GL_FRAMEBUFFER, count, fboAttachments + offset);
}





EGLConfig VGlOperation::chooseColorConfig( const EGLDisplay display, const int redBits,
                                           const int greeBits, const int blueBits, const int depthBits, const int samples, const bool pbuffer )
{
    static const int MAX_CONFIGS = 1024;
    EGLConfig configs[MAX_CONFIGS];
    EGLint numConfigs = 0;

    if (EGL_FALSE == eglGetConfigs(display,
                                     configs, MAX_CONFIGS, &numConfigs)) {
        vWarn("eglGetConfigs() failed");
        return NULL;
    }
    //vInfo("eglGetConfigs() =" << numConfigs << "configs");

    const EGLint configAttribs[] = {
        EGL_BLUE_SIZE,  	blueBits,
        EGL_GREEN_SIZE, 	greeBits,
        EGL_RED_SIZE,   	redBits,
        EGL_DEPTH_SIZE,   	depthBits,
        EGL_SAMPLES,		samples,
        EGL_NONE
    };

    for (int esVersion = 3 ; esVersion >= 2 ; esVersion--) {
        for (int i = 0; i < numConfigs; i++) {
            EGLint	value = 0;

            eglGetConfigAttrib(display, configs[i], EGL_RENDERABLE_TYPE, &value);

            if ((esVersion == 2) && (value & EGL_OPENGL_ES2_BIT) != EGL_OPENGL_ES2_BIT) {
                continue;
            }

            if ((esVersion == 3) && ( value & EGL_OPENGL_ES3_BIT_KHR ) != EGL_OPENGL_ES3_BIT_KHR) {
                continue;
            }

            eglGetConfigAttrib(display, configs[i], EGL_SURFACE_TYPE , &value);
            const int surfs = EGL_WINDOW_BIT | (pbuffer ? EGL_PBUFFER_BIT : 0);
            if ((value & surfs) != surfs) {
                continue;
            }

            int	j = 0;
            for (; configAttribs[j] != EGL_NONE ; j += 2) {
                EGLint	value = 0;
                eglGetConfigAttrib(display, configs[i], configAttribs[j], &value);
                if (value != configAttribs[j+1]) {
                    break;
                }
            }
            if (configAttribs[j] == EGL_NONE) {
                //vInfo("Got an ES" << esVersion << "renderable config:" << (int)configs[i]);
                return configs[i];
            }
        }
    }
    return NULL;
}

void VGlOperation::eglInit( const EGLContext shareContext,
                             const int requestedGlEsVersion,
                             const int redBits, const int greenBits, const int blueBits,
                             const int depthBits, const int multisamples, const GLuint contextPriority )
{
    VGlOperation glOperation;
    //LOG("EglSetup: requestGlEsVersion(%d), redBits(%d), greenBits(%d), blueBits(%d), depthBits(%d), multisamples(%d), contextPriority(%d)",
//         requestedGlEsVersion, redBits, greenBits, blueBits, depthBits, multisamples, contextPriority);

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    EGLint majorVersion;
    EGLint minorVersion;
    eglInitialize(display, &majorVersion, &minorVersion);
    config = chooseColorConfig(display, redBits, greenBits, blueBits, depthBits, multisamples, true);
    if (config == 0) {
        vFatal("No acceptable EGL color configs.");
        return ;
    }

    for (int version = requestedGlEsVersion ; version >= 2 ; version--)
    {

        EGLint contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, version,
            EGL_NONE, EGL_NONE,
            EGL_NONE };

        if (contextPriority != EGL_CONTEXT_PRIORITY_MEDIUM_IMG) {
            contextAttribs[2] = EGL_CONTEXT_PRIORITY_LEVEL_IMG;
            contextAttribs[3] = contextPriority;
        }

        context = eglCreateContext(display, config, shareContext, contextAttribs);
        if (context != EGL_NO_CONTEXT) {
            //vInfo("Succeeded.");
            glEsVersion = version;

            EGLint configIDReadback;
            if (!eglQueryContext(display, context, EGL_CONFIG_ID, &configIDReadback)) {
                //vWarn("eglQueryContext EGL_CONFIG_ID failed");
            }
            break;
        }
    }
    if (context == EGL_NO_CONTEXT) {
        vWarn("eglCreateContext failed:" << glOperation.getEglErrorString());
        return ;
    }

    if (contextPriority != EGL_CONTEXT_PRIORITY_MEDIUM_IMG) {
        EGLint actualPriorityLevel;
        eglQueryContext(display, context, EGL_CONTEXT_PRIORITY_LEVEL_IMG, &actualPriorityLevel);
        switch (actualPriorityLevel) {
        case EGL_CONTEXT_PRIORITY_HIGH_IMG: //vInfo("Context is EGL_CONTEXT_PRIORITY_HIGH_IMG");
            break;
        case EGL_CONTEXT_PRIORITY_MEDIUM_IMG: //vInfo("Context is EGL_CONTEXT_PRIORITY_MEDIUM_IMG");
            break;
        case EGL_CONTEXT_PRIORITY_LOW_IMG: //vInfo("Context is EGL_CONTEXT_PRIORITY_LOW_IMG");
            break;
        default: //vInfo("Context has unknown priority level");
            break;
        }
    }
    const EGLint attrib_list[] =
    {
        EGL_WIDTH, 16,
        EGL_HEIGHT, 16,
        EGL_NONE
    };
    pbufferSurface = eglCreatePbufferSurface(display, config, attrib_list);

    if (pbufferSurface == EGL_NO_SURFACE) {
        vWarn("eglCreatePbufferSurface failed:" << glOperation.getEglErrorString());
        eglDestroyContext(display, context);
        context = EGL_NO_CONTEXT;
        return ;
    }

    if (eglMakeCurrent(display, pbufferSurface, pbufferSurface, context) == EGL_FALSE) {
        vWarn("eglMakeCurrent pbuffer failed:" << glOperation.getEglErrorString());
        eglDestroySurface(display, pbufferSurface);
        eglDestroyContext(display, context);
        context = EGL_NO_CONTEXT;
        return ;
    }

    gpuType = glOperation.eglGetGpuType();

    return ;
}

void VGlOperation::eglExit(  )
{
    VGlOperation glOperation;
    if (eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE) {
        vFatal("eglMakeCurrent: failed:" << glOperation.getEglErrorString());
    }

    if (eglDestroyContext(display, context) == EGL_FALSE) {
        vFatal("eglDestroyContext: failed:" << glOperation.getEglErrorString());
    }

    if (eglDestroySurface(display, pbufferSurface) == EGL_FALSE) {
        vFatal("eglDestroySurface: failed:" << glOperation.getEglErrorString());
    }

    glEsVersion = 0;
    gpuType = GPU_TYPE_UNKNOWN;
    display = 0;
    pbufferSurface = 0;
    config = 0;
    context = 0;
}

void VGlOperation::glFramebufferTexture2DMultisampleIMG(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples)
{
    PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG glFramebufferTexture2DMultisampleIMG_ = NULL;
    if (glIsExtensionString("GL_IMG_multisampled_render_to_texture", extensions)) {
        glFramebufferTexture2DMultisampleIMG_ = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG)getExtensionProc ("glFramebufferTexture2DMultisampleIMG");
    } else if (glIsExtensionString("GL_EXT_multisampled_render_to_texture", extensions)) {
        glFramebufferTexture2DMultisampleIMG_ = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG)getExtensionProc ("glFramebufferTexture2DMultisampleEXT");
    }
    glFramebufferTexture2DMultisampleIMG_(target, attachment, textarget, texture, level, samples);
}

void VGlOperation::glRenderbufferStorageMultisampleIMG(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG glRenderbufferStorageMultisampleIMG_ = NULL;
    if (glIsExtensionString("GL_IMG_multisampled_render_to_texture", extensions)) {
        glRenderbufferStorageMultisampleIMG_ = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG)getExtensionProc ("glRenderbufferStorageMultisampleIMG");
    }
    else if (glIsExtensionString("GL_EXT_multisampled_render_to_texture", extensions)) {
        // assign to the same function pointers as the IMG extension
        glRenderbufferStorageMultisampleIMG_ = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG)getExtensionProc ("glRenderbufferStorageMultisampleEXT");
    }
    glRenderbufferStorageMultisampleIMG_(target, samples, internalformat, width, height);
}

EGLSyncKHR VGlOperation::eglCreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list)
{
    PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR_;
    eglCreateSyncKHR_ = (PFNEGLCREATESYNCKHRPROC)getExtensionProc("eglCreateSyncKHR");
    return eglCreateSyncKHR_(dpy, type, attrib_list);
}

EGLBoolean VGlOperation::eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync)
{
    PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR_;
    eglDestroySyncKHR_ = (PFNEGLDESTROYSYNCKHRPROC)getExtensionProc("eglDestroySyncKHR");
    return eglDestroySyncKHR_(dpy, sync);
}

EGLint VGlOperation::eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout)
{
    PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR_;
    eglClientWaitSyncKHR_ = (PFNEGLCLIENTWAITSYNCKHRPROC)getExtensionProc("eglClientWaitSyncKHR");
    return eglClientWaitSyncKHR_(dpy, sync, flags, timeout);
}

void VGlOperation::glBindVertexArrayOES(GLuint array)
{
    PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOES_;
    if ( glIsExtensionString("GL_OES_vertex_array_object", extensions)) {
        glBindVertexArrayOES_ = (PFNGLBINDVERTEXARRAYOESPROC)eglGetProcAddress("glBindVertexArrayOES");
        glBindVertexArrayOES_(array);
    }
}

void VGlOperation::glDeleteVertexArraysOES(GLsizei n, const GLuint *arrays)
{
    PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOES_;
    if (glIsExtensionString("GL_OES_vertex_array_object", extensions)) {
        glDeleteVertexArraysOES_ = (PFNGLDELETEVERTEXARRAYSOESPROC)eglGetProcAddress("glDeleteVertexArraysOES");
        glDeleteVertexArraysOES_(n, arrays);
    }
}

void VGlOperation::glGenVertexArraysOES(GLsizei n, GLuint *arrays)
{
    PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOES_;
    if (glIsExtensionString("GL_OES_vertex_array_object", extensions)) {
        glGenVertexArraysOES_ = (PFNGLGENVERTEXARRAYSOESPROC)eglGetProcAddress("glGenVertexArraysOES");
        glGenVertexArraysOES_(n, arrays);
    }
}

void VGlOperation::glStartTilingQCOM(GLuint x, GLuint y, GLuint width, GLuint height, GLbitfield preserveMask)
{
    PFNGLSTARTTILINGQCOMPROC glStartTilingQCOM_;
    if (glIsExtensionString("GL_QCOM_tiled_rendering", extensions)) {
        glStartTilingQCOM_ = (PFNGLSTARTTILINGQCOMPROC)eglGetProcAddress("glStartTilingQCOM");
        glStartTilingQCOM_(x, y, width, height, preserveMask);
    }
}

void VGlOperation::glEndTilingQCOM(GLbitfield preserveMask)
{
    PFNGLENDTILINGQCOMPROC glEndTilingQCOM_;
    if (glIsExtensionString("GL_QCOM_tiled_rendering", extensions)) {
        glEndTilingQCOM_ = (PFNGLENDTILINGQCOMPROC)eglGetProcAddress("glEndTilingQCOM");
        glEndTilingQCOM_(preserveMask);
    }
}

void VGlOperation::glGenQueriesEXT(GLsizei n, GLuint *ids)
{
    typedef void (GL_APIENTRYP PFNGLGENQUERIESEXTPROC) (GLsizei n, GLuint *ids);
    PFNGLGENQUERIESEXTPROC glGenQueriesEXT_;
    if (glIsExtensionString("GL_EXT_disjoint_timer_query", extensions)) {
        glGenQueriesEXT_ = (PFNGLGENQUERIESEXTPROC)eglGetProcAddress("glGenQueriesEXT");
        glGenQueriesEXT_(n, ids);
    }
}

void VGlOperation::glDeleteQueriesEXT(GLsizei n, const GLuint *ids)
{
    typedef void (GL_APIENTRYP PFNGLDELETEQUERIESEXTPROC) (GLsizei n, const GLuint *ids);
    PFNGLDELETEQUERIESEXTPROC glDeleteQueriesEXT_;
    if (glIsExtensionString("GL_EXT_disjoint_timer_query", extensions)) {
        glDeleteQueriesEXT_ = (PFNGLDELETEQUERIESEXTPROC)eglGetProcAddress("glDeleteQueriesEXT");
        glDeleteQueriesEXT_(n, ids);
    }
}

void VGlOperation::glBeginQueryEXT(GLenum target, GLuint id)
{
    typedef void (GL_APIENTRYP PFNGLBEGINQUERYEXTPROC) (GLenum target, GLuint id);
    PFNGLBEGINQUERYEXTPROC glBeginQueryEXT_;
    if (glIsExtensionString("GL_EXT_disjoint_timer_query", extensions)) {
        glBeginQueryEXT_ = (PFNGLBEGINQUERYEXTPROC)eglGetProcAddress("glBeginQueryEXT");
        glBeginQueryEXT_(target, id);
    }
}

void VGlOperation::glEndQueryEXT(GLenum target)
{
    typedef void (GL_APIENTRYP PFNGLENDQUERYEXTPROC) (GLenum target);
    PFNGLENDQUERYEXTPROC glEndQueryEXT_;
    if (glIsExtensionString("GL_EXT_disjoint_timer_query", extensions)) {
        glEndQueryEXT_ = (PFNGLENDQUERYEXTPROC)eglGetProcAddress("glEndQueryEXT");
        glEndQueryEXT_(target);
    }
}

void VGlOperation::glQueryCounterEXT(GLuint id, GLenum target)
{
    typedef void (GL_APIENTRYP PFNGLQUERYCOUNTEREXTPROC) (GLuint id, GLenum target);
    PFNGLQUERYCOUNTEREXTPROC glQueryCounterEXT_;
    if (glIsExtensionString("GL_EXT_disjoint_timer_query", extensions)) {
        glQueryCounterEXT_ = (PFNGLQUERYCOUNTEREXTPROC)eglGetProcAddress("glQueryCounterEXT");
        glQueryCounterEXT_(id, target);
    }
}

void VGlOperation::glGetQueryObjectivEXT(GLuint id, GLenum pname, GLint *params)
{
    typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTIVEXTPROC) (GLuint id, GLenum pname, GLint *params);
    PFNGLGETQUERYOBJECTIVEXTPROC glGetQueryObjectivEXT_;
    if (glIsExtensionString("GL_EXT_disjoint_timer_query", extensions)) {
        glGetQueryObjectivEXT_ = (PFNGLGETQUERYOBJECTIVEXTPROC)eglGetProcAddress("glGetQueryObjectivEXT");
        glGetQueryObjectivEXT_(id, pname, params);
    }
}

void VGlOperation::glGetQueryObjectui64vEXT(GLuint id, GLenum pname, GLuint64 *params)
{
    typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTUI64VEXTPROC) (GLuint id, GLenum pname, GLuint64 *params);
    PFNGLGETQUERYOBJECTUI64VEXTPROC glGetQueryObjectui64vEXT_;
    if (glIsExtensionString("GL_EXT_disjoint_timer_query", extensions)) {
        glGetQueryObjectui64vEXT_  = (PFNGLGETQUERYOBJECTUI64VEXTPROC)eglGetProcAddress("glGetQueryObjectui64vEXT");
        glGetQueryObjectui64vEXT_(id, pname, params);
    }
}

void VGlOperation::glGetInteger64v(GLenum pname, GLint64 *params)
{
    typedef void (GL_APIENTRYP PFNGLGETINTEGER64VPROC) (GLenum pname, GLint64 *params);
    PFNGLGETINTEGER64VPROC glGetInteger64v_;
    if (glIsExtensionString("GL_EXT_disjoint_timer_query", extensions)) {
        glGetInteger64v_  = (PFNGLGETINTEGER64VPROC)eglGetProcAddress("glGetInteger64v");
        glGetInteger64v_(pname, params);
    }
}

void VGlOperation::glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
    typedef void (GL_APIENTRYP PFNGLBLITFRAMEBUFFER_) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
    PFNGLBLITFRAMEBUFFER_ glBlitFramebuffer_;
    glBlitFramebuffer_  = (PFNGLBLITFRAMEBUFFER_)eglGetProcAddress("glBlitFramebuffer");
    glBlitFramebuffer_(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

void  VGlOperation::glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum* attachments)
{
    typedef void (GL_APIENTRYP PFNGLINVALIDATEFRAMEBUFFER_) (GLenum target, GLsizei numAttachments, const GLenum* attachments);
    PFNGLINVALIDATEFRAMEBUFFER_ glInvalidateFramebuffer_;
    glInvalidateFramebuffer_  = (PFNGLINVALIDATEFRAMEBUFFER_)eglGetProcAddress("glInvalidateFramebuffer");
    glInvalidateFramebuffer_(target, numAttachments, attachments);
}

NV_NAMESPACE_END
