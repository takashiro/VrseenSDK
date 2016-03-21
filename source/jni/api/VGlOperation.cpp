#include "VGlOperation.h"

NV_NAMESPACE_BEGIN

VGlOperation::GpuType VGlOperation::EglGetGpuType()
{
    return EglGetGpuTypeLocal();
}

VGlOperation::GpuType VGlOperation::EglGetGpuTypeLocal()
{
    GpuType gpuType;
    const char * glRendererString = (const char *)glGetString( GL_RENDERER );
    if ( strstr( glRendererString, "Adreno (TM) 420" ) )
    {
        gpuType = GPU_TYPE_ADRENO_420;
    }
    else if ( strstr( glRendererString, "Adreno (TM) 330" ) )
    {
        gpuType = GPU_TYPE_ADRENO_330;
    }
    else if ( strstr( glRendererString, "Adreno" ) )
    {
        gpuType = GPU_TYPE_ADRENO;
    }
    else if ( strstr( glRendererString, "Mali-T760") )
    {
        const VString &hardware = VOsBuild::getString(VOsBuild::Hardware);
        if (hardware == "universal5433") {
            gpuType = GPU_TYPE_MALI_T760_EXYNOS_5433;
        } else if (hardware == "samsungexynos7420") {
            gpuType = GPU_TYPE_MALI_T760_EXYNOS_7420;
        } else {
            gpuType = GPU_TYPE_MALI_T760;
        }
    }
    else if ( strstr( glRendererString, "Mali" ) )
    {
        gpuType = GPU_TYPE_MALI;
    }
    else
    {
        gpuType = GPU_TYPE_UNKNOWN;
    }

    vInfo("SoC:" << VOsBuild::getString(VOsBuild::Hardware));
    vInfo("EglGetGpuType:" << gpuType);

    return gpuType;
}

EGLConfig VGlOperation::EglConfigForConfigID(const EGLDisplay display, const GLint configID)
{
    static const int MAX_CONFIGS = 1024;
    EGLConfig 	configs[MAX_CONFIGS];
    EGLint  	numConfigs = 0;

    if ( EGL_FALSE == eglGetConfigs( display,
            configs, MAX_CONFIGS, &numConfigs ) )
    {
        WARN( "eglGetConfigs() failed" );
        return NULL;
    }

    for ( int i = 0; i < numConfigs; i++ )
    {
        EGLint	value = 0;

        eglGetConfigAttrib( display, configs[i], EGL_CONFIG_ID, &value );
        if ( value == configID )
        {
            return configs[i];
        }
    }

    return NULL;
}
const char *VGlOperation::EglErrorString()
{
    const EGLint err = eglGetError();
    switch( err )
    {
    case EGL_SUCCESS:			return "EGL_SUCCESS";
    case EGL_NOT_INITIALIZED:	return "EGL_NOT_INITIALIZED";
    case EGL_BAD_ACCESS:		return "EGL_BAD_ACCESS";
    case EGL_BAD_ALLOC:			return "EGL_BAD_ALLOC";
    case EGL_BAD_ATTRIBUTE:		return "EGL_BAD_ATTRIBUTE";
    case EGL_BAD_CONTEXT:		return "EGL_BAD_CONTEXT";
    case EGL_BAD_CONFIG:		return "EGL_BAD_CONFIG";
    case EGL_BAD_CURRENT_SURFACE:return "EGL_BAD_CURRENT_SURFACE";
    case EGL_BAD_DISPLAY:		return "EGL_BAD_DISPLAY";
    case EGL_BAD_SURFACE:		return "EGL_BAD_SURFACE";
    case EGL_BAD_MATCH:			return "EGL_BAD_MATCH";
    case EGL_BAD_PARAMETER:		return "EGL_BAD_PARAMETER";
    case EGL_BAD_NATIVE_PIXMAP:	return "EGL_BAD_NATIVE_PIXMAP";
    case EGL_BAD_NATIVE_WINDOW:	return "EGL_BAD_NATIVE_WINDOW";
    case EGL_CONTEXT_LOST:		return "EGL_CONTEXT_LOST";
    default: return "Unknown egl error code";
    }
}

