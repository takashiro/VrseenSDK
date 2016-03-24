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
    return hadError;
}

EGLint VGlOperation::GL_FlushSync(int timeout)
{
    // if extension not present, return NO_SYNC

    EGLDisplay eglDisplay = eglGetCurrentDisplay();

    const EGLSyncKHR sync = eglCreateSyncKHR( eglDisplay, EGL_SYNC_FENCE_KHR, NULL );
    if ( sync == EGL_NO_SYNC_KHR )
    {
        return EGL_FALSE;
    }

    const EGLint wait = eglClientWaitSyncKHR( eglDisplay, sync,
                                              EGL_SYNC_FLUSH_COMMANDS_BIT_KHR, timeout );

    eglDestroySyncKHR( eglDisplay, sync );

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


    // Enabling this seems to cause strange problems in Unity


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

void VGlOperation::GL_Flush()
{
    const EGLint wait = GL_FlushSync( 0 );
    if ( wait == EGL_FALSE )
    {
        LOG("eglClientWaitSyncKHR returned EGL_FALSE");
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
    glInvalidateFramebuffer( GL_FRAMEBUFFER, count, ( isFBO == INV_FBO ? fboAttachments : attachments ) + offset );
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

void VGlOperation::DumpEglConfigs( const EGLDisplay display )
{
    static const int MAX_CONFIGS = 1024;
    EGLConfig 	configs[MAX_CONFIGS];
    EGLint  	numConfigs = 0;

    if ( EGL_FALSE == eglGetConfigs( display,
                                     configs, MAX_CONFIGS, &numConfigs ) )
    {
        WARN( "eglGetConfigs() failed" );
        return;
    }

    LOG( "ES2 configs:" );
    LOG( "  Config R G B A DP S M W P REND" );
    for ( int i = 0; i < numConfigs; i++ )
    {
        EGLint	red = 0;
        eglGetConfigAttrib( display, configs[i], EGL_RED_SIZE, &red );
        EGLint	green = 0;
        eglGetConfigAttrib( display, configs[i], EGL_GREEN_SIZE, &green );
        EGLint	blue = 0;
        eglGetConfigAttrib( display, configs[i], EGL_BLUE_SIZE, &blue );
        EGLint	alpha = 0;
        eglGetConfigAttrib( display, configs[i], EGL_ALPHA_SIZE, &alpha );
        EGLint	depth = 0;
        eglGetConfigAttrib( display, configs[i], EGL_DEPTH_SIZE, &depth );
        EGLint	stencil = 0;
        eglGetConfigAttrib( display, configs[i], EGL_STENCIL_SIZE, &stencil );
        EGLint	multisamples = 0;
        eglGetConfigAttrib( display, configs[i], EGL_SAMPLES, &multisamples );

        // EGL_SURFACE_TYPE is a bit field
        EGLint	surface = 0;
        eglGetConfigAttrib( display, configs[i], EGL_SURFACE_TYPE , &surface );
        EGLint window = (surface & EGL_WINDOW_BIT) != 0;
        EGLint pbuffer = (surface & EGL_PBUFFER_BIT) != 0;

        // EGL_RENDERABLE_TYPE is a bit field
        EGLint	renderable = 0;
        eglGetConfigAttrib( display, configs[i], EGL_RENDERABLE_TYPE , &renderable );
        //		EGLint window = (surface & EGL_WINDOW_BIT) != 0;
        //		EGLint pbuffer = (surface & EGL_PBUFFER_BIT) != 0;

        LOG( "%8i %i %i %i %i %2i %i %i %i %i 0x%02x 0x%02x", (int)configs[i], red, green, blue, alpha,
             depth, stencil, multisamples, window, pbuffer, renderable, surface);
    }
}

EGLConfig VGlOperation::ChooseColorConfig( const EGLDisplay display, const int redBits,
                                           const int greeBits, const int blueBits, const int depthBits, const int samples, const bool pbuffer )
{

    // DumpEglConfigs( display );

    // We do NOT want to use eglChooseConfig, because the Android EGL code pushes in
    // multisample flags behind our back if the user has selected the "force 4x MSAA"
    // option in settings, and that is completely wasted for our warp target.
    static const int MAX_CONFIGS = 1024;
    EGLConfig 	configs[MAX_CONFIGS];
    EGLint  	numConfigs = 0;

    if ( EGL_FALSE == eglGetConfigs( display,
                                     configs, MAX_CONFIGS, &numConfigs ) ) {
        WARN( "eglGetConfigs() failed" );
        return NULL;
    }
    LOG( "eglGetConfigs() = %i configs", numConfigs );

    // We don't want a depth/stencil buffer
    const EGLint configAttribs[] = {
        EGL_BLUE_SIZE,  	blueBits,
        EGL_GREEN_SIZE, 	greeBits,
        EGL_RED_SIZE,   	redBits,
        EGL_DEPTH_SIZE,   	depthBits,
        //			EGL_STENCIL_SIZE,  	0,
        EGL_SAMPLES,		samples,
        EGL_NONE
    };

    // look for OpenGL ES 3.0 configs first, then fall back to 2.0
    for ( int esVersion = 3 ; esVersion >= 2 ; esVersion-- )
    {
        for ( int i = 0; i < numConfigs; i++ )
        {
            EGLint	value = 0;

            // EGL_RENDERABLE_TYPE is a bit field
            eglGetConfigAttrib( display, configs[i], EGL_RENDERABLE_TYPE, &value );

            if ( ( esVersion == 2 ) && ( value & EGL_OPENGL_ES2_BIT ) != EGL_OPENGL_ES2_BIT )
            {
                continue;
            }
            if ( ( esVersion == 3 ) && ( value & EGL_OPENGL_ES3_BIT_KHR ) != EGL_OPENGL_ES3_BIT_KHR )
            {
                continue;
            }

            // For our purposes, the pbuffer config also needs to be compatible with
            // normal window rendering so it can share textures with the window context.
            // I am unsure if it would be portable to always request EGL_PBUFFER_BIT, so
            // I only do it on request.
            eglGetConfigAttrib( display, configs[i], EGL_SURFACE_TYPE , &value );
            const int surfs = EGL_WINDOW_BIT | ( pbuffer ? EGL_PBUFFER_BIT : 0 );
            if ( ( value & surfs ) != surfs )
            {
                continue;
            }

            int	j = 0;
            for ( ; configAttribs[j] != EGL_NONE ; j += 2 )
            {
                EGLint	value = 0;
                eglGetConfigAttrib( display, configs[i], configAttribs[j], &value );
                if ( value != configAttribs[j+1] )
                {
                    break;
                }
            }
            if ( configAttribs[j] == EGL_NONE )
            {
                LOG( "Got an ES %i renderable config: %i", esVersion, (int)configs[i] );
                return configs[i];
            }
        }
    }
    return NULL;
}

void VGlOperation::EglSetup( const EGLContext shareContext,
                             const int requestedGlEsVersion,
                             const int redBits, const int greenBits, const int blueBits,
                             const int depthBits, const int multisamples, const GLuint contextPriority )
{
    VGlOperation glOperation;
    LOG( "EglSetup: requestGlEsVersion(%d), redBits(%d), greenBits(%d), blueBits(%d), depthBits(%d), multisamples(%d), contextPriority(%d)",
         requestedGlEsVersion, redBits, greenBits, blueBits, depthBits, multisamples, contextPriority );


    // Get the built in display
    // TODO: check for external HDMI displays
    display = eglGetDisplay( EGL_DEFAULT_DISPLAY );

    // Initialize EGL
    EGLint majorVersion;
    EGLint minorVersion;
    eglInitialize( display, &majorVersion, &minorVersion );
    LOG( "eglInitialize gives majorVersion %i, minorVersion %i", majorVersion, minorVersion);

    const char * eglVendorString = eglQueryString( display, EGL_VENDOR );
    LOG( "EGL_VENDOR: %s", eglVendorString );
    const char * eglClientApisString = eglQueryString( display, EGL_CLIENT_APIS );
    LOG( "EGL_CLIENT_APIS: %s", eglClientApisString );
    const char * eglVersionString = eglQueryString( display, EGL_VERSION );
    LOG( "EGL_VERSION: %s", eglVersionString );
    const char * eglExtensionString = eglQueryString( display, EGL_EXTENSIONS );
    LOG( "EGL_EXTENSIONS:" );
    LogStringWords( eglExtensionString );

    // We do NOT want to use eglChooseConfig, because the Android EGL code pushes in
    // multisample flags behind our back if the user has selected the "force 4x MSAA"
    // option in developer settings, and that is completely wasted for our warp target.
    config = ChooseColorConfig( display, redBits, greenBits, blueBits, depthBits, multisamples, true /* pBuffer compatible */ );
    if ( config == 0 )
    {
        FAIL( "No acceptable EGL color configs." );
        return ;
    }

    // The EGLContext is created with the EGLConfig
    // Try to get an OpenGL ES 3.0 context first, which is required to do
    // MSAA to framebuffer objects on Adreno.
    for ( int version = requestedGlEsVersion ; version >= 2 ; version-- )
    {
        LOG( "Trying for a EGL_CONTEXT_CLIENT_VERSION %i context shared with %p:",
             version, shareContext );
        // We want the application context to be lower priority than the TimeWarp context.
        EGLint contextAttribs[] = {
            EGL_CONTEXT_CLIENT_VERSION, version,
            EGL_NONE, EGL_NONE,
            EGL_NONE };

        // Don't set EGL_CONTEXT_PRIORITY_LEVEL_IMG at all if set to EGL_CONTEXT_PRIORITY_MEDIUM_IMG,
        // It is the caller's responsibility to use that if the driver doesn't support it.
        if ( contextPriority != EGL_CONTEXT_PRIORITY_MEDIUM_IMG )
        {
            contextAttribs[2] = EGL_CONTEXT_PRIORITY_LEVEL_IMG;
            contextAttribs[3] = contextPriority;
        }

        context = eglCreateContext( display, config, shareContext, contextAttribs );
        if ( context != EGL_NO_CONTEXT )
        {
            LOG( "Succeeded." );
            glEsVersion = version;

            EGLint configIDReadback;
            if ( !eglQueryContext( display, context, EGL_CONFIG_ID, &configIDReadback ) )
            {
                WARN("eglQueryContext EGL_CONFIG_ID failed" );
            }
            EGLConfig configCheck = glOperation.EglConfigForConfigID( display, configIDReadback );

            LOG( "Created context with config %i, query returned ID %i = config %i",
                 (int)config, configIDReadback, (int)configCheck );
            break;
        }
    }
    if ( context == EGL_NO_CONTEXT )
    {
        WARN( "eglCreateContext failed: %s", glOperation.EglErrorString() );
        return ;
    }

    if ( contextPriority != EGL_CONTEXT_PRIORITY_MEDIUM_IMG )
    {
        // See what context priority we actually got
        EGLint actualPriorityLevel;
        eglQueryContext( display, context, EGL_CONTEXT_PRIORITY_LEVEL_IMG, &actualPriorityLevel );
        switch ( actualPriorityLevel )
        {
        case EGL_CONTEXT_PRIORITY_HIGH_IMG: LOG( "Context is EGL_CONTEXT_PRIORITY_HIGH_IMG" ); break;
        case EGL_CONTEXT_PRIORITY_MEDIUM_IMG: LOG( "Context is EGL_CONTEXT_PRIORITY_MEDIUM_IMG" ); break;
        case EGL_CONTEXT_PRIORITY_LOW_IMG: LOG( "Context is EGL_CONTEXT_PRIORITY_LOW_IMG" ); break;
        default: LOG( "Context has unknown priority level" ); break;
        }
    }

    // Because EGL_KHR_surfaceless_context is not widespread (Only on Tegra as of
    // September 2013), we need to create a tiny pbuffer surface to make the context
    // current.
    //
    // It is necessary to use a config with the same characteristics that the
    // context was created with, plus the pbuffer flag, or we will get an
    // EGL_BAD_MATCH error on the eglMakeCurrent() call.
    //
    // This is necessary to support 565 framebuffers, which may be important
    // for higher refresh rate displays.
    const EGLint attrib_list[] =
    {
        EGL_WIDTH, 16,
        EGL_HEIGHT, 16,
        EGL_NONE
    };
    pbufferSurface = eglCreatePbufferSurface( display, config, attrib_list );
    if ( pbufferSurface == EGL_NO_SURFACE )
    {
        WARN( "eglCreatePbufferSurface failed: %s", glOperation.EglErrorString() );
        eglDestroyContext( display, context );
        context = EGL_NO_CONTEXT;
        return ;
    }

    if ( eglMakeCurrent( display, pbufferSurface, pbufferSurface, context ) == EGL_FALSE )
    {
        WARN( "eglMakeCurrent pbuffer failed: %s", glOperation.EglErrorString() );
        eglDestroySurface( display, pbufferSurface );
        eglDestroyContext( display, context );
        context = EGL_NO_CONTEXT;
        return ;
    }

    const char * glVendorString = (const char *) glGetString(GL_VENDOR);
    LOG( "GL_VENDOR: %s", glVendorString);
    const char * glRendererString = (const char *) glGetString(GL_RENDERER);
    LOG( "GL_RENDERER: %s", glRendererString);
    const char * glVersionString = (const char *) glGetString(GL_VERSION);
    LOG( "GL_VERSION: %s", glVersionString);
    const char * glSlVersionString = (const char *) glGetString(
                GL_SHADING_LANGUAGE_VERSION);
    LOG( "GL_SHADING_LANGUAGE_VERSION: %s", glSlVersionString);

    gpuType = glOperation.EglGetGpuType();

    return ;
}

void VGlOperation::EglShutdown(  )
{
    VGlOperation glOperation;
    if ( eglMakeCurrent( display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT ) == EGL_FALSE )
    {
        FAIL( "eglMakeCurrent: failed: %s", glOperation.EglErrorString() );
    }
    if ( eglDestroyContext( display, context ) == EGL_FALSE )
    {
        FAIL( "eglDestroyContext: failed: %s", glOperation.EglErrorString() );
    }
    if ( eglDestroySurface( display, pbufferSurface ) == EGL_FALSE )
    {
        FAIL( "eglDestroySurface: failed: %s", glOperation.EglErrorString() );
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
    PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG glFramebufferTexture2DMultisampleIMG_;
    if ( GL_ExtensionStringPresent( "GL_IMG_multisampled_render_to_texture", extensions ) )
    {
        glFramebufferTexture2DMultisampleIMG_ = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG)GetExtensionProc ( "glFramebufferTexture2DMultisampleIMG" );
    }
    else if ( GL_ExtensionStringPresent( "GL_EXT_multisampled_render_to_texture", extensions ) )
    {
        // assign to the same function pointers as the IMG extension
        glFramebufferTexture2DMultisampleIMG_ = (PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEIMG)GetExtensionProc ( "glFramebufferTexture2DMultisampleEXT" );
    }
    glFramebufferTexture2DMultisampleIMG_(target, attachment, textarget, texture, level, samples);
}

void VGlOperation::glRenderbufferStorageMultisampleIMG(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG glRenderbufferStorageMultisampleIMG_;
    if ( GL_ExtensionStringPresent( "GL_IMG_multisampled_render_to_texture", extensions ) )
    {
        glRenderbufferStorageMultisampleIMG_ = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG)GetExtensionProc ( "glRenderbufferStorageMultisampleIMG" );
    }
    else if ( GL_ExtensionStringPresent( "GL_EXT_multisampled_render_to_texture", extensions ) )
    {
        // assign to the same function pointers as the IMG extension
        glRenderbufferStorageMultisampleIMG_ = (PFNGLRENDERBUFFERSTORAGEMULTISAMPLEIMG)GetExtensionProc ( "glRenderbufferStorageMultisampleEXT" );
    }
    glRenderbufferStorageMultisampleIMG_(target, samples, internalformat, width, height);
}

EGLSyncKHR VGlOperation::eglCreateSyncKHR(EGLDisplay dpy, EGLenum type, const EGLint *attrib_list)
{
    PFNEGLCREATESYNCKHRPROC eglCreateSyncKHR_;
    eglCreateSyncKHR_ = (PFNEGLCREATESYNCKHRPROC)GetExtensionProc( "eglCreateSyncKHR" );
    return eglCreateSyncKHR_(dpy, type, attrib_list);
}

EGLBoolean VGlOperation::eglDestroySyncKHR(EGLDisplay dpy, EGLSyncKHR sync)
{
    PFNEGLDESTROYSYNCKHRPROC eglDestroySyncKHR_;
    eglDestroySyncKHR_ = (PFNEGLDESTROYSYNCKHRPROC)GetExtensionProc( "eglDestroySyncKHR" );
    return eglDestroySyncKHR_(dpy, sync);
}

EGLint VGlOperation::eglClientWaitSyncKHR(EGLDisplay dpy, EGLSyncKHR sync, EGLint flags, EGLTimeKHR timeout)
{
    PFNEGLCLIENTWAITSYNCKHRPROC eglClientWaitSyncKHR_;
    eglClientWaitSyncKHR_ = (PFNEGLCLIENTWAITSYNCKHRPROC)GetExtensionProc( "eglClientWaitSyncKHR" );
    return eglClientWaitSyncKHR_(dpy, sync, flags, timeout);
}

void VGlOperation::glBindVertexArrayOES(GLuint array)
{
    PFNGLBINDVERTEXARRAYOESPROC glBindVertexArrayOES_;
    if ( GL_ExtensionStringPresent( "GL_OES_vertex_array_object", extensions ) ) {
        glBindVertexArrayOES_ = (PFNGLBINDVERTEXARRAYOESPROC)eglGetProcAddress("glBindVertexArrayOES");
    }
    glBindVertexArrayOES_(array);
}

void VGlOperation::glDeleteVertexArraysOES(GLsizei n, const GLuint *arrays)
{
    PFNGLDELETEVERTEXARRAYSOESPROC glDeleteVertexArraysOES_;
    if ( GL_ExtensionStringPresent( "GL_OES_vertex_array_object", extensions ) )
    {
        glDeleteVertexArraysOES_ = (PFNGLDELETEVERTEXARRAYSOESPROC)eglGetProcAddress("glDeleteVertexArraysOES");
    }
    glDeleteVertexArraysOES_(n, arrays);
}

void VGlOperation::glGenVertexArraysOES(GLsizei n, GLuint *arrays)
{
    PFNGLGENVERTEXARRAYSOESPROC glGenVertexArraysOES_;
    if ( GL_ExtensionStringPresent( "GL_OES_vertex_array_object", extensions ) )
    {
        glGenVertexArraysOES_ = (PFNGLGENVERTEXARRAYSOESPROC)eglGetProcAddress("glGenVertexArraysOES");
    }
    glGenVertexArraysOES_(n, arrays);
}

void VGlOperation::glStartTilingQCOM(GLuint x, GLuint y, GLuint width, GLuint height, GLbitfield preserveMask)
{
    PFNGLSTARTTILINGQCOMPROC glStartTilingQCOM_;
    if ( GL_ExtensionStringPresent( "GL_QCOM_tiled_rendering", extensions ) )
    {
        glStartTilingQCOM_ = (PFNGLSTARTTILINGQCOMPROC)eglGetProcAddress("glStartTilingQCOM");
    }
    glStartTilingQCOM_(x, y, width, height, preserveMask);
}

void VGlOperation::glEndTilingQCOM(GLbitfield preserveMask)
{
    PFNGLENDTILINGQCOMPROC glEndTilingQCOM_;
    if ( GL_ExtensionStringPresent( "GL_QCOM_tiled_rendering", extensions ) )
    {
        glEndTilingQCOM_ = (PFNGLENDTILINGQCOMPROC)eglGetProcAddress("glEndTilingQCOM");
    }
    glEndTilingQCOM_(preserveMask);
}

void VGlOperation::glGenQueriesEXT(GLsizei n, GLuint *ids)
{
    typedef void (GL_APIENTRYP PFNGLGENQUERIESEXTPROC) (GLsizei n, GLuint *ids);
    PFNGLGENQUERIESEXTPROC glGenQueriesEXT_;
    if ( GL_ExtensionStringPresent( "GL_EXT_disjoint_timer_query", extensions ) )
    {
        glGenQueriesEXT_ = (PFNGLGENQUERIESEXTPROC)eglGetProcAddress("glGenQueriesEXT");
    }
    glGenQueriesEXT_(n, ids);
}

void VGlOperation::glDeleteQueriesEXT(GLsizei n, const GLuint *ids)
{
    typedef void (GL_APIENTRYP PFNGLDELETEQUERIESEXTPROC) (GLsizei n, const GLuint *ids);
    PFNGLDELETEQUERIESEXTPROC glDeleteQueriesEXT_;
    if ( GL_ExtensionStringPresent( "GL_EXT_disjoint_timer_query", extensions ) )
    {
        glDeleteQueriesEXT_ = (PFNGLDELETEQUERIESEXTPROC)eglGetProcAddress("glDeleteQueriesEXT");
    }
    glDeleteQueriesEXT_(n, ids);
}

void VGlOperation::glBeginQueryEXT(GLenum target, GLuint id)
{
    typedef void (GL_APIENTRYP PFNGLBEGINQUERYEXTPROC) (GLenum target, GLuint id);
    PFNGLBEGINQUERYEXTPROC glBeginQueryEXT_;
    if ( GL_ExtensionStringPresent( "GL_EXT_disjoint_timer_query", extensions ) )
    {
        glBeginQueryEXT_ = (PFNGLBEGINQUERYEXTPROC)eglGetProcAddress("glBeginQueryEXT");
    }
    glBeginQueryEXT_(target, id);
}

void VGlOperation::glEndQueryEXT(GLenum target)
{
    typedef void (GL_APIENTRYP PFNGLENDQUERYEXTPROC) (GLenum target);
    PFNGLENDQUERYEXTPROC glEndQueryEXT_;
    if ( GL_ExtensionStringPresent( "GL_EXT_disjoint_timer_query", extensions ) )
    {
        glEndQueryEXT_ = (PFNGLENDQUERYEXTPROC)eglGetProcAddress("glEndQueryEXT");
    }
    glEndQueryEXT_(target);
}

void VGlOperation::glQueryCounterEXT(GLuint id, GLenum target)
{
    typedef void (GL_APIENTRYP PFNGLQUERYCOUNTEREXTPROC) (GLuint id, GLenum target);
    PFNGLQUERYCOUNTEREXTPROC glQueryCounterEXT_;
    if ( GL_ExtensionStringPresent( "GL_EXT_disjoint_timer_query", extensions ) )
    {
        glQueryCounterEXT_ = (PFNGLQUERYCOUNTEREXTPROC)eglGetProcAddress("glQueryCounterEXT");
    }
    glQueryCounterEXT_(id, target);
}

void VGlOperation::glGetQueryObjectivEXT(GLuint id, GLenum pname, GLint *params)
{
    typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTIVEXTPROC) (GLuint id, GLenum pname, GLint *params);
    PFNGLGETQUERYOBJECTIVEXTPROC glGetQueryObjectivEXT_;
    if ( GL_ExtensionStringPresent( "GL_EXT_disjoint_timer_query", extensions ) )
    {
        glGetQueryObjectivEXT_ = (PFNGLGETQUERYOBJECTIVEXTPROC)eglGetProcAddress("glGetQueryObjectivEXT");
    }
    glGetQueryObjectivEXT_(id, pname, params);
}

void VGlOperation::glGetQueryObjectui64vEXT(GLuint id, GLenum pname, GLuint64 *params)
{
    typedef void (GL_APIENTRYP PFNGLGETQUERYOBJECTUI64VEXTPROC) (GLuint id, GLenum pname, GLuint64 *params);
    PFNGLGETQUERYOBJECTUI64VEXTPROC glGetQueryObjectui64vEXT_;
    if ( GL_ExtensionStringPresent( "GL_EXT_disjoint_timer_query", extensions ) )
    {
        glGetQueryObjectui64vEXT_  = (PFNGLGETQUERYOBJECTUI64VEXTPROC)eglGetProcAddress("glGetQueryObjectui64vEXT");
    }
    glGetQueryObjectui64vEXT_(id, pname, params);
}

void VGlOperation::glGetInteger64v(GLenum pname, GLint64 *params)
{
    typedef void (GL_APIENTRYP PFNGLGETINTEGER64VPROC) (GLenum pname, GLint64 *params);
    PFNGLGETINTEGER64VPROC glGetInteger64v_;
    if ( GL_ExtensionStringPresent( "GL_EXT_disjoint_timer_query", extensions ) )
    {
        glGetInteger64v_  = (PFNGLGETINTEGER64VPROC)eglGetProcAddress("glGetInteger64v");
    }
    glGetInteger64v_(pname, params);
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
