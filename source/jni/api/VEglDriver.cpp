#include "VEglDriver.h"

NV_NAMESPACE_BEGIN



VEglDriver::GpuType VEglDriver::eglGetGpuType()
{

    GpuType gpuType;
    EGLContext mcontext = eglGetCurrentContext();
    if (mcontext == EGL_NO_CONTEXT)
        return GPU_TYPE_UNKNOWN;
    else
    {
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

    return gpuType;
    }
}


VEglDriver::VEglDriver()
{

    m_glEsVersion = 0;
    m_gpuType = GPU_TYPE_UNKNOWN;
    m_display = EGL_NO_DISPLAY;
    m_pbufferSurface = EGL_NO_SURFACE;
    m_config = 0 ;
    m_context = EGL_NO_CONTEXT;
}

void VEglDriver::updateEglConfig(EGLContext eglShareContext)
{

EGLint configID;

if ( !eglQueryContext(eglGetCurrentDisplay(), eglShareContext, EGL_CONFIG_ID, &configID ) )
{
    vFatal("eglQueryContext EGL_CONFIG_ID failed");
}


m_config = eglConfigForConfigID(configID );
if ( m_config == NULL )
{
    vFatal("updateEglConfig failed");
}
}
void VEglDriver::updateEglConfig()
{

EGLint configID;

if ( !eglQueryContext(m_display, m_context, EGL_CONFIG_ID, &configID ) )
{
    vFatal("eglQueryContext EGL_CONFIG_ID failed");
}


m_config = eglConfigForConfigID(configID );
if ( m_config == NULL )
{
    vFatal("updateEglConfig failed");
}
}

EGLConfig VEglDriver::eglConfigForConfigID(const GLint configID)
{

    if(m_display == EGL_NO_DISPLAY)
     m_display = eglGetCurrentDisplay();


    static const int MAX_CONFIGS = 1024;
    EGLConfig configs[MAX_CONFIGS];
    EGLint numConfigs = 0;

    if (EGL_FALSE == eglGetConfigs(m_display,
                                   configs,
                                   MAX_CONFIGS,
                                   &numConfigs)) {
      return NULL;
    }

    for (int i = 0; i < numConfigs; i++) {
        EGLint	value = 0;

        eglGetConfigAttrib(m_display, configs[i], EGL_CONFIG_ID, &value);
        if (value == configID) {
            return configs[i];
        }
    }

    return NULL;
}
const char *VEglDriver::getEglErrorString()
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

const char * VEglDriver::getGlErrorEnum(const GLenum e)
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

bool VEglDriver::logErrorsEnum(const char *logTitle)
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

EGLint VEglDriver::glWaitforFlush(int timeout)
{


    const EGLSyncKHR sync = eglCreateSyncKHR(eglGetCurrentDisplay(), EGL_SYNC_FENCE_KHR, NULL);
    if (sync == EGL_NO_SYNC_KHR) {
        return EGL_FALSE;
    }

    const EGLint wait = eglClientWaitSyncKHR(eglGetCurrentDisplay(), sync, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, timeout);

    eglDestroySyncKHR(eglGetCurrentDisplay(), sync);

    return wait;
}

void * VEglDriver::getExtensionProc( const char * name )
{
    void * ptr = reinterpret_cast<void*>(eglGetProcAddress(name));
    if (!ptr) {
        //vInfo("NOT FOUND:" << name);
    }
    return ptr;
}

void VEglDriver::logExtensions()
{

/*
    GLint MaxTextureSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTextureSize);
    //vInfo("GL_MAX_TEXTURE_SIZE =" << MaxTextureSize);

    GLint MaxVertexUniformVectors = 0;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, &MaxVertexUniformVectors);
    //vInfo("GL_MAX_VERTEX_UNIFORM_VECTORS =" << MaxVertexUniformVectors);

    GLint MaxFragmentUniformVectors = 0;
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, &MaxFragmentUniformVectors);
    //vInfo("GL_MAX_FRAGMENT_UNIFORM_VECTORS =" << MaxFragmentUniformVectors);
    */
}

