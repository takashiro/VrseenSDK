#include <VAlgorithm.h>

#include "VRLauncher.h"
#include <android/keycodes.h>
#include "FileLoader.h"
#include "core/VTimer.h"
#include "VTileButton.h"

#include <android/JniUtils.h>
#include <VZipFile.h>
#include <VThread.h>
#include <VStandardPath.h>
#include <VFile.h>
#include <VLog.h>
#include <VImage.h>
#include <GazeCursor.h>
#include <VGui.h>
#include <VTexture.h>
#include "VTileButton.h"
#include <VTexture.h>
#include <VFile.h>

NV_NAMESPACE_BEGIN

static const char * DEFAULT_PANO = "assets/placeholderBackground.jpg";

extern "C" {

void Java_com_vrseen_vrlauncher_VRLauncher_construct(JNIEnv *jni, jclass, jobject activity)
{
    // This is called by the java UI thread.
    (new VRLauncher(jni, jni->GetObjectClass(activity), activity))->onCreate(nullptr, nullptr, nullptr);
}

void Java_com_vrseen_vrlauncher_VRLauncher_setBackground(JNIEnv *jni, jclass, jstring jpath)
{
    VRLauncher *photo = (VRLauncher *) vApp->appInterface();
    photo->onStart(JniUtils::Convert(jni, jpath));
}

} // extern "C"

VRLauncher::DoubleBufferedTextureData::DoubleBufferedTextureData()
    : CurrentIndex( 0 )
{
    for ( int i = 0; i < 2; ++i )
    {
        TexId[ i ] = 0;
        Width[ i ] = 0;
        Height[ i ] = 0;
    }
}

VRLauncher::DoubleBufferedTextureData::~DoubleBufferedTextureData()
{
    glDeleteTextures(1, &TexId[0]);
    glDeleteTextures(1, &TexId[1]);
}

GLuint VRLauncher::DoubleBufferedTextureData::GetRenderTexId() const
{
    return TexId[ CurrentIndex ^ 1 ];
}

GLuint VRLauncher::DoubleBufferedTextureData::GetLoadTexId() const
{
    return TexId[ CurrentIndex ];
}

void VRLauncher::DoubleBufferedTextureData::SetLoadTexId( const GLuint texId )
{
    TexId[ CurrentIndex ] = texId;
}

void VRLauncher::DoubleBufferedTextureData::Swap()
{
    CurrentIndex ^= 1;
}

void VRLauncher::DoubleBufferedTextureData::SetSize( const int width, const int height )
{
    Width[ CurrentIndex ] = width;
    Height[ CurrentIndex ] = height;
}

bool VRLauncher::DoubleBufferedTextureData::SameSize( const int width, const int height ) const
{
    return ( Width[ CurrentIndex ] == width && Height[ CurrentIndex ] == height );
}

VRLauncher::VRLauncher(JNIEnv *jni, jclass activityClass, jobject activityObject)
    : VMainActivity(jni, activityClass, activityObject)
    , m_currentPanoIsCubeMap( false )
    , m_menuState( MENU_NONE )
    , m_useOverlay( true )
    , m_useSrgb( true )
    , m_backgroundCommands( 100 )
    , m_eglClientVersion( 0 )
    , m_eglDisplay( 0 )
    , m_eglConfig( 0 )
    , m_eglPbufferSurface( 0 )
    , m_eglShareContext( 0 )
{
    m_shutdownRequest.setState( false );
}

VRLauncher::~VRLauncher()
{
}

//============================================================================================

