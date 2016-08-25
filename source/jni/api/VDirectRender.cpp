#include "VDirectRender.h"

#include <unistd.h>						// sleep, etc
#include <stdlib.h>
#include <assert.h>
#include <jni.h>
#include "../App.h"


PFNEGLLOCKSURFACEKHRPROC	eglLockSurfaceKHR_;
PFNEGLUNLOCKSURFACEKHRPROC	eglUnlockSurfaceKHR_;

NV_NAMESPACE_BEGIN

// JDC: we NEED an egl extension string for this
static bool FrontBufferExtensionPresent()
{
    return true;	// will crash on some unsupported systems...
//	return false;
}

VrSurfaceManager::VrSurfaceManager() :
        m_env( NULL ),
        m_surfaceClass( NULL ),
        m_setFrontBufferID( NULL ),
        m_getFrontBufferAddressID( NULL ),
        m_getSurfaceBufferAddressID( NULL ),
        m_getClientBufferAddressID( NULL )
{
}

VrSurfaceManager::~VrSurfaceManager()
{
    shutdown();
}

void VrSurfaceManager::init( JNIEnv * jni_ )
{
    if ( jni_ == NULL )
    {
        vInfo( "VrSurfaceManager::Init - Invalid jni" );
        return;
    }

    m_env = jni_;

    // ADAM note
    // We don't have VRSurfaceManager. so we won't able to get surfaceClass
    // Therefore we don;t have front buffer ID

    // Determine if the Java Front Buffer IF exists. If not, fall back
    // to using the egl extensions.
    jclass lc = m_env->FindClass( "android/app/VRSurfaceManager" );
    if ( lc != NULL )
    {
        m_surfaceClass = (jclass)m_env->NewGlobalRef( lc );
        vInfo( "Found VrSurfaceManager API");
        m_env->DeleteLocalRef( lc );
    }

    // Clear NoClassDefFoundError, if thrown
    if ( m_env->ExceptionOccurred() )
    {
        m_env->ExceptionClear();
        vInfo( "Clearing JNI Exceptions" );
    }

    // Look up the Java Front Buffer IF method IDs
    if ( m_surfaceClass != NULL )
    {
        m_setFrontBufferID = m_env->GetStaticMethodID( m_surfaceClass, "setFrontBuffer", "(IZ)V" );
        m_getFrontBufferAddressID = m_env->GetStaticMethodID( m_surfaceClass, "getFrontBufferAddress", "(I)I" );
        m_getSurfaceBufferAddressID = m_env->GetStaticMethodID( m_surfaceClass, "getSurfaceBufferAddress", "(I[II)I" );
        m_getClientBufferAddressID = m_env->GetStaticMethodID( m_surfaceClass, "getClientBufferAddress", "(I)I" );
    }
}

void VrSurfaceManager::shutdown()
{
    vInfo( "VrSurfaceManager::Shutdown" );

    if ( m_surfaceClass != NULL )
    {
        m_env->DeleteGlobalRef( m_surfaceClass );
        m_surfaceClass = NULL;
    }

    m_env = NULL;
}

bool VrSurfaceManager::setFrontBuffer( const EGLSurface surface_, const bool set )
{
    bool gvrFrontbuffer = false;

    if ( FrontBufferExtensionPresent() )
    {
        // Test the Java IF first (present on Note 4)
        if ( m_setFrontBufferID != NULL )
        {
            vInfo( "Calling java method");
            // Use the Java Front Buffer IF
            m_env->CallStaticVoidMethod( m_surfaceClass, m_setFrontBufferID, (int)surface_, set );

            // test address - currently crashes if executed here
            /*
            void * addr = (void *)env->CallStaticIntMethod( surfaceClass, getFrontBufferAddressID, (int)surface_ );
            if ( addr ) {
                vInfo( "getFrontBufferAddress succeeded %i", addr );
                gvrFrontbufferExtension = true;
            } else {
                vInfo( "getFrontBufferAddress failed %i", addr );
                gvrFrontbufferExtension = false;
            }
            */

            gvrFrontbuffer = true;

            // Catch Permission denied exception
            if ( m_env->ExceptionOccurred() )
            {
                vWarn( "Exception: egl_GVR_FrontBuffer failed" );
                m_env->ExceptionClear();
                gvrFrontbuffer = false;
            }
        }
#if 0  // keep old code
        // If the Java IF is not present or permission was denied,
        // fallback to the egl extension found on the S5s.
        if ( set && ( m_setFrontBufferID == NULL || gvrFrontbuffer == false ) )
        {
            // Fall back to using the EGL Front Buffer extension
            typedef void * (*PFN_GVR_FrontBuffer) (EGLSurface surface);
            PFN_GVR_FrontBuffer egl_GVR_FrontBuffer = NULL;

            // look for the extension
            egl_GVR_FrontBuffer = (PFN_GVR_FrontBuffer)eglGetProcAddress( "egl_GVR_FrontBuffer" );
            if ( egl_GVR_FrontBuffer == NULL )
            {
                vInfo( "Not found: egl_GVR_FrontBuffer" );
                gvrFrontbuffer = false;
            }
            else
            {
                vInfo( "Found: egl_GVR_FrontBuffer" );
                void * ret = egl_GVR_FrontBuffer( surface_ );
                if ( ret )
                {
                    vInfo( "egl_GVR_FrontBuffer succeeded" );
                    gvrFrontbuffer = true;
                }
                else
                {
                    vWarn( "egl_GVR_FrontBuffer failed" );
                    gvrFrontbuffer = false;
                }
            }
        }
#else
        // try to setup front buffer
        if ( set && ( m_setFrontBufferID == NULL || gvrFrontbuffer == false ) )
        {
            // we cannot fall back to using the EGL Front Buffer extension since we don't have such extension
            // directly return true
            gvrFrontbuffer = true;
        }
#endif
    }
    else
    {
        // ignore it
        vInfo( "Not found: egl_GVR_FrontBuffer" );
        gvrFrontbuffer = false;
    }

    return gvrFrontbuffer;
}

