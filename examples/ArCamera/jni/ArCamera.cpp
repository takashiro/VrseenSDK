#include "ArCamera.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <jni.h>
#include <android/keycodes.h>

#include <VPath.h>
#include <VZipFile.h>
#include <VFile.h>
#include <VResource.h>
#include <VEyeItem.h>
#include <VTimer.h>
#include <VColor.h>
#include <VArray.h>
#include <VString.h>
#include <VEglDriver.h>

#include <android/JniUtils.h>

#include "SurfaceTexture.h"
#include "VTexture.h"
#include "BitmapFont.h"
#include "GazeCursor.h"
#include "App.h"

NV_NAMESPACE_BEGIN
static const char* cameraVertexShaderSrc =
        "//uniform mat4 Mvpm;\n"
        "//uniform mat4 Texm;\n"
        "attribute vec4 Position;\n"
        "attribute vec2 TexCoord;\n"
        "uniform mediump vec4 UniformColor;\n"
        "varying  highp vec2 oTexCoord;\n"
        "//varying  lowp vec4 oColor;\n"
        "void main()\n"
        "{\n"
        "   //vec4 top = Texm * Position;\n"
        "   //vec4 bottom = Mvpm * Position;\n"
        "   //gl_Position = mix( bottom, top, TexCoord.y );\n"
        "   gl_Position = Position;\n"
        "   oTexCoord = TexCoord;\n"
        "   //oColor = VertexColor * UniformColor;\n"
        "}\n";
static const char* cameraFragmentShaderSrc =
        "#extension GL_OES_EGL_image_external : require\n"
        "uniform samplerExternalOES cam_tex;\n"
		"uniform sampler2D bg_tex;\n"
        "uniform lowp vec4 ColorBias;\n"
        "varying highp vec2 oTexCoord;\n"
        "//varying lowp vec4 oColor;\n"
        "void main()\n"
        "{\n"
		"   //gl_FragColor = vec4(1,1,1,1);\n"
        "gl_FragColor = 0.99*texture2D( cam_tex, oTexCoord ) + 0.01*texture2D( bg_tex, oTexCoord );\n"
		"//gl_FragColor = texture2D( cam_tex, oTexCoord );\n"
        "}\n";
const char * panoVertexShaderSource =
		"uniform highp mat4 Mvpm;\n"
				"uniform highp mat4 Texm;\n"
				"attribute vec4 Position;\n"
				"attribute vec2 TexCoord;\n"
				"varying  highp vec2 oTexCoord;\n"
				"void main()\n"
				"{\n"
				"   gl_Position = Mvpm * Position;\n"
				"   oTexCoord = vec2( Texm * vec4( TexCoord, 0, 1 ) );\n"
				"}\n";
const char * panoFragmentShaderSource =
		"//#extension GL_OES_EGL_image_external : require\n"
				"uniform sampler2D Texture0;\n"
				"uniform lowp vec4 UniformColor;\n"
				"uniform lowp vec4 ColorBias;\n"
				"varying highp vec2 oTexCoord;\n"
				"void main()\n"
				"{\n"
				"//if(oTexCoord[0] < 0.05) gl_FragColor = vec4(1,0,0,1);\n"
				"//else\n"
				"	gl_FragColor = texture2D( Texture0, oTexCoord );\n"
				"}\n";