void VRLauncher::init(const VString &, const VString &, const VString &)
{
    // This is called by the VR thread, not the java UI thread.
    vInfo("--------------- PanoPhoto OneTimeInit ---------------");

    //-------------------------------------------------------------------------
    m_texturedMvpProgram.initShader(VGlShader::getTexturedMvpVertexShaderSource(),VGlShader::getUniformTextureProgramShaderSource());
    m_cubeMapPanoProgram.initShader(VGlShader::getCubeMapPanoVertexShaderSource(),VGlShader::getCubeMapPanoProgramShaderSource());

    // launch cube pano -should always exist!
    m_PhotoUrl = DEFAULT_PANO;

    vInfo("Creating Globe");
    m_globe.createSphere();

    // Stay exactly at the origin, so the panorama globe is equidistant
    // Don't clear the head model neck length, or swipe view panels feel wrong.
    VViewSettings viewParms = vApp->viewSettings();
    viewParms.eyeHeight = 0.0f;
    vApp->setViewSettings( viewParms );

    // Optimize for 16 bit depth in a modest globe size
    m_scene.Znear = 0.1f;
    m_scene.Zfar = 200.0f;

    InitFileQueue( vApp, this );

    //---------------------------------------------------------
    // OpenGL initialization for shared context for
    // background loading thread done on the main thread
    //---------------------------------------------------------

    // Get values for the current OpenGL context
    m_eglDisplay = eglGetCurrentDisplay();
    if ( m_eglDisplay == EGL_NO_DISPLAY )
    {
        vFatal("EGL_NO_DISPLAY");
    }

    m_eglShareContext = eglGetCurrentContext();
    if ( m_eglShareContext == EGL_NO_CONTEXT )
    {
        vFatal("EGL_NO_CONTEXT");
    }

    EGLint attribList[] =
    {
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    EGLint numConfigs;
    if ( !eglChooseConfig( m_eglDisplay, attribList, &m_eglConfig, 1, &numConfigs ) )
    {
        vFatal("eglChooseConfig failed");
    }

    if ( m_eglConfig == NULL )
    {
        vFatal("EglConfig NULL");
    }
    if ( !eglQueryContext( m_eglDisplay, m_eglShareContext, EGL_CONTEXT_CLIENT_VERSION, ( EGLint * )&m_eglClientVersion ) )
    {
        vFatal("eglQueryContext EGL_CONTEXT_CLIENT_VERSION failed");
    }
    vInfo("Current EGL_CONTEXT_CLIENT_VERSION:" << m_eglClientVersion);

    EGLint SurfaceAttribs [ ] =
    {
        EGL_WIDTH, 1,
        EGL_HEIGHT, 1,
        EGL_NONE
    };


    m_eglPbufferSurface = eglCreatePbufferSurface( m_eglDisplay, m_eglConfig, SurfaceAttribs );
    if ( m_eglPbufferSurface == EGL_NO_SURFACE ) {
        vFatal("eglCreatePbufferSurface failed:" << VEglDriver::getEglErrorString());
    }
    EGLint bufferWidth, bufferHeight;
    if ( !eglQuerySurface( m_eglDisplay, m_eglPbufferSurface, EGL_WIDTH, &bufferWidth ) ||
         !eglQuerySurface( m_eglDisplay, m_eglPbufferSurface, EGL_HEIGHT, &bufferHeight ) )
    {
        vFatal("eglQuerySurface failed:" << VEglDriver::getEglErrorString());
    }

    // spawn the background loading thread with the command list
    pthread_attr_t loadingThreadAttr;
    pthread_attr_init( &loadingThreadAttr );
    sched_param sparam;
    sparam.sched_priority = VThread::GetOSPriority(VThread::NormalPriority);
    pthread_attr_setschedparam( &loadingThreadAttr, &sparam );

    pthread_t loadingThread;
    const int createErr = pthread_create( &loadingThread, &loadingThreadAttr, &BackgroundGLLoadThread, this );
    if ( createErr != 0 )
    {
        vInfo("pthread_create returned" << createErr);
    }

    // We might want to save the view state and position for perfect recall
    VString sdcard = "/storage/emulated/0/VRSeen/SDK/VRLauncher/";
    const char *buttonImages[4] = {"game1.jpg", "game2.jpg", "video1.jpg", "video2.jpg"};
    VTileButton *buttons[4];

    VGui *gui = vApp->gui();
    VRect3f buttonSize(-0.8f, -0.6f, 0.0f, 0.8f, 0.6f, 0.0f);
    for (int i = 0; i < 4; i++){
        VTileButton *button = new VTileButton;
        buttons[i] = button;
        VFile image(sdcard + buttonImages[i], VFile::ReadOnly);
        button->setRect(buttonSize);
        button->setImage(image);
        gui->addItem(button);
    }

    buttons[0]->setPos(VVect3f(-0.85f, 0.65f, -3.0f));
    buttons[1]->setPos(VVect3f(-0.85f, -0.65f, -3.0f));
    buttons[2]->setPos(VVect3f(0.85f, 0.65f, -3.0f));
    buttons[3]->setPos(VVect3f(0.85f, -0.65f, -3.0f));
}

//============================================================================================
void VRLauncher::onStart(const VString &url)
{
    m_PhotoUrl = url;
    if (!m_PhotoUrl.isEmpty()) {
        SetMenuState( MENU_BACKGROUND_INIT );
        vInfo("StartPhoto(" << m_PhotoUrl << ")");
	}
}


void VRLauncher::shutdown()
{
    // This is called by the VR thread, not the java UI thread.
    vInfo("--------------- PanoPhoto OneTimeShutdown ---------------");

    // Shut down background loader
    m_shutdownRequest.setState( true );

    m_globe.destroy();

    m_texturedMvpProgram.destroy();
    m_cubeMapPanoProgram.destroy();

    if ( eglDestroySurface( m_eglDisplay, m_eglPbufferSurface ) == EGL_FALSE )
    {
        vFatal("eglDestroySurface: shutdown failed");
    }
}

void * VRLauncher::BackgroundGLLoadThread( void * v )
{
    pthread_setname_np( pthread_self(), "BackgrndGLLoad" );

    VRLauncher * photos = ( VRLauncher * )v;

    // Create a new GL context on this thread, sharing it with the main thread context
    // so the loaded background texture can be passed.
    EGLint loaderContextAttribs [ ] =
    {
        EGL_CONTEXT_CLIENT_VERSION, photos->m_eglClientVersion,
        EGL_NONE, EGL_NONE,
        EGL_NONE
    };


    EGLContext EglBGLoaderContext = eglCreateContext( photos->m_eglDisplay, photos->m_eglConfig, photos->m_eglShareContext, loaderContextAttribs );
    if ( EglBGLoaderContext == EGL_NO_CONTEXT )
    {
        vFatal("eglCreateContext failed:" << VEglDriver::getEglErrorString());
    }

    // Make the context current on the window, so no more makeCurrent calls will be needed
    if ( eglMakeCurrent( photos->m_eglDisplay, photos->m_eglPbufferSurface, photos->m_eglPbufferSurface, EglBGLoaderContext ) == EGL_FALSE )
    {
        vFatal("BackgroundGLLoadThread eglMakeCurrent failed:" << VEglDriver::getEglErrorString());
    }

    // run until Shutdown requested
    for ( ;; )
    {
        if ( photos->m_shutdownRequest.state() )
        {
            vInfo("BackgroundGLLoadThread ShutdownRequest received");
            break;
        }

        photos->m_backgroundCommands.wait();
        VEvent event = photos->m_backgroundCommands.next();
        vInfo("BackgroundGLLoadThread Commands:" << event.name);
        if (event.name == "pano") {
            uchar *data = static_cast<uchar *>(event.data.at(0).toPointer());
            int width = event.data.at(1).toInt();
            int height = event.data.at(2).toInt();

            const double start = VTimer::Seconds( );

            // Resample oversize images so gl can load them.
            // We could consider resampling to GL_MAX_TEXTURE_SIZE exactly for better quality.
            GLint maxTextureSize = 0;
            glGetIntegerv( GL_MAX_TEXTURE_SIZE, &maxTextureSize );

            VImage image(data, width, height);
            data = nullptr;
            while (width > maxTextureSize || width > maxTextureSize) {
                vInfo("Quartering oversize" << width << height << "image");
                image.quarter(true);
                width = image.width();
                height = image.height();
            }

            photos->loadRgbaTexture(image.data(), width, height, true);

            // Add a sync object for uploading textures
            EGLSyncKHR GpuSync = VEglDriver::eglCreateSyncKHR( photos->m_eglDisplay, EGL_SYNC_FENCE_KHR, NULL );
            if ( GpuSync == EGL_NO_SYNC_KHR ) {
                vFatal("BackgroundGLLoadThread eglCreateSyncKHR_():EGL_NO_SYNC_KHR");
            }

            // Force it to flush the commands and wait until the textures are fully uploaded
            if ( EGL_FALSE == VEglDriver::eglClientWaitSyncKHR( photos->m_eglDisplay, GpuSync, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR,
                                                                EGL_FOREVER_KHR ) )
            {
                vInfo("BackgroundGLLoadThread eglClientWaitSyncKHR returned EGL_FALSE");
            }

            vApp->eventLoop().post("loaded pano");

            const double end = VTimer::Seconds();
            vInfo(end - start << "s to load" << width << height << "res pano map");
        } else if (event.name == "cube") {
            int size = event.data.at(0).toInt();
            uchar *data[6];
            for (int i = 0; i < 6; i++) {
                data[i] = static_cast<uchar *>(event.data.at(i + 1).toPointer());
            }

            const double start = VTimer::Seconds( );

            photos->loadRgbaCubeMap( size, data, true );
            for ( int i = 0; i < 6; i++ )
            {
                free( data[ i ] );
            }

            // Add a sync object for uploading textures
            EGLSyncKHR GpuSync = VEglDriver::eglCreateSyncKHR( photos->m_eglDisplay, EGL_SYNC_FENCE_KHR, NULL );
            if ( GpuSync == EGL_NO_SYNC_KHR ) {
                vFatal("BackgroundGLLoadThread eglCreateSyncKHR_():EGL_NO_SYNC_KHR");
            }

            // Force it to flush the commands and wait until the textures are fully uploaded
            if ( EGL_FALSE == VEglDriver::eglClientWaitSyncKHR( photos->m_eglDisplay, GpuSync, EGL_SYNC_FLUSH_COMMANDS_BIT_KHR,
                                                                EGL_FOREVER_KHR ) )
            {
                vInfo("BackgroundGLLoadThread eglClientWaitSyncKHR returned EGL_FALSE");
            }

            vApp->eventLoop().post("loaded cube");

            const double end = VTimer::Seconds();
            vInfo(end - start << "s to load" << size << "res cube map");
        }
    }

    // release the window so it can be made current by another thread
    if ( eglMakeCurrent( photos->m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT ) == EGL_FALSE )
    {
        vFatal("BackgroundGLLoadThread eglMakeCurrent: shutdown failed");
    }

    if ( eglDestroyContext( photos->m_eglDisplay, EglBGLoaderContext ) == EGL_FALSE )
    {
        vFatal("BackgroundGLLoadThread eglDestroyContext: shutdown failed");
    }
    return NULL;
}

void VRLauncher::command(const VEvent &event )
{
    if (event.name == "loaded pano") {
        m_backgroundPanoTexData.Swap();
        m_currentPanoIsCubeMap = false;
        vApp->gazeCursor().ClearGhosts();
        return;
    }

    if (event.name == "loaded cube") {
        m_backgroundCubeTexData.Swap();
        m_currentPanoIsCubeMap = true;
        vApp->gazeCursor().ClearGhosts();
        return;
    }
}

bool VRLauncher::useOverlay() const {
    // Don't enable the overlay when in throttled state
    return m_useOverlay;
}

void VRLauncher::configureVrMode(VKernel* kernel) {
    // We need very little CPU for pano browsing, but a fair amount of GPU.
    // The CPU clock should ramp up above the minimum when necessary.
    vInfo("ConfigureClocks: PanoPhoto only needs minimal clocks");

    // No hard edged geometry, so no need for MSAA
    kernel->msaa = 1;
}

void VRLauncher::loadRgbaCubeMap( const int resolution, const uchar * const rgba[ 6 ], const bool useSrgbFormat )
{

    VEglDriver::logErrorsEnum( "enter LoadRgbaCubeMap" );

    const GLenum glFormat = GL_RGBA;
    const GLenum glInternalFormat = useSrgbFormat ? GL_SRGB8_ALPHA8 : GL_RGBA;

    // Create texture storage once
    GLuint texId = m_backgroundCubeTexData.GetLoadTexId();
    if ( texId == 0 || !m_backgroundCubeTexData.SameSize( resolution, resolution ) )
    {
        glDeleteTextures(1, &texId);
        glGenTextures( 1, &texId );
        glBindTexture( GL_TEXTURE_CUBE_MAP, texId );
        glTexStorage2D( GL_TEXTURE_CUBE_MAP, 1, glInternalFormat, resolution, resolution );
        for ( int side = 0; side < 6; side++ )
        {
            glTexSubImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, 0, 0, 0, resolution, resolution, glFormat, GL_UNSIGNED_BYTE, rgba[ side ] );
        }
        m_backgroundCubeTexData.SetSize( resolution, resolution );
        m_backgroundCubeTexData.SetLoadTexId( texId );
    }
    else // reuse the texture storage
    {
        glBindTexture( GL_TEXTURE_CUBE_MAP, texId );
        for ( int side = 0; side < 6; side++ )
        {
            glTexSubImage2D( GL_TEXTURE_CUBE_MAP_POSITIVE_X + side, 0, 0, 0, resolution, resolution, glFormat, GL_UNSIGNED_BYTE, rgba[ side ] );
        }

    }
    glGenerateMipmap( GL_TEXTURE_CUBE_MAP );

    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

    glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );

    VEglDriver::logErrorsEnum( "leave LoadRgbaCubeMap" );
}