void * VrSurfaceManager::getFrontBufferAddress( const EGLSurface surface )
{
    if ( m_getFrontBufferAddressID != NULL )
    {
        return (void *)m_env->CallStaticIntMethod( m_surfaceClass, m_getFrontBufferAddressID, (int)surface );
    }

    vInfo( "getFrontBufferAddress not found" );
    return NULL;
}

void * VrSurfaceManager::getSurfaceBufferAddress( const EGLSurface surface, int attribs[], const int attr_size, const int pitch )
{
    if ( m_getSurfaceBufferAddressID != NULL )
    {
        jintArray attribs_array = m_env->NewIntArray( attr_size );
        m_env->SetIntArrayRegion( attribs_array, 0, attr_size, attribs );
        return (void *)m_env->CallStaticIntMethod( m_surfaceClass, m_getSurfaceBufferAddressID, (int)surface, attribs_array, pitch );
    }

    vInfo( "getSurfaceBufferAddress not found" );
    return NULL;
}

void * VrSurfaceManager::getClientBufferAddress( const EGLSurface surface )
{
    if ( m_getClientBufferAddressID != NULL )
    {
        // Use the Java Front Buffer IF
        void * ret = (void *)m_env->CallStaticIntMethod( m_surfaceClass, m_getClientBufferAddressID, (int)surface );
        vInfo( "getClientBufferAddress");
        return ret;
    }
    else
    {
        // Fall back to using the EGL Front Buffer extension
        typedef EGLClientBuffer (*PFN_EGL_SEC_getClientBufferForFrontBuffer) (EGLSurface surface);
        PFN_EGL_SEC_getClientBufferForFrontBuffer EGL_SEC_getClientBufferForFrontBuffer =
                (PFN_EGL_SEC_getClientBufferForFrontBuffer)eglGetProcAddress( "EGL_SEC_getClientBufferForFrontBuffer" );
        if ( EGL_SEC_getClientBufferForFrontBuffer == NULL )
        {
            vInfo( "Not found: EGL_SEC_getClientBufferForFrontBuffer" );
            return NULL;
        }
        void * ret = EGL_SEC_getClientBufferForFrontBuffer( eglGetCurrentSurface( EGL_DRAW ) );
        vInfo( "EGL_SEC_getClientBufferForFrontBuffer");
        return ret;
    }

    return NULL;
}

// For Adreno, we can render half-screens three different ways.
enum tilerControl_t
{
    FB_TILED_RENDERING,			// works properly, but re-issues geometry for each tile
    FB_BINNING_CONTROL,			// doesn't work on 330, but does on 420
    FB_WRITEONLY_RENDERING,		// blended vignettes don't work
    FB_MALI
};

tilerControl_t tilerControl;

DirectRender::DirectRender() :
        windowSurface( 0 ),
        m_wantFrontBuffer( false ),
        m_display( 0 ),
        m_context( 0 ),
        m_width( 0 ),
        m_height( 0 ),
        m_gvrFrontbufferExtension( false )
{
}