extern "C" {
static GLuint createShader(const char *src,GLuint shaderType){
	GLuint shader = glCreateShader( shaderType );
	glShaderSource( shader, 1, &src, 0 );
    glCompileShader( shader );

    GLint r;
    glGetShaderiv( shader, GL_COMPILE_STATUS, &r );
    if ( r == GL_FALSE )
    {
        vInfo( "Compiling shader: "<<src<<"****** failed ******\n" );
        GLchar msg[4096];
        glGetShaderInfoLog( shader, sizeof( msg ), 0, msg );
        vInfo( msg );
        return 0;
    }
	return shader;
}
static GLuint createProgram(const char *ver, const char * fag){

	GLuint vshader = createShader(ver,GL_VERTEX_SHADER);
	GLuint fshader = createShader(fag,GL_FRAGMENT_SHADER);
	GLuint p = glCreateProgram();
    glAttachShader( p, vshader );
    glAttachShader( p, fshader );
	glLinkProgram( p );
    GLint r;
    glGetProgramiv( p, GL_LINK_STATUS, &r );
    if ( r == GL_FALSE )
    {
        GLchar msg[1024];
        glGetProgramInfoLog( p, sizeof( msg ), 0, msg );
        vFatal( "Linking program failed: "<<msg );
    }
	return p;
}
static GLuint createRect(GLuint p){
    VGlGeometry geometry;
    struct vertices_t
    {
        float positions[4][3];
        float uvs[4][2];
    }
            vertices =
            {
                    { { -0.99, -0.99, 0 }, { -0.99, 0.99, 0 }, { 0.99, -0.99, 0 }, { 0.99, 0.99, 0 } },
                    { { 0, 1 }, { 0, 0 }, { 1, 1 }, { 1, 0 } }
            };
    unsigned short indices[6] = { 0, 1, 2, 2, 1, 3 };

    geometry.vertexCount = 4;
    geometry.indexCount = 6;

    VEglDriver::glGenVertexArraysOES( 1, &geometry.vertexArrayObject );
    VEglDriver::glBindVertexArrayOES( geometry.vertexArrayObject );

    glGenBuffers( 1, &geometry.vertexBuffer );
    glBindBuffer( GL_ARRAY_BUFFER, geometry.vertexBuffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), &vertices, GL_STATIC_DRAW );

    glGenBuffers( 1, &geometry.indexBuffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, geometry.indexBuffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );


	GLuint ind = glGetAttribLocation(p,"Position");
    glEnableVertexAttribArray( ind );
    glVertexAttribPointer( ind, 3, GL_FLOAT, false,
                           sizeof( vertices.positions[0] ), (const GLvoid *)offsetof( vertices_t, positions ) );

	ind = glGetAttribLocation(p,"TexCoord");
    glEnableVertexAttribArray( ind );
    glVertexAttribPointer( ind, 2, GL_FLOAT, false,
                           sizeof( vertices.uvs[0] ), (const GLvoid *)offsetof( vertices_t, uvs ) );
    VEglDriver::glBindVertexArrayOES( 0 );
	return geometry.vertexArrayObject;
}
/*jobject Java_com_vrseen_arcamera_ArCamera_nativeGetCameraSurfaceTexture( JNIEnv *jni, jclass clazz,
        jlong appPtr )
{
    LOG( "getCameraSurfaceTexture: %i",
            (vApp->GetCameraTexture()->textureId ));
    return vApp->GetCameraTexture()->javaObject;
}

void Java_com_vrseen_arcamera_ArCamera_nativeSetCameraFov( JNIEnv *jni, jclass clazz,
        jlong appPtr, jfloat fovHorizontal, jfloat fovVertical )
{
    LOG( "nativeSetCameraFov %.2f %.2f", fovHorizontal, fovVertical );
    VVariantArray args;
    args << fovHorizontal << fovVertical;
    vApp->eventLoop().post("video", std::move(args));
}*/

void Java_com_vrseen_arcamera_ArCamera_construct(JNIEnv *jni, jclass, jobject activity)
{
    (new ArCamera(jni, jni->GetObjectClass(activity), activity))->onCreate(nullptr, nullptr, nullptr);
}

void Java_com_vrseen_arcamera_ArCamera_onStart(JNIEnv *jni, jclass, jstring jpath)
{
    ArCamera *video = (ArCamera *) vApp->appInterface();
    video->onStart(JniUtils::Convert(jni, jpath));
}

void Java_com_vrseen_arcamera_ArCamera_onFrameAvailable(JNIEnv *, jclass)
{
    ArCamera *video = (ArCamera *) vApp->appInterface();
    video->setFrameAvailable(true);
}

jobject Java_com_vrseen_arcamera_ArCamera_createMovieTexture(JNIEnv *, jclass)
{
	// set up a message queue to get the return message
	// TODO: make a class that encapsulates this work
    VEventLoop result(1);
    vApp->eventLoop().post("newVideo", &result);
	result.wait();
    VEvent event = result.next();
    if (event.name == "surfaceTexture") {
        return static_cast<jobject>(event.data.toPointer());
    }
	vWarn("Failed to create movie texture");
    return NULL;
}

void Java_com_vrseen_arcamera_ArCamera_onVideoSizeChanged(JNIEnv *, jclass, jint width, jint height)
{
    VVariantArray args;
    args << width << height;
    vApp->eventLoop().post("video", std::move(args));
}