bool VEglDriver::glIsExtensionString(const char *extension)
{


   static const char *extensions = initExtensions();
    if(extensions)
    {


    if (strstr(extensions, extension)) {
       // vInfo("Found:" << extension);
       // vInfo("gl strstr end");
        return true;
    } else {
        //vInfo("Not found:" << extension);
        return false;
    }
    }
    else
    {
       // vInfo("there is context for extensions");
        return false;
    }
}

void VEglDriver::glFinish()
{
    const EGLint wait = glWaitforFlush(100000000);
    if (wait == EGL_TIMEOUT_EXPIRED_KHR) {
        //vInfo("EGL_TIMEOUT_EXPIRED_KHR");
    }

    if ( wait == EGL_FALSE ) {
        //vInfo("eglClientWaitSyncKHR returned EGL_FALSE");
    }
}

void VEglDriver::glFlush()
{
    const EGLint wait = glWaitforFlush(0);
    if (wait == EGL_FALSE) {
        //vInfo("eglClientWaitSyncKHR returned EGL_FALSE");
    }
}

void VEglDriver::glDisableFramebuffer(const bool colorBuffer, const bool depthBuffer)
{
    const int offset = static_cast<int>(!colorBuffer);
    const int count = static_cast<int>(colorBuffer) + (static_cast<int>(depthBuffer))*2;

    const GLenum fboAttachments[3] = {GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
    glInvalidateFramebuffer(GL_FRAMEBUFFER, count, fboAttachments + offset);
}





EGLConfig VEglDriver::chooseColorConfig( const int redBits,
                                           const int greeBits, const int blueBits, const int depthBits, const int samples, const bool pbuffer )
{
    static const int MAX_CONFIGS = 1024;
    EGLConfig configs[MAX_CONFIGS];
    EGLint numConfigs = 0;
    if(m_display == EGL_NO_DISPLAY)
       m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    if (EGL_FALSE == eglGetConfigs(m_display,
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

            eglGetConfigAttrib(m_display, configs[i], EGL_RENDERABLE_TYPE, &value);

            if ((esVersion == 2) && (value & EGL_OPENGL_ES2_BIT) != EGL_OPENGL_ES2_BIT) {
                continue;
            }

            if ((esVersion == 3) && ( value & EGL_OPENGL_ES3_BIT_KHR ) != EGL_OPENGL_ES3_BIT_KHR) {
                continue;
            }

            eglGetConfigAttrib(m_display, configs[i], EGL_SURFACE_TYPE , &value);
            const int surfs = EGL_WINDOW_BIT | (pbuffer ? EGL_PBUFFER_BIT : 0);
            if ((value & surfs) != surfs) {
                continue;
            }

            int	j = 0;
            for (; configAttribs[j] != EGL_NONE ; j += 2) {
                EGLint	value = 0;
                eglGetConfigAttrib(m_display, configs[i], configAttribs[j], &value);
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
 void VEglDriver::updateDisplay()
 {

   m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   if (m_display == 0) {
       vFatal("No acceptable display ( vegldriver:updateDisplay.");
       return ;
   }

 }

 const char * VEglDriver::initExtensions()
  {

      EGLContext mcontext= eglGetCurrentContext();

   if (mcontext != EGL_NO_CONTEXT)
   {

       const char* extensions = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));

      if (NULL == extensions)
      {
          vWarn("glGetString( GL_EXTENSIONS ) returned NULL");
          return NULL;
      }
      return extensions;
   }
   else
       return NULL;
  }

bool VEglDriver::eglInit( const EGLContext shareContext,
                             const int requestedGlEsVersion,
                             const int redBits, const int greenBits, const int blueBits,
                             const int depthBits, const int multisamples, const GLuint contextPriority )
{

    vInfo("egl init ");
    if (m_context == EGL_NO_CONTEXT)
    {
        if (m_display == EGL_NO_DISPLAY) {
            m_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        }

        EGLint majorVersion;
        EGLint minorVersion;
        eglInitialize(m_display, &majorVersion, &minorVersion);
        m_config = chooseColorConfig(redBits, greenBits, blueBits, depthBits, multisamples, true);
        if (m_config == 0) {
            vFatal("No acceptable EGL color configs.");
            return false;
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

            m_context = eglCreateContext(m_display,m_config, shareContext, contextAttribs);
            if (m_context != EGL_NO_CONTEXT) {
                m_glEsVersion = version;
                vInfo("glEsVersion:"<< m_glEsVersion);
                EGLint configIDReadback;
                if (!eglQueryContext(m_display, m_context, EGL_CONFIG_ID, &configIDReadback)) {
                  vWarn("eglQueryContext EGL_CONFIG_ID failed");
                }
                break;
            }
        }

        if (m_context == EGL_NO_CONTEXT) {
            vWarn("eglCreateContext failed:" << getEglErrorString());
            return false;
        }

        if (m_pbufferSurface == EGL_NO_SURFACE)
        {
            const EGLint attrib_list[] =
            {
            EGL_WIDTH, 16,
            EGL_HEIGHT, 16,
            EGL_NONE
            };
            m_pbufferSurface = eglCreatePbufferSurface(m_display, m_config, attrib_list);

            if (m_pbufferSurface == EGL_NO_SURFACE)
            {
            vWarn("eglCreatePbufferSurface failed:" << getEglErrorString());
            eglDestroyContext(m_display, m_context);
            m_context = EGL_NO_CONTEXT;
            return false;
            }
        }

        if (eglMakeCurrent(m_display, m_pbufferSurface, m_pbufferSurface, m_context) == EGL_FALSE)
        {
            vWarn("eglMakeCurrent pbuffer failed:" << getEglErrorString());
            eglDestroySurface(m_display, m_pbufferSurface);
            eglDestroyContext(m_display, m_context);
            m_context = EGL_NO_CONTEXT;
            return false;
        }

        m_extensions = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
        if (NULL == m_extensions) {
            vInfo("glGetString( GL_EXTENSIONS ) returned NULL");
        }

        m_gpuType = eglGetGpuType();
        return true;
    }

    return false;
}


void VEglDriver::eglExit()
{

    if( m_display)
    {
         if (eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE) {
        vFatal("eglMakeCurrent: failed:" << getEglErrorString());
        }

    }
    if(m_context != EGL_NO_CONTEXT)
    {
        if (eglDestroyContext(m_display, m_context) == EGL_FALSE)
        {
        vFatal("eglDestroyContext: failed:" << getEglErrorString());
        }
         m_context = EGL_NO_CONTEXT;
    }
    if(m_pbufferSurface != EGL_NO_SURFACE)
    {
        if (eglDestroySurface(m_display, m_pbufferSurface) == EGL_FALSE)
        {
        vFatal("eglDestroySurface: failed:" << getEglErrorString());
        }
        m_pbufferSurface = EGL_NO_SURFACE;
    }
     m_display = 0;

}
 VEglDriver::~VEglDriver(  )
{

    if( m_display)
    {
         if (eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE) {
        vFatal("eglMakeCurrent: failed:" << getEglErrorString());
        }

    }
    if(m_context != EGL_NO_CONTEXT)
    {
        if (eglDestroyContext(m_display, m_context) == EGL_FALSE)
        {
        vFatal("eglDestroyContext: failed:" << getEglErrorString());
        }
         m_context = EGL_NO_CONTEXT;
    }
    if(m_pbufferSurface != EGL_NO_SURFACE)
    {
        if (eglDestroySurface(m_display, m_pbufferSurface) == EGL_FALSE)
        {
        vFatal("eglDestroySurface: failed:" << getEglErrorString());
        }
        m_pbufferSurface = EGL_NO_SURFACE;
    }

    m_glEsVersion = 0;
    m_gpuType = GPU_TYPE_UNKNOWN;
    m_config = 0;
     m_display = 0;

}

void VEglDriver::glFramebufferTexture2DMultisampleIMG(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLsizei samples)
{

    PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG glFramebufferTexture2DMultisampleIMG_ = NULL;
    if (glIsExtensionString("GL_IMG_multisampled_render_to_texture")) {
        glFramebufferTexture2DMultisampleIMG_ = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG)getExtensionProc ("glFramebufferTexture2DMultisampleIMG");
    } else if (glIsExtensionString("GL_EXT_multisampled_render_to_texture")) {
        glFramebufferTexture2DMultisampleIMG_ = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG)getExtensionProc ("glFramebufferTexture2DMultisampleEXT");
    }
    glFramebufferTexture2DMultisampleIMG_(target, attachment, textarget, texture, level, samples);
}

void VEglDriver::glRenderbufferStorageMultisampleIMG(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{

    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG glRenderbufferStorageMultisampleIMG_ = NULL;
    if (glIsExtensionString("GL_IMG_multisampled_render_to_texture")) {
        glRenderbufferStorageMultisampleIMG_ = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG)getExtensionProc ("glRenderbufferStorageMultisampleIMG");
    }
    else if (glIsExtensionString("GL_EXT_multisampled_render_to_texture")) {
        // assign to the same function pointers as the IMG extension
        glRenderbufferStorageMultisampleIMG_ = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG)getExtensionProc ("glRenderbufferStorageMultisampleEXT");
    }
    glRenderbufferStorageMultisampleIMG_(target, samples, internalformat, width, height);
}