void DirectRender::initForCurrentSurface( JNIEnv * jni, bool wantFrontBuffer_,int buildVersionSDK_)
{
    vInfo( "DirectRender::InitForCurrentSurface");

    m_wantFrontBuffer = wantFrontBuffer_;

    m_display = eglGetDisplay( EGL_DEFAULT_DISPLAY );
    m_context = eglGetCurrentContext();

    windowSurface = eglGetCurrentSurface( EGL_DRAW );

    EGLint res;
	eglQuerySurface(m_display, windowSurface, EGL_RENDER_BUFFER, &res);

	if (res == EGL_SINGLE_BUFFER)
	{
		vInfo("single buffer is used!");
	}
	else if (res == EGL_BACK_BUFFER)
	{
		vInfo("back buffer is used!");
	}
	else
	{
		vInfo("error no buffer is used!");
	}

	EGLint configID;
	if ( !eglQueryContext( m_display, m_context, EGL_CONFIG_ID, &configID ) )
	{
		vFatal( "eglQueryContext EGL_CONFIG_ID failed" );
	}
	EGLConfig eglConfig = m_eglStatus.eglConfigForConfigID(configID );
	if ( eglConfig == NULL )
	{
		vFatal( "EglConfigForConfigID failed" );
	}
	  //TODO::Compare the EGLConfig which create the EGLSurface
		vInfo("<<---EGLConfig attributes list start--->>");
		EGLint value = 0;

		eglGetConfigAttrib( m_eglStatus.m_display, eglConfig, EGL_ALPHA_SIZE, &value);
		vInfo("EGL_ALPHA_SIZE : " <<value);
		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_ALPHA_MASK_SIZE, &value);
		vInfo("EGL_ALPHA_MASK_SIZE : "<<value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_BIND_TO_TEXTURE_RGB, &value);
		if (value==EGL_TRUE)
		{
			vInfo("EGL_BIND_TO_TEXTURE_RGB : TRUE");
		}
		else
			vInfo("EGL_BIND_TO_TEXTURE_RGB : false");

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_BIND_TO_TEXTURE_RGBA, &value);
		if (value==EGL_TRUE)
		{
			vInfo("EGL_BIND_TO_TEXTURE_RGBA : TRUE");
		}
		else
			vInfo("EGL_BIND_TO_TEXTURE_RGBA : false");

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_BLUE_SIZE, &value);
		vInfo("EGL_BLUE_SIZE : " << value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_BUFFER_SIZE, &value);
		vInfo("EGL_BUFFER_SIZE : " << value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_COLOR_BUFFER_TYPE, &value);
		if (value == EGL_RGB_BUFFER)
		{
			vInfo("EGL_COLOR_BUFFER_TYPE : EGL_RGB_BUFFER");
		}
		else
		{
			vInfo("EGL_COLOR_BUFFER_TYPE : EGL_LUMINANCE_BUFFER");
		}

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_CONFIG_CAVEAT, &value);
		if (value == EGL_SLOW_CONFIG)
		{
			vInfo("EGL_CONFIG_CAVEAT : EGL_SLOW_CONFIG");
		}
		else if (value == EGL_NON_CONFORMANT_CONFIG)
		{
			vInfo("EGL_CONFIG_VAVEAT : EGL_NON_CONFORMANT_CONFIG");
		}
		else
		{
			vInfo("EGL_CONFIG_VAVEAT : EGL_NONE");
		}

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_CONFIG_ID, &value);
		vInfo("EGL_CONFIG_ID : " << value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_CONFORMANT, &value);
		vInfo("EGL_CONFORMANT : " << value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_DEPTH_SIZE, &value);
		vInfo("EGL_DEPTH_SIZE : "<< value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_GREEN_SIZE, &value);
		vInfo("EGL_GREEN_SIZE : " << value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_LEVEL, &value);
		vInfo("EGL_LEVEL : " << value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_LUMINANCE_SIZE, &value);
		vInfo("EGL_LUMINANCE_SIZE : " << value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_MAX_PBUFFER_WIDTH, &value);
		vInfo("EGL_MAX_PBUFFER_WIDTH : " << value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_MAX_PBUFFER_HEIGHT, &value);
		vInfo("EGL_MAX_PBUFFER_HEIGHT : " << value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_MAX_SWAP_INTERVAL, &value);
		vInfo("EGL_MAX_SWAP_INTERVAL : " << value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_MIN_SWAP_INTERVAL, &value);
		vInfo("EGL_MIN_SWAP_INTERVAL : " << value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_NATIVE_RENDERABLE, &value);
		if(value == EGL_TRUE)
		{
			vInfo("EGL_NATIVE_RENDERABLE : TRUE");
		}
		else
		{
			vInfo("EGL_NATIVE_RENDERABLE : FALSE");
		}

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_NATIVE_VISUAL_ID, &value);
		vInfo("EGL_NATIVE_VISUAL_ID : " << value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_NATIVE_VISUAL_TYPE, &value);
		vInfo("EGL_NATIVE_VISUAL_TYPE : " << value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_RED_SIZE, &value);
		vInfo("EGL_RED_SIZE : " << value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_RENDERABLE_TYPE, &value);
		vInfo("EGL_RENDERABLE_TYPE : " << value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_SAMPLE_BUFFERS, &value);
		vInfo("EGL_SAMPLE_BUFFERS : " << value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_SAMPLES, &value);
		vInfo("EGL_SAMPLES : "<< value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_STENCIL_SIZE, &value);
		vInfo("EGL_STENCIL_SIZE : "<< value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_SURFACE_TYPE, &value);
		vInfo("EGL_SURFACE_TYPE : "<< value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_TRANSPARENT_TYPE, &value);
		vInfo("EGL_TRANSPARENT_TYPE £º " <<value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_TRANSPARENT_RED_VALUE, &value);
		vInfo("EGL_TRANSPARENT_RED_VALUE" << value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_TRANSPARENT_GREEN_VALUE, &value);
		vInfo("EGL_TRANSPARENT_GREEN_VALUE" << value);

		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_TRANSPARENT_BLUE_VALUE, &value);
		vInfo("EGL_TRANSPARENT_BLUE_VALUE" << value);

		vInfo("<<---EGLConfig attributes list END--->>");
		//END THE COMPARE

		//TODO::EGLSurface attributes
		EGLDisplay tmpdisplay = m_eglStatus.m_display;

		EGLSurface tmpsurface = windowSurface;
		vInfo("<<---EGLSurface attributes list start--->>");
		eglQuerySurface(tmpdisplay, tmpsurface, EGL_CONFIG_ID, &value);
		vInfo("EGL_CONFIG_ID : "<< value);

		eglQuerySurface(tmpdisplay, tmpsurface, EGL_HEIGHT, &value);
		vInfo("EGL_HEIGHT : " << value);

		eglQuerySurface(tmpdisplay, tmpsurface, EGL_HORIZONTAL_RESOLUTION, &value);
		vInfo("EGL_HORIZONTAL_RESOLUTION : " << value);

		eglQuerySurface(tmpdisplay, tmpsurface, EGL_LARGEST_PBUFFER, &value);
		vInfo("EGL_LARGEST_PBUFFER :" << value);

		eglQuerySurface(tmpdisplay, tmpsurface, EGL_MIPMAP_LEVEL, &value);
		vInfo("EGL_MIPMAP_LEVEL : " << value);

		eglQuerySurface(tmpdisplay, tmpsurface, EGL_MIPMAP_TEXTURE, &value);
		if (value == EGL_TRUE)
		{
			vInfo("EGL_MIPMAP_TEXTURE : TRUE" );
		}
		else
			vInfo("EGL_MIPMAP_TEXTURE : FALSE");

		eglQuerySurface(tmpdisplay, tmpsurface, EGL_MULTISAMPLE_RESOLVE, &value);
		if (value == EGL_MULTISAMPLE_RESOLVE_DEFAULT )
		{
			vInfo("EGL_MULTISAMPLE_RESOLVE : EGL_MULTISAMPLE_RESOLVE_DEFAULT ");
		}
		else
			vInfo("EGL_MULTISAMPLE_RESOLVE : EGL_MULTISAMPLE_RESOLVE_BOX");

		eglQuerySurface(tmpdisplay, tmpsurface, EGL_PIXEL_ASPECT_RATIO, &value);
		vInfo("EGL_PIXEL_ASPECT_RATIO : " << value);

		eglQuerySurface(tmpdisplay, tmpsurface, EGL_RENDER_BUFFER, &value);
		if (value == EGL_BACK_BUFFER)
		{
			vInfo("EGL_RENDER_BUFFER : EGL_BACK_BUFFER");
		}
		else if (value == EGL_SINGLE_BUFFER)
		{
			vInfo("EGL_RENDER_BUFFER : EGL_SINGLE_BUFFER");
		}
		else
			vInfo("EGL_RENDER_BUFFER : OTHER_BUFFER");

		eglQuerySurface(tmpdisplay, tmpsurface, EGL_SWAP_BEHAVIOR, &value);
		if (value == EGL_BUFFER_PRESERVED)
		{
			vInfo("EGL_SWAP_BEHAVIOR : EGL_BUFFER_PRESERVED");
		}
		else if (value == EGL_BUFFER_DESTROYED)
		{
			vInfo("EGL_SWAP_BEHAVIOR : EGL_BUFFER_DESTROYED");
		}

		eglQuerySurface(tmpdisplay, tmpsurface, EGL_TEXTURE_FORMAT, &value);
		if(value == EGL_NO_TEXTURE)
		{
			vInfo("EGL_TEXTURE_FORMAT : EGL_NO_TEXTURE");
		}
		else if (value == EGL_TEXTURE_RGB)
		{
			vInfo("EGL_TEXTURE_FORMAT : EGL_TEXTURE_RGB");
		}
		else if (value == EGL_TEXTURE_RGBA)
		{
			vInfo("EGL_TEXTURE_FORMAT : EGL_TEXTURE_RGBA");
		}

		eglQuerySurface(tmpdisplay , tmpsurface, EGL_TEXTURE_TARGET, &value);
		if (value == EGL_NO_TEXTURE)
		{
			vInfo("EGL_TEXTURE_TARGET : EGL_NO_TEXTURE");
		}
		else if (value == EGL_TEXTURE_2D)
		{
			vInfo("EGL_TEXTURE_TARGET : EGL_TEXTURE_2D");
		}

		eglQuerySurface(tmpdisplay, tmpsurface, EGL_VERTICAL_RESOLUTION, &value);
		vInfo("EGL_VERTICAL_RESOLUTION : " << value);

		eglQuerySurface(tmpdisplay, tmpsurface, EGL_WIDTH, &value);
		vInfo("EGL_WIDTH : " << value);

		vInfo("<<---EGLSurface attributes list end--->>");

		vInfo("<<---EGLContext attributes list start-->>");

		EGLContext tmpcontext = eglGetCurrentContext();
		eglQueryContext(tmpdisplay, tmpcontext, EGL_CONFIG_ID, &value);
		vInfo("EGL_CONFIG_ID : " << value);

		eglQueryContext(tmpdisplay, tmpcontext, EGL_CONTEXT_CLIENT_TYPE, &value);
		if (value == EGL_OPENGL_API)
		{
			vInfo("EGL_CONTEXT_CLIENT_TYPE : EGL_OPENGL_API");
		}
		else if (value == EGL_OPENGL_ES_API)
		{
			vInfo("EGL_CONTEXT_CLIENT_TYPE : EGL_OPENGL_ES_API");
		}
		else if (value == EGL_OPENVG_API)
		{
			vInfo("EGL_CONTEXT_CLIENT_TYPE : EGL_OPENVG_API");
		}

		eglQueryContext(tmpdisplay, tmpcontext, EGL_CONTEXT_CLIENT_VERSION, &value);
		vInfo("EGL_CONTEXT_CLIENT_VERSION : "<< value);

		eglQueryContext(tmpdisplay, tmpcontext, EGL_RENDER_BUFFER, &value);
		if (value == EGL_SINGLE_BUFFER)
		{
			vInfo("EGL_RENDER_BUFFER : EGL_SINGLE_BUFFER");
		}
		else if (value == EGL_BACK_BUFFER)
		{
			vInfo("EGL_RENDER_BUFFER : EGL_SINGLE_BUFFER");
		}
		else
		{
			vInfo("EGL_RENDER_BUFFER : EGL_NONE");
		}

		vInfo("<<---EGLContext attributes list end--->>");



    // NOTE: On Mali as well as under Android-L, we need to perform
    // an initial swapbuffers in order for the front-buffer extension
    // to work.
    // NOTE: On Adreno KitKat, we cannot apply the initial swapbuffers
    // as it will result in poor performance.

    static const int KITKAT_WATCH = 20;
    const VEglDriver::GpuType gpuType = m_eglStatus.eglGetGpuType();
    if ( ( buildVersionSDK_ > KITKAT_WATCH ) ||	// if the SDK is Lollipop or higher
         ( gpuType & VEglDriver::GpuType::GPU_TYPE_MALI ) != 0 )		// or the GPU is Mali
    {
        vInfo( "Performing an initial swapbuffers for Mali and/or Android-L" );
        // When we use the protected Trust Zone framebuffer there is trash in the
        // swap chain, so clear it out.
        glClearColor( 0, 0, 0, 0 );
        glClear( GL_COLOR_BUFFER_BIT );
        eglSwapBuffers( m_display, windowSurface );	// swap buffer will operate que/deque related process internally
        // now ready to set usage to proper surface
    }

    // Get the surface size.
    eglQuerySurface( m_display, windowSurface, EGL_WIDTH, &m_width );
    eglQuerySurface( m_display, windowSurface, EGL_HEIGHT, &m_height );

    if ( !wantFrontBuffer_ )
    {
        vInfo( "Running without front buffer");
    }
    else
    {
        m_surfaceManager.init( jni );

        m_gvrFrontbufferExtension = m_surfaceManager.setFrontBuffer( windowSurface, m_wantFrontBuffer );

        //TODO::Compare the EGLConfig which create the EGLSurface
        		vInfo("<<---EGLConfig attributes list start--->>");
        		EGLint value = 0;

        		eglGetConfigAttrib( m_eglStatus.m_display, eglConfig, EGL_ALPHA_SIZE, &value);
        		vInfo("EGL_ALPHA_SIZE : " <<value);
        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_ALPHA_MASK_SIZE, &value);
        		vInfo("EGL_ALPHA_MASK_SIZE : "<<value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_BIND_TO_TEXTURE_RGB, &value);
        		if (value==EGL_TRUE)
        		{
        			vInfo("EGL_BIND_TO_TEXTURE_RGB : TRUE");
        		}
        		else
        			vInfo("EGL_BIND_TO_TEXTURE_RGB : false");

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_BIND_TO_TEXTURE_RGBA, &value);
        		if (value==EGL_TRUE)
        		{
        			vInfo("EGL_BIND_TO_TEXTURE_RGBA : TRUE");
        		}
        		else
        			vInfo("EGL_BIND_TO_TEXTURE_RGBA : false");

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_BLUE_SIZE, &value);
        		vInfo("EGL_BLUE_SIZE : " << value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_BUFFER_SIZE, &value);
        		vInfo("EGL_BUFFER_SIZE : " << value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_COLOR_BUFFER_TYPE, &value);
        		if (value == EGL_RGB_BUFFER)
        		{
        			vInfo("EGL_COLOR_BUFFER_TYPE : EGL_RGB_BUFFER");
        		}
        		else
        		{
        			vInfo("EGL_COLOR_BUFFER_TYPE : EGL_LUMINANCE_BUFFER");
        		}

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_CONFIG_CAVEAT, &value);
        		if (value == EGL_SLOW_CONFIG)
        		{
        			vInfo("EGL_CONFIG_CAVEAT : EGL_SLOW_CONFIG");
        		}
        		else if (value == EGL_NON_CONFORMANT_CONFIG)
        		{
        			vInfo("EGL_CONFIG_VAVEAT : EGL_NON_CONFORMANT_CONFIG");
        		}
        		else
        		{
        			vInfo("EGL_CONFIG_VAVEAT : EGL_NONE");
        		}

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_CONFIG_ID, &value);
        		vInfo("EGL_CONFIG_ID : " << value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_CONFORMANT, &value);
        		vInfo("EGL_CONFORMANT : " << value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_DEPTH_SIZE, &value);
        		vInfo("EGL_DEPTH_SIZE : "<< value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_GREEN_SIZE, &value);
        		vInfo("EGL_GREEN_SIZE : " << value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_LEVEL, &value);
        		vInfo("EGL_LEVEL : " << value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_LUMINANCE_SIZE, &value);
        		vInfo("EGL_LUMINANCE_SIZE : " << value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_MAX_PBUFFER_WIDTH, &value);
        		vInfo("EGL_MAX_PBUFFER_WIDTH : " << value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_MAX_PBUFFER_HEIGHT, &value);
        		vInfo("EGL_MAX_PBUFFER_HEIGHT : " << value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_MAX_SWAP_INTERVAL, &value);
        		vInfo("EGL_MAX_SWAP_INTERVAL : " << value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_MIN_SWAP_INTERVAL, &value);
        		vInfo("EGL_MIN_SWAP_INTERVAL : " << value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_NATIVE_RENDERABLE, &value);
        		if(value == EGL_TRUE)
        		{
        			vInfo("EGL_NATIVE_RENDERABLE : TRUE");
        		}
        		else
        		{
        			vInfo("EGL_NATIVE_RENDERABLE : FALSE");
        		}

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_NATIVE_VISUAL_ID, &value);
        		vInfo("EGL_NATIVE_VISUAL_ID : " << value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_NATIVE_VISUAL_TYPE, &value);
        		vInfo("EGL_NATIVE_VISUAL_TYPE : " << value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_RED_SIZE, &value);
        		vInfo("EGL_RED_SIZE : " << value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_RENDERABLE_TYPE, &value);
        		vInfo("EGL_RENDERABLE_TYPE : " << value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_SAMPLE_BUFFERS, &value);
        		vInfo("EGL_SAMPLE_BUFFERS : " << value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_SAMPLES, &value);
        		vInfo("EGL_SAMPLES : "<< value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_STENCIL_SIZE, &value);
        		vInfo("EGL_STENCIL_SIZE : "<< value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_SURFACE_TYPE, &value);
        		vInfo("EGL_SURFACE_TYPE : "<< value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_TRANSPARENT_TYPE, &value);
        		vInfo("EGL_TRANSPARENT_TYPE £º " <<value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_TRANSPARENT_RED_VALUE, &value);
        		vInfo("EGL_TRANSPARENT_RED_VALUE" << value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_TRANSPARENT_GREEN_VALUE, &value);
        		vInfo("EGL_TRANSPARENT_GREEN_VALUE" << value);

        		eglGetConfigAttrib(m_eglStatus.m_display, eglConfig, EGL_TRANSPARENT_BLUE_VALUE, &value);
        		vInfo("EGL_TRANSPARENT_BLUE_VALUE" << value);

        		vInfo("<<---EGLConfig attributes list END--->>");
        		//END THE COMPARE

        		//TODO::EGLSurface attributes
        		EGLDisplay tmpdisplay = m_eglStatus.m_display;

        		EGLSurface tmpsurface = windowSurface;

        		vInfo("<<---EGLSurface attributes list start--->>");

        		eglQuerySurface(tmpdisplay, tmpsurface, EGL_CONFIG_ID, &value);
        		vInfo("EGL_CONFIG_ID : "<< value);

        		eglQuerySurface(tmpdisplay, tmpsurface, EGL_HEIGHT, &value);
        		vInfo("EGL_HEIGHT : " << value);

        		eglQuerySurface(tmpdisplay, tmpsurface, EGL_HORIZONTAL_RESOLUTION, &value);
        		vInfo("EGL_HORIZONTAL_RESOLUTION : " << value);

        		eglQuerySurface(tmpdisplay, tmpsurface, EGL_LARGEST_PBUFFER, &value);
        		vInfo("EGL_LARGEST_PBUFFER :" << value);

        		eglQuerySurface(tmpdisplay, tmpsurface, EGL_MIPMAP_LEVEL, &value);
        		vInfo("EGL_MIPMAP_LEVEL : " << value);

        		eglQuerySurface(tmpdisplay, tmpsurface, EGL_MIPMAP_TEXTURE, &value);
        		if (value == EGL_TRUE)
        		{
        			vInfo("EGL_MIPMAP_TEXTURE : TRUE" );
        		}
        		else
        			vInfo("EGL_MIPMAP_TEXTURE : FALSE");

        		eglQuerySurface(tmpdisplay, tmpsurface, EGL_MULTISAMPLE_RESOLVE, &value);
        		if (value == EGL_MULTISAMPLE_RESOLVE_DEFAULT )
        		{
        			vInfo("EGL_MULTISAMPLE_RESOLVE : EGL_MULTISAMPLE_RESOLVE_DEFAULT ");
        		}
        		else
        			vInfo("EGL_MULTISAMPLE_RESOLVE : EGL_MULTISAMPLE_RESOLVE_BOX");

        		eglQuerySurface(tmpdisplay, tmpsurface, EGL_PIXEL_ASPECT_RATIO, &value);
        		vInfo("EGL_PIXEL_ASPECT_RATIO : " << value);

        		eglQuerySurface(tmpdisplay, tmpsurface, EGL_RENDER_BUFFER, &value);
        		if (value == EGL_BACK_BUFFER)
        		{
        			vInfo("EGL_RENDER_BUFFER : EGL_BACK_BUFFER");
        		}
        		else if (value == EGL_SINGLE_BUFFER)
        		{
        			vInfo("EGL_RENDER_BUFFER : EGL_SINGLE_BUFFER");
        		}
        		else
        			vInfo("EGL_RENDER_BUFFER : OTHER_BUFFER");

        		eglQuerySurface(tmpdisplay, tmpsurface, EGL_SWAP_BEHAVIOR, &value);
        		if (value == EGL_BUFFER_PRESERVED)
        		{
        			vInfo("EGL_SWAP_BEHAVIOR : EGL_BUFFER_PRESERVED");
        		}
        		else if (value == EGL_BUFFER_DESTROYED)
        		{
        			vInfo("EGL_SWAP_BEHAVIOR : EGL_BUFFER_DESTROYED");
        		}

        		eglQuerySurface(tmpdisplay, tmpsurface, EGL_TEXTURE_FORMAT, &value);
        		if(value == EGL_NO_TEXTURE)
        		{
        			vInfo("EGL_TEXTURE_FORMAT : EGL_NO_TEXTURE");
        		}
        		else if (value == EGL_TEXTURE_RGB)
        		{
        			vInfo("EGL_TEXTURE_FORMAT : EGL_TEXTURE_RGB");
        		}
        		else if (value == EGL_TEXTURE_RGBA)
        		{
        			vInfo("EGL_TEXTURE_FORMAT : EGL_TEXTURE_RGBA");
        		}

        		eglQuerySurface(tmpdisplay , tmpsurface, EGL_TEXTURE_TARGET, &value);
        		if (value == EGL_NO_TEXTURE)
        		{
        			vInfo("EGL_TEXTURE_TARGET : EGL_NO_TEXTURE");
        		}
        		else if (value == EGL_TEXTURE_2D)
        		{
        			vInfo("EGL_TEXTURE_TARGET : EGL_TEXTURE_2D");
        		}

        		eglQuerySurface(tmpdisplay, tmpsurface, EGL_VERTICAL_RESOLUTION, &value);
        		vInfo("EGL_VERTICAL_RESOLUTION : " << value);

        		eglQuerySurface(tmpdisplay, tmpsurface, EGL_WIDTH, &value);
        		vInfo("EGL_WIDTH : " << value);

        		vInfo("<<---EGLSurface attributes list end--->>");

				vInfo("<<---EGLContext attributes list start-->>");

				EGLContext tmpcontext = eglGetCurrentContext();
				eglQueryContext(tmpdisplay, tmpcontext, EGL_CONFIG_ID, &value);
				vInfo("EGL_CONFIG_ID : " << value);

				eglQueryContext(tmpdisplay, tmpcontext, EGL_CONTEXT_CLIENT_TYPE, &value);
				if (value == EGL_OPENGL_API)
				{
					vInfo("EGL_CONTEXT_CLIENT_TYPE : EGL_OPENGL_API");
				}
				else if (value == EGL_OPENGL_ES_API)
				{
					vInfo("EGL_CONTEXT_CLIENT_TYPE : EGL_OPENGL_ES_API");
				}
				else if (value == EGL_OPENVG_API)
				{
					vInfo("EGL_CONTEXT_CLIENT_TYPE : EGL_OPENVG_API");
				}

				eglQueryContext(tmpdisplay, tmpcontext, EGL_CONTEXT_CLIENT_VERSION, &value);
				vInfo("EGL_CONTEXT_CLIENT_VERSION : "<< value);

				eglQueryContext(tmpdisplay, tmpcontext, EGL_RENDER_BUFFER, &value);
				if (value == EGL_SINGLE_BUFFER)
				{
					vInfo("EGL_RENDER_BUFFER : EGL_SINGLE_BUFFER");
				}
				else if (value == EGL_BACK_BUFFER)
				{
					vInfo("EGL_RENDER_BUFFER : EGL_SINGLE_BUFFER");
				}
				else
				{
					vInfo("EGL_RENDER_BUFFER : EGL_NONE");
				}

				vInfo("<<---EGLContext attributes list end--->>");


        EGLSurface windowsurface = eglGetCurrentSurface(EGL_DRAW);
		eglQuerySurface(m_display, windowsurface, EGL_RENDER_BUFFER, &res);

		if (res == EGL_SINGLE_BUFFER)
		{
			vInfo("single buffer is used!");
		}
		else if (res == EGL_BACK_BUFFER)
		{
			vInfo("back buffer is used!");
		}
		else
		{
			vInfo("error no buffer is used!");
		}

        if(m_gvrFrontbufferExtension) vInfo( "Running with front buffer")
        else vInfo( "Running without front buffer");

        if ( ( gpuType & VEglDriver::GpuType::GPU_TYPE_MALI ) != 0 )
        {
            vInfo( "Mali GPU" );
            tilerControl = FB_MALI;
        }
        else if ( ( gpuType & VEglDriver::GpuType::GPU_TYPE_ADRENO ) != 0 )
        {
            // Query the number of samples on the display
            EGLint configID;
            if ( !eglQueryContext( m_display, m_context, EGL_CONFIG_ID, &configID ) )
            {
                vFatal( "eglQueryContext EGL_CONFIG_ID failed" );
            }
            EGLConfig eglConfig = m_eglStatus.eglConfigForConfigID(configID );
            if ( eglConfig == NULL )
            {
                vFatal( "EglConfigForConfigID failed" );
            }
            EGLint samples = 0;
            eglGetConfigAttrib( m_display, eglConfig, EGL_SAMPLES, &samples );

            if ( gpuType == VEglDriver::GpuType::GPU_TYPE_ADRENO_330 )
            {
                vInfo( "Adreno 330 GPU" );
                tilerControl = FB_TILED_RENDERING;
            }
            else
            {
                vInfo( "Adreno GPU" );

                // NOTE: On KitKat, only tiled render mode will continue to work
                // with multisamples set on the frame buffer (at a performance
                // loss). On Lollipop, having multisamples set on the frame buffer
                // is an error for all render modes and will result in a black screen.
                if ( samples != 0 )
                {
                    // TODO: We may want to make this a FATAL ERROR.

                    vWarn( "**********************************************" );
                    vWarn( "ERROR: frame buffer uses MSAA - turn off MSAA!" );
                    vWarn( "**********************************************" );
                    tilerControl = FB_TILED_RENDERING;
                }
                else
                {
                    // NOTE: Currently (2014-11-19) the memory controller
                    // clock is not fixed when running with fixed CPU/GPU levels.
                    // For direct render mode, the fluctuation may cause significant
                    // performance issues.

                    // FIXME: Enable tiled render mode for now until we are able
                    // to run with fixed memory clock.
                    tilerControl = FB_TILED_RENDERING;
                }
            }
        }

        // draw stuff to the screen without swapping to see if it is working
        // ExerciseFrontBuffer();
    }
}