void Java_com_vrseen_arcamera_ArCamera_onCompletion(JNIEnv *, jclass)
{
	vInfo("nativeVideoCompletion");
    vApp->eventLoop().post( "completion" );
}

} // extern "C"



ArCamera::ArCamera(JNIEnv *jni, jclass activityClass, jobject activityObject)
    : VMainActivity(jni, activityClass, activityObject)
    , m_videoWasPlayingWhenPaused(false)
    , m_menuState(MENU_NONE)
    , m_useSrgb(false)
    , m_movieTexture(nullptr)
    , m_videoWidth(0)
    , m_videoHeight(480)
    , m_backgroundTexId(0)
    , m_backgroundWidth(0)
    , m_backgroundHeight(0)
    , m_frameAvailable(false)
{
}

ArCamera::~ArCamera()
{
}

void ArCamera::init(const VString &, const VString &, const VString &)
{
	vInfo("--------------- Oculus360Videos OneTimeInit ---------------");

	vApp->vrParms().colorFormat = VColor::COLOR_8888;
    vApp->vrParms().commonParameterDepth = VEyeItem::CommonParameter::DepthFormat_16;
	vApp->vrParms().multisamples = 2;

    m_panoramaProgram.initShader(VGlShader::getPanoVertexShaderSource(),VGlShader::getPanoProgramShaderSource()	);

    m_fadedPanoramaProgram.initShader(cameraVertexShaderSrc,cameraFragmentShaderSrc);
    m_singleColorTextureProgram.initShader(VGlShader::getSingleTextureVertexShaderSource(),VGlShader::getUniformSingleTextureProgramShaderSource());

	// always fall back to valid background
    if (m_backgroundTexId == 0) {
        VTexture background(VResource("assets/background.jpg"), VTexture::UseSRGB);
        m_backgroundTexId = background.id();
        vAssert(m_backgroundTexId);
        m_backgroundWidth = background.width();
        m_backgroundHeight = background.height();
	}

	vInfo("Creating Globe");
    m_globe.createSphere();

	// Stay exactly at the origin, so the panorama globe is equidistant
	// Don't clear the head model neck length, or swipe view panels feel wrong.
	VViewSettings viewParms = vApp->viewSettings();
	viewParms.eyeHeight = 0.0f;
	vApp->setViewSettings( viewParms );

	// Optimize for 16 bit depth in a modest theater size
    m_scene.Znear = 0.1f;
    m_scene.Zfar = 200.0f;

	program = createProgram(cameraVertexShaderSrc,cameraFragmentShaderSrc);
	bgProgram = createProgram(panoVertexShaderSource,panoFragmentShaderSource);
	locMVP = glGetUniformLocation(bgProgram, "Mvpm");
	locTexMatrix = glGetUniformLocation(bgProgram,"Texm");


	vao = createRect(program);
	numEyes = vApp->renderMonoMode() ? 1 : 2;

	GLint tmp0 ;
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &tmp0);
	backFbo = tmp0;

	glGenFramebuffers(1, &fbo1);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo1);
	texids = new GLuint[numEyes];
	glGenTextures(numEyes, texids);
	bufferParms = VEyeItem::settings;
	glActiveTexture(GL_TEXTURE2);
	for (int i = 0; i < numEyes; i++) {
		glBindTexture(GL_TEXTURE_2D, texids[i]);
		if (bufferParms.colorFormat == VColor::COLOR_565) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
						 bufferParms.widthScale * bufferParms.resolution,
						 bufferParms.resolution, 0,
						 GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
		} else if (bufferParms.colorFormat == VColor::COLOR_5551) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB5_A1,
						 bufferParms.widthScale * bufferParms.resolution,
						 bufferParms.resolution, 0,
						 GL_RGBA, GL_UNSIGNED_SHORT_5_5_5_1, NULL);
		} else if (bufferParms.colorFormat == VColor::COLOR_8888_sRGB) {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8,
						 bufferParms.widthScale * bufferParms.resolution,
						 bufferParms.resolution, 0,
						 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		} else {
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
						 bufferParms.widthScale * bufferParms.resolution,
						 bufferParms.resolution, 0,
						 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		switch (bufferParms.commonParameterTexture) {
			case VEyeItem::NearestTextureFilter:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				vInfo("textureFilter = TEXTURE_FILTER_NEAREST");
				break;
			case VEyeItem::BilinearTextureFilter:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				vInfo("textureFilter = TEXTURE_FILTER_BILINEAR");
				break;
			case VEyeItem::Aniso2TextureFilter:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 2);
				vInfo("textureFilter = TEXTURE_FILTER_ANISO_2");
				break;
			case VEyeItem::Aniso4TextureFilter:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4);
				vInfo("textureFilter = TEXTURE_FILTER_ANISO_4");
				break;
			default:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				vInfo("textureFilter = TEXTURE_FILTER_BILINEAR");
				break;
		}
	}
}