void VRLauncher::loadRgbaTexture( const unsigned char * data, int width, int height, const bool useSrgbFormat )
{

    VEglDriver::logErrorsEnum( "enter LoadRgbaTexture" );

    const GLenum glFormat = GL_RGBA;
    const GLenum glInternalFormat = useSrgbFormat ? GL_SRGB8_ALPHA8 : GL_RGBA;

    // Create texture storage once
    GLuint texId = m_backgroundPanoTexData.GetLoadTexId();
    if ( texId == 0 || !m_backgroundPanoTexData.SameSize( width, height ) )
    {
        glDeleteTextures(1, &texId);
        glGenTextures( 1, &texId );
        glBindTexture( GL_TEXTURE_2D, texId );
        glTexStorage2D( GL_TEXTURE_2D, 1, glInternalFormat, width, height );
        glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, width, height, glFormat, GL_UNSIGNED_BYTE, data );
        m_backgroundPanoTexData.SetSize( width, height );
        m_backgroundPanoTexData.SetLoadTexId( texId );
    }
    else // reuse the texture storage
    {
        glBindTexture( GL_TEXTURE_2D, texId );
        glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, width, height, glFormat, GL_UNSIGNED_BYTE, data );
    }

    VTexture texture(texId);
    texture.buildMipmaps();
    texture.trilinear();

    glBindTexture( GL_TEXTURE_2D, texId );
    // Because equirect panos pinch at the poles so much,
    // they would pull in mip maps so deep you would see colors
    // from the opposite half of the pano.  Clamping the level avoids this.
    // A well filtered pano shouldn't have any high frequency texels
    // that alias near the poles.
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 2 );
    glBindTexture( GL_TEXTURE_2D, 0 );

    VEglDriver::logErrorsEnum( "leave LoadRgbaTexture" );
}