EGLSyncKHR VEglDriver::eglCreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list)
{
    PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR_;
    eglCreateSyncKHR_ = (PFNEGLCREATESYNCKHRPROC)getExtensionProc("eglCreateSyncKHR");
    return eglCreateSyncKHR_(dpy, type, attrib_list);
}

EGLBoolean VEglDriver::eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync)
{
    PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR_;
    eglDestroySyncKHR_ = (PFNEGLDESTROYSYNCKHRPROC)getExtensionProc("eglDestroySyncKHR");
    return eglDestroySyncKHR_(dpy, sync);
}

EGLint VEglDriver::eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout)
{
    PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR_;
    eglClientWaitSyncKHR_ = (PFNEGLCLIENTWAITSYNCKHRPROC)getExtensionProc("eglClientWaitSyncKHR");
    return eglClientWaitSyncKHR_(dpy, sync, flags, timeout);
}

void VEglDriver::glBindVertexArrayOES(GLuint array)
{
    PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOES_;
    if ( glIsExtensionString("GL_OES_vertex_array_object")) {
        glBindVertexArrayOES_ = (PFNGLBINDVERTEXARRAYOESPROC)eglGetProcAddress("glBindVertexArrayOES");
        glBindVertexArrayOES_(array);
    }
}

void VEglDriver::glDeleteVertexArraysOES(GLsizei n, const GLuint *arrays)
{
    PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOES_;
    if (glIsExtensionString("GL_OES_vertex_array_object")) {
        glDeleteVertexArraysOES_ = (PFNGLDELETEVERTEXARRAYSOESPROC)eglGetProcAddress("glDeleteVertexArraysOES");
        glDeleteVertexArraysOES_(n, arrays);
    }
}