void DirectRender::shutdown()
{
    vInfo( "DirectRender::Shutdown");
    if ( m_wantFrontBuffer )
    {
        if ( windowSurface != EGL_NO_SURFACE )
        {
            m_surfaceManager.setFrontBuffer( windowSurface, false );
            windowSurface = EGL_NO_SURFACE;
        }

        m_surfaceManager.shutdown();
    }
}

// Conventionally swapped rendering will return false.
bool DirectRender::isFrontBuffer() const
{
    return m_wantFrontBuffer && m_gvrFrontbufferExtension;
}

void DirectRender::beginDirectRendering( int x, int y, int width, int height )
{
    switch( tilerControl )
    {
        case FB_TILED_RENDERING:
        {
            m_eglStatus.glStartTilingQCOM( x, y, width, height, 0 );
            glScissor( x, y, width, height );
            break;
        }
        case FB_BINNING_CONTROL:
        {
            glHint( GL_BINNING_CONTROL_HINT_QCOM, GL_RENDER_DIRECT_TO_FRAMEBUFFER_QCOM );
            glScissor( x, y, width, height );
            break;
        }
        case FB_WRITEONLY_RENDERING:
        {
            glEnable( GL_WRITEONLY_RENDERING_QCOM );
            glScissor( x, y, width, height );
            break;
        }
        case FB_MALI:
        {
            const GLenum attachments[3] = { GL_COLOR, GL_DEPTH, GL_STENCIL };
            m_eglStatus.glInvalidateFramebuffer( GL_FRAMEBUFFER, 3, attachments );
            glScissor( x, y, width, height );
            // This clear is not absolutely necessarily but ARM prefers an explicit glClear call to avoid ambiguity.
            //glClearColor( 0, 0, 0, 1 );
            //glClear( GL_COLOR_BUFFER_BIT );
            break;
        }
        default:
        {
            glScissor( x, y, width, height );
            break;
        }
    }
}