VMatrix4f CubeMatrixForViewMatrix( const VMatrix4f & viewMatrix )
{
    VMatrix4f m = viewMatrix;
    // clear translation
    for ( int i = 0; i < 3; i++ )
    {
        m.cell[ i ][ 3 ] = 0.0f;
    }
    return m.inverted();
}

VMatrix4f VRLauncher::drawEyeView( const int eye, const float fovDegrees )
{
    // Don't draw the scene at all if it is faded out
    const bool drawScene = true;

    const VMatrix4f view = drawScene ?
                m_scene.DrawEyeView( eye, fovDegrees )
              : m_scene.MvpForEye( eye, fovDegrees );

    //const float color = m_currentFadeLevel;
    // Dim pano when browser open
    float fadeColor = 1.0f;

    if ( useOverlay() && m_currentPanoIsCubeMap )
    {
        // Clear everything to 0 alpha so the overlay plane shows through.
        glClearColor( 0, 0, 0, 0 );
        glClear( GL_COLOR_BUFFER_BIT );

        const VMatrix4f	m( CubeMatrixForViewMatrix( m_scene.CenterViewMatrix() ) );
        GLuint texId = m_backgroundCubeTexData.GetRenderTexId();
        glBindTexture( GL_TEXTURE_CUBE_MAP, texId );
        glTexParameteri( GL_TEXTURE_CUBE_MAP, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT,
                         m_useSrgb ? VEglDriver::GL_DECODE_EXT : VEglDriver::GL_SKIP_DECODE_EXT );
        glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );


        vApp->swapParms().WarpOptions = ( m_useSrgb ? 0 : SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER );
        vApp->swapParms( ).Images[ eye ][ 1 ].TexId = texId;
        vApp->swapParms().Images[ eye ][ 1 ].TexCoordsFromTanAngles = m;
        vApp->swapParms().Images[ eye ][ 1 ].Pose = m_frameInput.pose;
        vApp->swapParms().WarpProgram = WP_CHROMATIC_MASKED_CUBE;
        for ( int i = 0; i < 4; i++ )
        {
            vApp->swapParms().ProgramParms[ i ] = fadeColor;
        }


//        vApp->kernel()->m_smoothOptions = ( m_useSrgb ? 0 : VK_INHIBIT_SRGB_FB );
//        vApp->kernel()->m_texId[ eye ][ 1 ] = texId;
//        vApp->kernel()->m_texMatrix[ eye ][ 1 ] = m;
//
//        VRotationState &pose = vApp->kernel()->m_pose[ eye ][ 1 ];
//        pose = m_frameInput.pose;
//        vApp->kernel()->m_smoothProgram = VK_CUBE_CB;
//        for ( int i = 0; i < 4; i++ )
//        {
//            vApp->kernel()->m_programParms[ i ] = fadeColor;
//        }
    }
    else
    {
        vApp->swapParms().WarpOptions = m_useSrgb ? 0 : SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER;
        vApp->swapParms().Images[ eye ][ 1 ].TexId = 0;
        vApp->swapParms().WarpProgram = WP_CHROMATIC;
        for ( int i = 0; i < 4; i++ )
        {
            vApp->swapParms().ProgramParms[ i ] = 1.0f;
        }

//        vApp->kernel()->m_smoothOptions = m_useSrgb ? 0 : VK_INHIBIT_SRGB_FB;
//        vApp->kernel()->m_texId[ eye ][ 1 ] = 0;
//        vApp->kernel()->m_smoothProgram = VK_DEFAULT_CB;
//        for ( int i = 0; i < 4; i++ )
//        {
//            vApp->kernel()->m_programParms[ i ] = 1.0f;
//        }

        glActiveTexture( GL_TEXTURE0 );
        if ( m_currentPanoIsCubeMap )
        {
            glBindTexture( GL_TEXTURE_CUBE_MAP, m_backgroundCubeTexData.GetRenderTexId( ) );
            glTexParameteri( GL_TEXTURE_CUBE_MAP, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT,
                             m_useSrgb ? VEglDriver::GL_DECODE_EXT : VEglDriver::GL_SKIP_DECODE_EXT );
        }
        else
        {
            glBindTexture( GL_TEXTURE_2D, m_backgroundPanoTexData.GetRenderTexId( ) );
            glTexParameteri( GL_TEXTURE_2D, VEglDriver::GL_TEXTURE_SRGB_DECODE_EXT,
                             m_useSrgb ? VEglDriver::GL_DECODE_EXT : VEglDriver::GL_SKIP_DECODE_EXT );
        }

        VGlShader & prog = m_currentPanoIsCubeMap ? m_cubeMapPanoProgram : m_texturedMvpProgram;

        glUseProgram( prog.program );

        glUniform4f( prog.uniformColor, fadeColor, fadeColor, fadeColor, fadeColor );
        glUniformMatrix4fv( prog.uniformModelViewProMatrix, 1, GL_FALSE /* not transposed */,
                            view.transposed().cell[ 0 ] );

        m_globe.drawElements();

        glBindTexture( GL_TEXTURE_CUBE_MAP, 0 );
        glBindTexture( GL_TEXTURE_2D, 0 );
    }


   VEglDriver::logErrorsEnum( "photo draw" );
    return view;
}