void ArCamera::shutdown()
{
	// This is called by the VR thread, not the java UI thread.
	vInfo("--------------- Oculus360Videos OneTimeShutdown ---------------");
    m_globe.destroy();

    glDeleteTextures(1, &m_backgroundTexId);

    if (m_movieTexture != NULL) {
        delete m_movieTexture;
        m_movieTexture = NULL;
	}

    m_panoramaProgram.destroy();
    m_fadedPanoramaProgram.destroy();
    m_singleColorTextureProgram.destroy();
}

void ArCamera::configureVrMode(VKernel* kernel)
{
	// We need very little CPU for pano browsing, but a fair amount of GPU.
	// The CPU clock should ramp up above the minimum when necessary.
	vInfo("ConfigureClocks: Oculus360Videos only needs minimal clocks");
	// All geometry is blended, so save power with no MSAA
	kernel->msaa = 1;
}

bool ArCamera::onKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType )
{
	if ( ( ( keyCode == AKEYCODE_BACK ) && ( eventType == KeyState::KEY_EVENT_SHORT_PRESS ) ) ||
		( ( keyCode == KEYCODE_B ) && ( eventType == KeyState::KEY_EVENT_UP ) ) )
	{
        if ( m_menuState == MENU_VIDEO_LOADING )
        {
            return true;
        }

		// if no video is playing (either paused or stopped), let VrLib handle the back key
    }
	else if ( keyCode == AKEYCODE_P && eventType == KeyState::KEY_EVENT_DOWN )
	{
        //pause( true );
	}

	return false;
}

void ArCamera::command(const VEvent &event )
{
	// Always include the space in MatchesHead to prevent problems
	// with commands with matching prefixes.

    if (event.name == "newVideo") {
        delete m_movieTexture;
        m_movieTexture = new SurfaceTexture( vApp->vrJni() );
        vInfo("RC_NEW_VIDEO texId" << m_movieTexture->textureId);

        VEventLoop *receiver = static_cast<VEventLoop *>(event.data.toPointer());
        receiver->post("surfaceTexture", m_movieTexture->javaObject);

		// don't draw the screen until we have the new size
        m_videoWidth = 0;
		return;

    } else if (event.name == "completion") {// video complete, return to menu
        setMenuState( MENU_BROWSER );
		return;

    } else if (event.name == "video") {
        m_videoWidth = event.data.at(0).toInt();
        m_videoHeight = event.data.at(1).toInt();

        if ( m_menuState != MENU_VIDEO_PLAYING ) // If video is already being played dont change the state to video ready
		{
            setMenuState( MENU_VIDEO_READY );
		}

		return;

    } else if (event.name == "startError") {
		// FIXME: this needs to do some parameter magic to fix xliff tags
		VString message;
//		VrLocale::GetString( vApp->vrJni(), vApp->javaObject(), "@string/playback_failed", "@string/playback_failed", message );
//        message = "@string/playback_failed";
//        VString fileName = m_videoUrl.fileName();
//        message = VrLocale::GetXliffFormattedString(message, fileName.toLatin1().data());
        message = "playback failed";
		BitmapFont & font = vApp->defaultFont();
		font.WordWrapText( message, 1.0f );
        vApp->text.show(message, 4.5f);
        setMenuState( MENU_BROWSER );
		return;
	}

}

