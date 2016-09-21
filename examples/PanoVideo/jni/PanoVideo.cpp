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
#include <gui/VPixmap.h>

#include "SurfaceTexture.h"
#include "VTexture.h"
#include "BitmapFont.h"
#include "App.h"
#include "PanoVideo.h"
#include "VGui.h"
#include "VProgressBar.h"

NV_NAMESPACE_BEGIN

extern "C" {

void Java_com_vrseen_panovideo_PanoVideo_construct(JNIEnv *jni, jclass, jobject activity)
{
    (new PanoVideo(jni, jni->GetObjectClass(activity), activity))->onCreate(nullptr, nullptr, nullptr);
}

void Java_com_vrseen_panovideo_PanoVideo_onStart(JNIEnv *jni, jclass, jstring jpath)
{
    PanoVideo *video = (PanoVideo *) vApp->appInterface();
    video->onStart(JniUtils::Convert(jni, jpath));
}

void Java_com_vrseen_panovideo_PanoVideo_onFrameAvailable(JNIEnv *, jclass)
{
    PanoVideo *video = (PanoVideo *) vApp->appInterface();
    video->setFrameAvailable(true);
}

jobject Java_com_vrseen_panovideo_PanoVideo_createMovieTexture(JNIEnv *, jclass)
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

void Java_com_vrseen_panovideo_PanoVideo_onVideoSizeChanged(JNIEnv *, jclass, jint width, jint height)
{
    VVariantArray args;
    args << width << height;
    vApp->eventLoop().post("video", std::move(args));
}

void Java_com_vrseen_panovideo_PanoVideo_onCompletion(JNIEnv *, jclass)
{
	vInfo("nativeVideoCompletion");
    vApp->eventLoop().post( "completion" );
}

void Java_com_vrseen_panovideo_PanoVideo_getCurrentPos(JNIEnv *, jclass, jint current, jint movielength)
{

	int curpos = current;
	int length = movielength;
	if (current == movielength)
		return;
	PanoVideo* pv = static_cast<PanoVideo*>(vApp->appInterface());

	if (pv->progressBar == nullptr)
		return;
	float ratio = float (curpos) / float(length);
	pv->progressBar->change(ratio);
}

bool Java_com_vrseen_panovideo_PanoVideo_mediaPause(JNIEnv *, jclass)
{
	PanoVideo* pv = static_cast<PanoVideo*>(vApp->appInterface());

	return pv->pause;
}



} // extern "C"


PanoVideo::PanoVideo(JNIEnv *jni, jclass activityClass, jobject activityObject)
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
	, m_movieFormat(VT_2D)
{
	progressBar = nullptr;
	pause = false;
}

PanoVideo::~PanoVideo()
{
}

void PanoVideo::init(const VString &, const VString &, const VString &)
{
	vInfo("--------------- Oculus360Videos OneTimeInit ---------------");

	vApp->vrParms().colorFormat = VColor::COLOR_8888;
    vApp->vrParms().commonParameterDepth = VEyeItem::CommonParameter::DepthFormat_16;
	vApp->vrParms().multisamples = 2;

	m_panoramaProgram.useMultiview = true;
    m_panoramaProgram.initShader(VGlShader::getPanoVertexShaderSource(),VGlShader::getPanoProgramShaderSource()	);

//    m_fadedPanoramaProgram.initShader(VGlShader::getFadedPanoVertexShaderSource(),VGlShader::getFadedPanoProgramShaderSource());
//
//	// always fall back to valid background
//    if (m_backgroundTexId == 0) {
//        VTexture background(VResource("assets/background.jpg"), VTexture::UseSRGB);
//        m_backgroundTexId = background.id();
//        vAssert(m_backgroundTexId);
//        m_backgroundWidth = background.width();
//        m_backgroundHeight = background.height();
//	}

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

	progressBar = new VProgressBar();
	progressBar->setRect(VRect3f(VVect3f(-3.0, -1.5, -4.0), VVect3f(3.0, -1.2, -4.0)));

	progressBar->setBarImage(VTexture(VResource("assets/rectangle.png")));
	progressBar->setTagImage(VTexture(VResource("assets/square.png")));
	progressBar->setIncImage(VTexture(VResource("assets/rectangle-jindu.png")));

	progressBar->getTag()->setOnFocusListener([=](){PanoVideo::mediaPause();});
	progressBar->getBar()->setOnStareListener([this]() {
												  this->movePos(vApp->gui()->getMVP());
											  });

	VGui * gui  = vApp->gui();

	gui->addItem(progressBar);

	vApp->eventLoop().post("activityInitCompleted");
}