float Fade( double now, double start, double length )
{
    return VAlgorithm::Clamp( ( ( now - start ) / length ), 0.0, 1.0 );
}

void VRLauncher::startBackgroundPanoLoad(const VString &filename)
{
    vInfo("StartBackgroundPanoLoad" << filename);

    // Queue1 will determine if this is a cube map and then post a message for each
    // cube face to the other queues.
    char const * command = filename.endsWith("_nz.jpg", false) ? "cube" : "pano";

    // Dump any load that hasn't started
    Queue1.clear();

    // Start a background load of the current pano image
    Queue1.post(command, filename);
}

void VRLauncher::SetMenuState( const VRLauncher::OvrMenuState state )
{
    m_menuState = state;

    switch ( m_menuState )
    {
    case MENU_NONE:
        break;
    case MENU_BACKGROUND_INIT:
        startBackgroundPanoLoad(m_PhotoUrl);
        break;
    default:
        vAssert( false );
        break;
    }
}

VMatrix4f VRLauncher::onNewFrame( const VFrame vrFrame )
{
    m_frameInput = vrFrame;

    // disallow player movement
    VFrame vrFrameWithoutMove = vrFrame;
    vrFrameWithoutMove.input.sticks[ 0 ][ 0 ] = 0.0f;
    vrFrameWithoutMove.input.sticks[ 0 ][ 1 ] = 0.0f;
    //m_scene.Frame( vApp->vrViewParms(), vrFrameWithoutMove, vApp->swapParms().ExternalVelocity );

    m_scene.Frame( vApp->viewSettings(), vrFrameWithoutMove,  vApp->swapParms().ExternalVelocity );

    // We could disable the srgb convert on the FBO. but this is easier
    vApp->vrParms().colorFormat = m_useSrgb ? VColor::COLOR_8888_sRGB : VColor::COLOR_8888;

    return m_scene.CenterViewMatrix();
}

bool VRLauncher::wantSrgbFramebuffer() const
{
    return true;
}

NV_NAMESPACE_END