const char * VGlOperation::GL_ErrorForEnum(const GLenum e)
{
    switch( e )
    {
    case GL_NO_ERROR: return "GL_NO_ERROR";
    case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
    default: return "Unknown gl error code";
    }
}

bool VGlOperation::GL_CheckErrors(const char *logTitle)
{
    bool hadError = false;

    // There can be multiple errors that need reporting.
    do
    {
        GLenum err = glGetError();
        if ( err == GL_NO_ERROR )
        {
            break;
        }
        hadError = true;
        WARN( "%s GL Error: %s", logTitle, GL_ErrorForEnum( err ) );
        if ( err == GL_OUT_OF_MEMORY )
        {
            FAIL( "GL_OUT_OF_MEMORY" );
        }
    } while ( 1 );
}

EGLint VGlOperation::GL_FlushSync(int timeout)
{
    // if extension not present, return NO_SYNC
    if ( eglCreateSyncKHR_ == NULL )
    {
        return EGL_FALSE;
    }

    EGLDisplay eglDisplay = eglGetCurrentDisplay();

    const EGLSyncKHR sync = eglCreateSyncKHR_( eglDisplay, EGL_SYNC_FENCE_KHR, NULL );
    if ( sync == EGL_NO_SYNC_KHR )
    {
        return EGL_FALSE;
    }

    const EGLint wait = eglClientWaitSyncKHR_( eglDisplay, sync,
                            EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, timeout );

    eglDestroySyncKHR_( eglDisplay, sync );

    return wait;
}

void * VGlOperation::GetExtensionProc( const char * name )
{
    void * ptr = (void *)eglGetProcAddress( name );
    if ( !ptr )
    {
        LOG( "NOT FOUND: %s", name );
    }
    return ptr;
}