void DirectRender::endDirectRendering() const
{
    switch( tilerControl )
    {
        case FB_TILED_RENDERING:
        {
            m_eglStatus.glEndTilingQCOM( GL_COLOR_BUFFER_BIT0_QCOM );
            break;
        }
        case FB_BINNING_CONTROL:
        {
            glHint( GL_BINNING_CONTROL_HINT_QCOM, GL_DONT_CARE );
            // Flush explicitly
            glFlush();		// GL_Flush() with KHR_sync seems to be synchronous
            break;
        }
        case FB_WRITEONLY_RENDERING:
        {
            glDisable( GL_WRITEONLY_RENDERING_QCOM );
            // Flush explicitly
            glFlush();		// GL_Flush() with KHR_sync seems to be synchronous
            break;
        }
        case FB_MALI:
        {
            const GLenum attachments[2] = { GL_DEPTH, GL_STENCIL };
            m_eglStatus.glInvalidateFramebuffer( GL_FRAMEBUFFER, 2, attachments );
            // Flush explicitly
            glFlush();		// GL_Flush() with KHR_sync seems to be synchronous
            break;
        }
        default:
        {
            glFlush();		// GL_Flush() with KHR_sync seems to be synchronous
            break;
        }
    }
}

void DirectRender::swapBuffers() const
{
    eglSwapBuffers( m_display, windowSurface );
}

void DirectRender::getScreenResolution( int & width_, int & height_ ) const
{
    height_ = m_height;
    width_ = m_width;
}

NV_NAMESPACE_END