void PanoVideo::movePos(const VMatrix4f &mvp)
{
	VMatrix4f trans =  mvp * progressBar->getBar()->transform();
	VVect3f realpos = trans.transform(progressBar->getBar()->boundingRect().start);
	float ratio = -realpos.x / (trans.transform(progressBar->getBar()->boundingRect().end).x - trans.transform(progressBar->getBar()->boundingRect().start).x);

	JNIEnv * tmpjni = vApp->vrJni();
	jobject jobj = vApp->javaObject();
	jmethodID setpos = tmpjni->GetMethodID(tmpjni->GetObjectClass(jobj), "setPos", "(F)V");

	tmpjni->CallVoidMethod(jobj, setpos, ratio);
}

void PanoVideo::mediaPause()
{
	JNIEnv * tmpjni = vApp->vrJni();
	jobject jobj = vApp->javaObject();
	jmethodID mediapause = tmpjni->GetMethodID(tmpjni->GetObjectClass(jobj), "Pause", "()V");
	jmethodID mediaresume = tmpjni->GetMethodID(tmpjni->GetObjectClass(jobj), "Resume", "()V");
	jmethodID mediaisplaying = tmpjni->GetMethodID(tmpjni->GetObjectClass(jobj), "IsPlaying", "()Z");


	if (tmpjni->CallBooleanMethod(jobj, mediaisplaying))
	{
		tmpjni->CallVoidMethod(jobj, mediapause);
	}
	else
	{
		tmpjni->CallVoidMethod(jobj, mediaresume);
	}

}

void PanoVideo::shutdown()
{
	// This is called by the VR thread, not the java UI thread.
	vInfo("--------------- Oculus360Videos OneTimeShutdown ---------------");
    m_globe.destroy();

//    glDeleteTextures(1, &m_backgroundTexId);

    if (m_movieTexture != NULL) {
        delete m_movieTexture;
        m_movieTexture = NULL;
	}

    m_panoramaProgram.destroy();
//    m_fadedPanoramaProgram.destroy();
}

void PanoVideo::configureVrMode(VKernel* kernel)
{
	// We need very little CPU for pano browsing, but a fair amount of GPU.
	// The CPU clock should ramp up above the minimum when necessary.
	vInfo("ConfigureClocks: Oculus360Videos only needs minimal clocks");
	// All geometry is blended, so save power with no MSAA
	kernel->msaa = 1;
	VEyeItem::settings.useMultiview = true;
}

bool PanoVideo::onKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType )
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