VMatrix4f	ArCamera::texmForVideo( const int eye )
{
    if (m_videoUrl.endsWith("_TB.mp4", false)) {
        // top / bottom stereo panorama
		return eye ?
            VMatrix4f( 1, 0, 0, 0,
			0, 0.5, 0, 0.5,
			0, 0, 1, 0,
			0, 0, 0, 1 )
			:
            VMatrix4f( 1, 0, 0, 0,
			0, 0.5, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 );
	}
    if (m_videoUrl.endsWith("_BT.mp4", false)) {
        // top / bottom stereo panorama
		return ( !eye ) ?
            VMatrix4f( 1, 0, 0, 0,
			0, 0.5, 0, 0.5,
			0, 0, 1, 0,
			0, 0, 0, 1 )
			:
            VMatrix4f( 1, 0, 0, 0,
			0, 0.5, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 );
	}
    if (m_videoUrl.endsWith("_LR.mp4", false)) {
        // left / right stereo panorama
		return eye ?
            VMatrix4f( 0.5, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 )
			:
            VMatrix4f( 0.5, 0, 0, 0.5,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 );
	}
    if (m_videoUrl.endsWith("_RL.mp4", false)) {
        // left / right stereo panorama
		return ( !eye ) ?
            VMatrix4f( 0.5, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 )
			:
            VMatrix4f( 0.5, 0, 0, 0.5,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 );
	}

	// default to top / bottom stereo
    if ( m_videoWidth == m_videoHeight )
	{	// top / bottom stereo panorama
		return eye ?
            VMatrix4f( 1, 0, 0, 0,
			0, 0.5, 0, 0.5,
			0, 0, 1, 0,
			0, 0, 0, 1 )
			:
            VMatrix4f( 1, 0, 0, 0,
			0, 0.5, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 );

		// We may want to support swapping top/bottom
	}
    return VMatrix4f();
}

VMatrix4f	ArCamera::texmForBackground( const int eye )
{
    if ( m_backgroundWidth == m_backgroundHeight )
	{	// top / bottom stereo panorama
		return eye ?
            VMatrix4f(
			1, 0, 0, 0,
			0, 0.5, 0, 0.5,
			0, 0, 1, 0,
			0, 0, 0, 1 )
			:
            VMatrix4f(
			1, 0, 0, 0,
			0, 0.5, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 );

		// We may want to support swapping top/bottom
	}
    return VMatrix4f();
}
VMatrix4f ArCamera::drawEyeView( const int eye, const float fovDegrees )
{
    VMatrix4f mvp = m_scene.MvpForEye( eye, fovDegrees );

    if ( ( m_movieTexture != NULL ))
	{
        // draw animated movie panorama

/*
		glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_EXTERNAL_OES, m_movieTexture->textureId );

*/

		glActiveTexture( GL_TEXTURE1 );
        glBindTexture( GL_TEXTURE_2D, m_backgroundTexId );

		glDisable( GL_DEPTH_TEST );
		glDisable( GL_CULL_FACE );

		glUseProgram( bgProgram );
		//glUseProgram(m_panoramaProgram.program);

		GLint tt = glGetUniformLocation(bgProgram,"Texture0");
		glUniform1i(tt,1);
		// Videos have center as initial focal point - need to rotate 90 degrees to start there
        const VMatrix4f view = m_scene.ViewMatrixForEye( 0 ) * VMatrix4f::RotationY( M_PI / 2 );
        const VMatrix4f proj = m_scene.ProjectionMatrixForEye( 0, fovDegrees );

        glUniformMatrix4fv( locTexMatrix, 1, GL_FALSE, texmForVideo( eye ).transposed().cell[ 0 ] );
        glUniformMatrix4fv( locMVP, 1, GL_FALSE, ( proj * view ).transposed().cell[ 0 ] );
        m_globe.drawElements();

		//glBindTexture( GL_TEXTURE_EXTERNAL_OES, 0 );	// don't leave it bound
	}

	return mvp;
}

void ArCamera::stop()
{
    delete m_movieTexture;
    m_movieTexture = NULL;
    vWarn("DELETING MOVIE TEXTURE StopVideo()");
}

void ArCamera::onStart(const VString &url)
{
    m_videoUrl = url;
    if (!m_videoUrl.isEmpty()) {
        setMenuState( MENU_VIDEO_LOADING );
        vInfo("StartVideo(" << m_videoUrl << ")");
	}
}