void VGlOperation::GL_FindExtensions()
{
    // get extension pointers
    const char * extensions = (const char *)glGetString( GL_EXTENSIONS );
    if ( NULL == extensions )
    {
        LOG( "glGetString( GL_EXTENSIONS ) returned NULL" );
        return;
    }

    // Unfortunately, the Android log truncates strings over 1024 bytes long,
    // even if there are \n inside, so log each word in the string separately.
    LOG( "GL_EXTENSIONS:" );
    LogStringWords( extensions );

    const bool es3 = ( strncmp( (const char *)glGetString( GL_VERSION ), "OpenGL ES 3", 11 ) == 0 );
    LOG( "es3 = %s", es3 ? "TRUE" : "FALSE" );

    if ( GL_ExtensionStringPresent( "GL_EXT_discard_framebuffer", extensions ) )
    {
        EXT_discard_framebuffer = true;
        glDiscardFramebufferEXT_ = (PFNGLDISCARDFRAMEBUFFEREXTPROC)GetExtensionProc( "glDiscardFramebufferEXT" );
    }

    if ( GL_ExtensionStringPresent( "GL_IMG_multisampled_render_to_texture", extensions ) )
    {
        IMG_multisampled_render_to_texture = true;
        glRenderbufferStorageMultisampleIMG_ = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG)GetExtensionProc ( "glRenderbufferStorageMultisampleIMG" );
        glFramebufferTexture2DMultisampleIMG_ = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG)GetExtensionProc ( "glFramebufferTexture2DMultisampleIMG" );
    }
    else if ( GL_ExtensionStringPresent( "GL_EXT_multisampled_render_to_texture", extensions ) )
    {
        // assign to the same function pointers as the IMG extension
        IMG_multisampled_render_to_texture = true;
        glRenderbufferStorageMultisampleIMG_ = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG)GetExtensionProc ( "glRenderbufferStorageMultisampleEXT" );
        glFramebufferTexture2DMultisampleIMG_ = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG)GetExtensionProc ( "glFramebufferTexture2DMultisampleEXT" );
    }

    eglCreateSyncKHR_ = (PFNEGLCREATESYNCKHRPROC)GetExtensionProc( "eglCreateSyncKHR" );
    eglDestroySyncKHR_ = (PFNEGLDESTROYSYNCKHRPROC)GetExtensionProc( "eglDestroySyncKHR" );
    eglClientWaitSyncKHR_ = (PFNEGLCLIENTWAITSYNCKHRPROC)GetExtensionProc( "eglClientWaitSyncKHR" );
    eglSignalSyncKHR_ = (PFNEGLSIGNALSYNCKHRPROC)GetExtensionProc( "eglSignalSyncKHR" );
    eglGetSyncAttribKHR_ = (PFNEGLGETSYNCATTRIBKHRPROC)GetExtensionProc( "eglGetSyncAttribKHR" );

    if ( GL_ExtensionStringPresent( "GL_OES_vertex_array_object", extensions ) )
    {
        OES_vertex_array_object = true;
        glBindVertexArrayOES_ = (PFNGLBINDVERTEXARRAYOESPROC)eglGetProcAddress("glBindVertexArrayOES");
        glDeleteVertexArraysOES_ = (PFNGLDELETEVERTEXARRAYSOESPROC)eglGetProcAddress("glDeleteVertexArraysOES");
        glGenVertexArraysOES_ = (PFNGLGENVERTEXARRAYSOESPROC)eglGetProcAddress("glGenVertexArraysOES");
        glIsVertexArrayOES_ = (PFNGLISVERTEXARRAYOESPROC)eglGetProcAddress("glIsVertexArrayOES");
    }

    if ( GL_ExtensionStringPresent( "GL_QCOM_tiled_rendering", extensions ) )
    {
        QCOM_tiled_rendering = true;
        glStartTilingQCOM_ = (PFNGLSTARTTILINGQCOMPROC)eglGetProcAddress("glStartTilingQCOM");
        glEndTilingQCOM_ = (PFNGLENDTILINGQCOMPROC)eglGetProcAddress("glEndTilingQCOM");
    }

    // Enabling this seems to cause strange problems in Unity
    if ( GL_ExtensionStringPresent( "GL_EXT_disjoint_timer_query", extensions ) )
    {
        EXT_disjoint_timer_query = true;
        glGenQueriesEXT_ = (PFNGLGENQUERIESEXTPROC)eglGetProcAddress("glGenQueriesEXT");
        glDeleteQueriesEXT_ = (PFNGLDELETEQUERIESEXTPROC)eglGetProcAddress("glDeleteQueriesEXT");
        glIsQueryEXT_ = (PFNGLISQUERYEXTPROC)eglGetProcAddress("glIsQueryEXT");
        glBeginQueryEXT_ = (PFNGLBEGINQUERYEXTPROC)eglGetProcAddress("glBeginQueryEXT");
        glEndQueryEXT_ = (PFNGLENDQUERYEXTPROC)eglGetProcAddress("glEndQueryEXT");
        glQueryCounterEXT_ = (PFNGLQUERYCOUNTEREXTPROC)eglGetProcAddress("glQueryCounterEXT");
        glGetQueryivEXT_ = (PFNGLGETQUERYIVEXTPROC)eglGetProcAddress("glGetQueryivEXT");
        glGetQueryObjectivEXT_ = (PFNGLGETQUERYOBJECTIVEXTPROC)eglGetProcAddress("glGetQueryObjectivEXT");
        glGetQueryObjectuivEXT_ = (PFNGLGETQUERYOBJECTUIVEXTPROC)eglGetProcAddress("glGetQueryObjectuivEXT");
        glGetQueryObjecti64vEXT_ = (PFNGLGETQUERYOBJECTI64VEXTPROC)eglGetProcAddress("glGetQueryObjecti64vEXT");
        glGetQueryObjectui64vEXT_  = (PFNGLGETQUERYOBJECTUI64VEXTPROC)eglGetProcAddress("glGetQueryObjectui64vEXT");
        glGetInteger64v_  = (PFNGLGETINTEGER64VPROC)eglGetProcAddress("glGetInteger64v");
    }

    if ( GL_ExtensionStringPresent( "GL_EXT_texture_sRGB_decode", extensions ) )
    {
        HasEXT_sRGB_texture_decode = true;
    }

    if ( GL_ExtensionStringPresent( "GL_EXT_texture_filter_anisotropic", extensions ) )
    {
        EXT_texture_filter_anisotropic = true;
    }

    GLint MaxTextureSize = 0;
    glGetIntegerv( GL_MAX_TEXTURE_SIZE, &MaxTextureSize );
    LOG( "GL_MAX_TEXTURE_SIZE = %d", MaxTextureSize );

    GLint MaxVertexUniformVectors = 0;
    glGetIntegerv( GL_MAX_VERTEX_UNIFORM_VECTORS, &MaxVertexUniformVectors );
    LOG( "GL_MAX_VERTEX_UNIFORM_VECTORS = %d", MaxVertexUniformVectors );

    GLint MaxFragmentUniformVectors = 0;
    glGetIntegerv( GL_MAX_FRAGMENT_UNIFORM_VECTORS, &MaxFragmentUniformVectors );
    LOG( "GL_MAX_FRAGMENT_UNIFORM_VECTORS = %d", MaxFragmentUniformVectors );

    // ES3 functions we need to getprocaddress to allow linking against ES2 lib
    glBlitFramebuffer_  = (PFNGLBLITFRAMEBUFFER_)eglGetProcAddress("glBlitFramebuffer");
    glRenderbufferStorageMultisample_  = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLE_)eglGetProcAddress("glRenderbufferStorageMultisample");
    glInvalidateFramebuffer_  = (PFNGLINVALIDATEFRAMEBUFFER_)eglGetProcAddress("glInvalidateFramebuffer");
    glMapBufferRange_  = (PFNGLMAPBUFFERRANGE_)eglGetProcAddress("glMapBufferRange");
    glUnmapBuffer_  = (PFNGLUNMAPBUFFEROESPROC_)eglGetProcAddress("glUnmapBuffer");
}