void PanoVideo::command(const VEvent &event )
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

		vInfo(m_videoWidth);
		vInfo(m_videoHeight/2);

        if ( m_menuState != MENU_VIDEO_PLAYING ) // If video is already being played dont change the state to video ready
		{
            setMenuState( MENU_VIDEO_READY );
		}

		if (m_videoWidth/2  > m_videoHeight)
		{
			m_movieFormat = VT_LEFT_RIGHT_3D;
		}
		else if (m_videoWidth/2  < m_videoHeight)
		{
			m_movieFormat = VT_TOP_BOTTOM_3D;
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

VMatrix4f PanoVideo::drawEyeView( const int eye, const float fovDegrees )
{
	modelViewProMatrix[eye] = m_scene.MvpForEye( eye, fovDegrees );

    if ( ( m_menuState == MENU_VIDEO_PLAYING ) && ( m_movieTexture != NULL ) )
	{
		vApp->swapParms().WarpProgram = WP_CHROMATIC;
		vApp->swapParms().Images[eye][1].TexId = 0;

		// draw animated movie panorama
		glActiveTexture( GL_TEXTURE0 );
        glBindTexture( GL_TEXTURE_EXTERNAL_OES, m_movieTexture->textureId );

//		glActiveTexture( GL_TEXTURE1 );
//        glBindTexture( GL_TEXTURE_2D, m_backgroundTexId );

		glDisable( GL_DEPTH_TEST );
		glDisable( GL_CULL_FACE );

        VGlShader & prog = m_panoramaProgram;

		glUseProgram( prog.program );
		glUniform4f( prog.uniformColor, 1.0f, 1.0f, 1.0f, 1.0f );

		if(VEyeItem::settings.useMultiview)
		{
			// Videos have center as initial focal point - need to rotate 90 degrees to start there
			modelViewProMatrix[1] = m_scene.MvpForEye( 1, fovDegrees );
			VMatrix4f mvp[2];
			mvp[0] = modelViewProMatrix[0] * VMatrix4f::RotationY( M_PI / 2 );
			mvp[1] = modelViewProMatrix[1] * VMatrix4f::RotationY( M_PI / 2 );

			glUniformMatrix4fv(prog.uniformModelViewProMatrix, 2, GL_TRUE, mvp[0].data());

			VMatrix4f texMatrix[2];
			texMatrix[0] = vApp->appInterface()->getTexMatrix(0,m_movieFormat).transposed();
			texMatrix[1] = vApp->appInterface()->getTexMatrix(1,m_movieFormat).transposed();
			glUniformMatrix4fv(prog.uniformTexMatrix, 2, GL_FALSE, texMatrix[0].data());
		}
		else
		{
			// Videos have center as initial focal point - need to rotate 90 degrees to start there
			const VMatrix4f mvp = modelViewProMatrix[eye] * VMatrix4f::RotationY( M_PI / 2 );

			glUniformMatrix4fv( prog.uniformModelViewProMatrix, 1, GL_FALSE, mvp.transposed().cell[ 0 ] );
			glUniformMatrix4fv( prog.uniformTexMatrix, 1, GL_FALSE, /* not transposed */
								vApp->appInterface()->getTexMatrix(eye,m_movieFormat).transposed().data());
		}

        m_globe.drawElements();

		glBindTexture( GL_TEXTURE_EXTERNAL_OES, 0 );	// don't leave it bound
	}

	return modelViewProMatrix[eye];
}

void PanoVideo::stop()
{
    delete m_movieTexture;
    m_movieTexture = NULL;
    vWarn("DELETING MOVIE TEXTURE StopVideo()");
}

void PanoVideo::onStart(const VString &url)
{
    m_videoUrl = url;
    if (!m_videoUrl.isEmpty()) {
        setMenuState( MENU_VIDEO_LOADING );
        vInfo("StartVideo(" << m_videoUrl << ")");
	}
}

void PanoVideo::setMenuState( const OvrMenuState state )
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

VMatrix4f PanoVideo::onNewFrame(VFrame vrFrame )
{
	// Disallow player foot movement, but we still want the head model
	// movement for the swipe view.
	VFrame vrFrameWithoutMove = vrFrame;
	vrFrameWithoutMove.input.sticks[ 0 ][ 0 ] = 0.0f;
	vrFrameWithoutMove.input.sticks[ 0 ][ 1 ] = 0.0f;
    m_scene.Frame( vApp->viewSettings(), vrFrameWithoutMove,vApp->swapParms().ExternalVelocity );

	// Check for new video frames
	// latch the latest movie frame to the texture.
    if ( m_movieTexture && m_videoWidth ) {
		glActiveTexture( GL_TEXTURE0 );
        m_movieTexture->Update();
		glBindTexture( GL_TEXTURE_EXTERNAL_OES, 0 );
        m_frameAvailable = false;
	}

    if ( m_menuState != MENU_BROWSER && m_menuState != MENU_VIDEO_LOADING )
	{
		if ( vrFrame.input.buttonReleased & ( BUTTON_TOUCH | BUTTON_A ) )
        {
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
    if ( ( m_menuState == MENU_VIDEO_READY ) &&
        ( m_movieTexture != NULL ) )
	{
        setMenuState( MENU_VIDEO_PLAYING );
		vApp->recenterYaw( true );
    }

	// We could disable the srgb convert on the FBO. but this is easier
    vApp->vrParms().colorFormat = m_useSrgb ? VColor::COLOR_8888_sRGB : VColor::COLOR_8888;

    return m_scene.CenterViewMatrix();
}

void PanoVideo::onResume()
{
	vInfo("Oculus360Videos::OnResume");
    if ( m_videoWasPlayingWhenPaused )
    {
        //pause( false );
	}
}

void PanoVideo::onPause()
{
	vInfo("Oculus360Videos::OnPause");
    //m_videoWasPlayingWhenPaused = IsVideoPlaying();
    if ( m_videoWasPlayingWhenPaused )
	{
        //pause( false );
	}
}

VMatrix4f PanoVideo::getModelViewProMatrix(int eye) const
{
	return modelViewProMatrix[eye];
}


NV_NAMESPACE_END