void ArCamera::setMenuState( const OvrMenuState state )
{
    m_menuState = state;
    switch ( m_menuState )
	{
	case MENU_NONE:
		break;
    case MENU_BROWSER:
        if (!m_videoUrl.isEmpty()) {
            stop();
            m_videoUrl.clear();
		}
		break;
	case MENU_VIDEO_LOADING:
        if ( m_movieTexture != NULL )
		{
            delete m_movieTexture;
            m_movieTexture = NULL;
        }
		break;
	case MENU_VIDEO_READY:
		break;
    case MENU_VIDEO_PLAYING:
		break;
	default:
		vInfo("Oculus360Videos::SetMenuState unknown state:" << static_cast< int >( state ));
		vAssert( false );
		break;
	}
}


VMatrix4f ArCamera::onNewFrame(VFrame vrFrame ) {
	// Disallow player foot movement, but we still want the head model
	// movement for the swipe view.
	VFrame vrFrameWithoutMove = vrFrame;
	vrFrameWithoutMove.input.sticks[0][0] = 0.0f;
	vrFrameWithoutMove.input.sticks[0][1] = 0.0f;
	m_scene.Frame(vApp->viewSettings(), vrFrameWithoutMove, vApp->swapParms().ExternalVelocity);

	// Check for new video frames
	// latch the latest movie frame to the texture.
	if (m_movieTexture) {
		glActiveTexture(GL_TEXTURE0);
		m_movieTexture->Update();
		glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
		m_frameAvailable = false;
	}

	if (m_menuState != MENU_BROWSER && m_menuState != MENU_VIDEO_LOADING) {
		if (vrFrame.input.buttonReleased & (BUTTON_TOUCH | BUTTON_A)) {
			/*if ( IsVideoPlaying() )
            {
                PauseVideo( false );
            }
            else
            {
                ResumeVideo();
            }*/
		}
	}

	// State transitions
	if ((m_menuState == MENU_VIDEO_READY) &&
		(m_movieTexture != NULL)) {
		setMenuState(MENU_VIDEO_PLAYING);
		vApp->recenterYaw(true);
	}

	// We could disable the srgb convert on the FBO. but this is easier
	vApp->vrParms().colorFormat = m_useSrgb ? VColor::COLOR_8888_sRGB : VColor::COLOR_8888;


	// Draw both eyes
//	vApp->drawEyeViewsPostDistorted( m_scene.CenterViewMatrix() );
	VMatrix4f eyeMvp = vApp->drawEyeViewsFirst(m_scene.CenterViewMatrix());

	vApp->drawEyeViewsSecond(eyeMvp);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo1);
	glUseProgram( program );
	GLint x=glGetUniformLocation(program,"cam_tex");
	glUniform1i(x,1);
	x=glGetUniformLocation(program,"bg_tex");
	glUniform1i(x,2);
	glScissor(0, 0, bufferParms.widthScale * bufferParms.resolution,
				  bufferParms.resolution);
	glViewport(0, 0, bufferParms.widthScale * bufferParms.resolution,
				   bufferParms.resolution);
	glClearColor(0, 0, 0, 1);
	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_EXTERNAL_OES, m_movieTexture->textureId );
	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );

	for(int eye = 0;eye<numEyes;++eye)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texids[eye],0);
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			vFatal(
					"render FBO is not complete: " <<
					status); // TODO: fall back to something else
		}
		glActiveTexture( GL_TEXTURE2 );
		glBindTexture( GL_TEXTURE_2D, vApp->swapParms().Images[eye][0].TexId);
		VEglDriver::glBindVertexArrayOES( vao );
		glDrawElements( GL_TRIANGLES, 6, GL_UNSIGNED_SHORT , NULL );
		vApp->swapParms().Images[eye][0].TexId = texids[eye];
		break;
	}
	vApp->swapParms().Images[1][0].TexId = texids[0];
	glActiveTexture(GL_TEXTURE1);
	glBindTexture( GL_TEXTURE_EXTERNAL_OES, 0 );
	glBindFramebuffer(GL_FRAMEBUFFER, backFbo);
	glUseProgram(0);
	vApp->kernel()->doSmooth(&vApp->swapParms());
    return m_scene.CenterViewMatrix();
}

void ArCamera::onResume()
{
	vInfo("Oculus360Videos::OnResume");
    if ( m_videoWasPlayingWhenPaused )
    {
        //pause( false );
	}
}

void ArCamera::onPause()
{
	vInfo("Oculus360Videos::OnPause");
    //m_videoWasPlayingWhenPaused = IsVideoPlaying();
    if ( m_videoWasPlayingWhenPaused )
	{
        //pause( false );
	}
}

NV_NAMESPACE_END