bool VGlOperation::GL_ExtensionStringPresent(const char *extension, const char *allExtensions)
{
    if ( strstr( allExtensions, extension ) )
    {
        LOG( "Found: %s", extension );
        return true;
    }
    else
    {
        LOG( "Not found: %s", extension );
        return false;
    }
}

void VGlOperation::GL_Finish()
{
    // Given the common driver "optimization" of ignoring glFinish, we
    // can't run reliably while drawing to the front buffer without
    // the Sync extension.
    if ( eglCreateSyncKHR_ != NULL )
    {
        // 100 milliseconds == 100000000 nanoseconds
        const EGLint wait = GL_FlushSync( 100000000 );
        if ( wait == EGL_TIMEOUT_EXPIRED_KHR )
        {
            LOG( "EGL_TIMEOUT_EXPIRED_KHR" );
        }
        if ( wait == EGL_FALSE )
        {
            LOG( "eglClientWaitSyncKHR returned EGL_FALSE" );
        }
    }
}

void VGlOperation::GL_Flush()
{
    if ( eglCreateSyncKHR_ != NULL )
    {
        const EGLint wait = GL_FlushSync( 0 );
        if ( wait == EGL_FALSE )
        {
            LOG("eglClientWaitSyncKHR returned EGL_FALSE");
        }
    }

    // Also do a glFlush() so it shows up in logging tools that
    // don't capture eglClientWaitSyncKHR_ calls.
//	glFlush();
}

void VGlOperation::GL_InvalidateFramebuffer(const invalidateTarget_t isFBO, const bool colorBuffer, const bool depthBuffer)
{
    const int offset = (int)!colorBuffer;
    const int count = (int)colorBuffer + ((int)depthBuffer)*2;

    const GLenum fboAttachments[3] = { GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT };
    const GLenum attachments[3] = { GL_COLOR_EXT, GL_DEPTH_EXT, GL_STENCIL_EXT };
    glInvalidateFramebuffer_( GL_FRAMEBUFFER, count, ( isFBO == INV_FBO ? fboAttachments : attachments ) + offset );
}

void VGlOperation::LogStringWords(const char *allExtensions)
{
    const char * start = allExtensions;
    while( 1 )
    {
        const char * end = strstr( start, " " );
        if ( end == NULL )
        {
            break;
        }
        unsigned int nameLen = (unsigned int)(end - start);
        if ( nameLen > 256 )
        {
            nameLen = 256;
        }
        char * word = new char[nameLen+1];
        memcpy( word, start, nameLen );
        word[nameLen] = '\0';
        LOG( "%s", word );
        delete[] word;

        start = end + 1;
    }
}

NV_NAMESPACE_END