void VEglDriver::glGenVertexArraysOES(GLsizei n, GLuint *arrays)
{
    PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOES_;
    if (glIsExtensionString("GL_OES_vertex_array_object")) {
        glGenVertexArraysOES_ = (PFNGLGENVERTEXARRAYSOESPROC)eglGetProcAddress("glGenVertexArraysOES");
        glGenVertexArraysOES_(n, arrays);
    }
}

void VEglDriver::glStartTilingQCOM(GLuint x, GLuint y, GLuint width, GLuint height, GLbitfield preserveMask)
{
    PFNGLSTARTTILINGQCOMPROC glStartTilingQCOM_;
    if (glIsExtensionString("GL_QCOM_tiled_rendering")) {
        glStartTilingQCOM_ = (PFNGLSTARTTILINGQCOMPROC)eglGetProcAddress("glStartTilingQCOM");
        glStartTilingQCOM_(x, y, width, height, preserveMask);
    }
}

void VEglDriver::glEndTilingQCOM(GLbitfield preserveMask)
{
    PFNGLENDTILINGQCOMPROC glEndTilingQCOM_;
    if (glIsExtensionString("GL_QCOM_tiled_rendering")) {
        glEndTilingQCOM_ = (PFNGLENDTILINGQCOMPROC)eglGetProcAddress("glEndTilingQCOM");
        glEndTilingQCOM_(preserveMask);
    }
}

void VEglDriver::glGenQueriesEXT(GLsizei n, GLuint *ids)
{
    typedef void (GL_APIENTRYP PFNGLGENQUERIESEXTPROC) (GLsizei n, GLuint *ids);
    PFNGLGENQUERIESEXTPROC glGenQueriesEXT_;
    if (glIsExtensionString("GL_EXT_disjoint_timer_query")) {
        glGenQueriesEXT_ = (PFNGLGENQUERIESEXTPROC)eglGetProcAddress("glGenQueriesEXT");
        glGenQueriesEXT_(n, ids);
    }
}

void VEglDriver::glDeleteQueriesEXT(GLsizei n, const GLuint *ids)
{
    typedef void (GL_APIENTRYP PFNGLDELETEQUERIESEXTPROC) (GLsizei n, const GLuint *ids);
    PFNGLDELETEQUERIESEXTPROC glDeleteQueriesEXT_;
    if (glIsExtensionString("GL_EXT_disjoint_timer_query")) {
        glDeleteQueriesEXT_ = (PFNGLDELETEQUERIESEXTPROC)eglGetProcAddress("glDeleteQueriesEXT");
        glDeleteQueriesEXT_(n, ids);
    }
}

void VEglDriver::glBeginQueryEXT(GLenum target, GLuint id)
{
    typedef void (GL_APIENTRYP PFNGLBEGINQUERYEXTPROC) (GLenum target, GLuint id);
    PFNGLBEGINQUERYEXTPROC glBeginQueryEXT_;
    if (glIsExtensionString("GL_EXT_disjoint_timer_query")) {
        glBeginQueryEXT_ = (PFNGLBEGINQUERYEXTPROC)eglGetProcAddress("glBeginQueryEXT");
        glBeginQueryEXT_(target, id);
    }
}

void VEglDriver::glEndQueryEXT(GLenum target)
{
    typedef void (GL_APIENTRYP PFNGLENDQUERYEXTPROC) (GLenum target);
    PFNGLENDQUERYEXTPROC glEndQueryEXT_;
    if (glIsExtensionString("GL_EXT_disjoint_timer_query")) {
        glEndQueryEXT_ = (PFNGLENDQUERYEXTPROC)eglGetProcAddress("glEndQueryEXT");
        glEndQueryEXT_(target);
    }
}

void VEglDriver::glQueryCounterEXT(GLuint id, GLenum target)
{
    typedef void (GL_APIENTRYP PFNGLQUERYCOUNTEREXTPROC) (GLuint id, GLenum target);
    PFNGLQUERYCOUNTEREXTPROC glQueryCounterEXT_;
    if (glIsExtensionString("GL_EXT_disjoint_timer_query")) {
        glQueryCounterEXT_ = (PFNGLQUERYCOUNTEREXTPROC)eglGetProcAddress("glQueryCounterEXT");
        glQueryCounterEXT_(id, target);
    }
}

void VEglDriver::glGetQueryObjectivEXT(GLuint id, GLenum pname, GLint *params)
{
    typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTIVEXTPROC) (GLuint id, GLenum pname, GLint *params);
    PFNGLGETQUERYOBJECTIVEXTPROC glGetQueryObjectivEXT_;
    if (glIsExtensionString("GL_EXT_disjoint_timer_query")) {
        glGetQueryObjectivEXT_ = (PFNGLGETQUERYOBJECTIVEXTPROC)eglGetProcAddress("glGetQueryObjectivEXT");
        glGetQueryObjectivEXT_(id, pname, params);
    }
}

void VEglDriver::glGetQueryObjectui64vEXT(GLuint id, GLenum pname, GLuint64 *params)
{
    typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTUI64VEXTPROC) (GLuint id, GLenum pname, GLuint64 *params);
    PFNGLGETQUERYOBJECTUI64VEXTPROC glGetQueryObjectui64vEXT_;
    if (glIsExtensionString("GL_EXT_disjoint_timer_query")) {
        glGetQueryObjectui64vEXT_  = (PFNGLGETQUERYOBJECTUI64VEXTPROC)eglGetProcAddress("glGetQueryObjectui64vEXT");
        glGetQueryObjectui64vEXT_(id, pname, params);
    }
}

void VEglDriver::glGetInteger64v(GLenum pname, GLint64 *params)
{
    typedef void (GL_APIENTRYP PFNGLGETINTEGER64VPROC) (GLenum pname, GLint64 *params);
    PFNGLGETINTEGER64VPROC glGetInteger64v_;
    if (glIsExtensionString("GL_EXT_disjoint_timer_query")) {
        glGetInteger64v_  = (PFNGLGETINTEGER64VPROC)eglGetProcAddress("glGetInteger64v");
        glGetInteger64v_(pname, params);
    }
}

void VEglDriver::glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
    typedef void (GL_APIENTRYP PFNGLBLITFRAMEBUFFER_) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
    PFNGLBLITFRAMEBUFFER_ glBlitFramebuffer_;
    glBlitFramebuffer_  = (PFNGLBLITFRAMEBUFFER_)eglGetProcAddress("glBlitFramebuffer");
    glBlitFramebuffer_(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

void  VEglDriver::glInvalidateFramebuffer(GLenum target, GLsizei numAttachments, const GLenum* attachments)
{
    typedef void (GL_APIENTRYP PFNGLINVALIDATEFRAMEBUFFER_) (GLenum target, GLsizei numAttachments, const GLenum* attachments);
    PFNGLINVALIDATEFRAMEBUFFER_ glInvalidateFramebuffer_;
    glInvalidateFramebuffer_  = (PFNGLINVALIDATEFRAMEBUFFER_)eglGetProcAddress("glInvalidateFramebuffer");
    glInvalidateFramebuffer_(target, numAttachments, attachments);
}


static bool isDepthEnabled = false;
static bool isCullEnabled = false;
static bool isBlendEnabled = false;
static bool isScissorEnabled = false;
static bool isStencilEnabled = false;

void VEglDriver::glPushAttrib()
{
    isDepthEnabled = glIsEnabled(GL_DEPTH_TEST);
    isCullEnabled = glIsEnabled(GL_CULL_FACE);
    isBlendEnabled = glIsEnabled(GL_BLEND);
    isScissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
    isStencilEnabled = glIsEnabled(GL_STENCIL_TEST);
}

void VEglDriver::glPopAttrib()
{
    isDepthEnabled?glEnable(GL_DEPTH_TEST):glDisable(GL_DEPTH_TEST);
    isCullEnabled?glEnable(GL_CULL_FACE):glDisable(GL_CULL_FACE);
    isBlendEnabled?glEnable(GL_BLEND):glDisable(GL_BLEND);
    isScissorEnabled?glEnable(GL_SCISSOR_TEST):glDisable(GL_SCISSOR_TEST);
    isStencilEnabled?glEnable(GL_STENCIL_TEST):glDisable(GL_STENCIL_TEST);
}

NV_NAMESPACE_END
