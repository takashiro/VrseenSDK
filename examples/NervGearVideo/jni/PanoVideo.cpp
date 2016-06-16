#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <jni.h>
#include <android/keycodes.h>

#include <VAlgorithm.h>
#include <VPath.h>
#include <VZipFile.h>
#include <VFile.h>
#include <VResource.h>

#include <fstream>

#include <android/JniUtils.h>

#include "VArray.h"
#include "VString.h"
#include "VEglDriver.h"
#include "SurfaceTexture.h"

#include "VTexture.h"
#include "BitmapFont.h"
#include "GazeCursor.h"
#include "App.h"
#include "SwipeView.h"
#include "PanoVideo.h"

#include <VEyeItem.h>

#include "gui/Fader.h"
#include "VDir.h"
#include "VrLocale.h"
#include "VStandardPath.h"
#include "VTimer.h"
#include "VColor.h"

static const float	FadeOutTime = 0.25f;
static const float	FadeOverTime = 1.0f;

namespace NervGear
{

extern "C" {

static jclass	GlobalActivityClass;
void Java_com_vrseen_nervgear_video_MainActivity_nativeSetAppInterface( JNIEnv *jni, jclass clazz, jobject activity,
		jstring fromPackageName, jstring commandString, jstring uriString )
{
	// This is called by the java UI thread.

	GlobalActivityClass = (jclass)jni->NewGlobalRef( clazz );

	vInfo("nativeSetAppInterface");
    (new PanoVideo(jni, clazz, activity))->onCreate(fromPackageName, commandString, uriString );
}

void Java_com_vrseen_nervgear_video_PanoVideo_onFrameAvailable(JNIEnv *, jclass)
{
    PanoVideo * panoVids = ( PanoVideo * ) vApp->appInterface();
	panoVids->SetFrameAvailable( true );
}

jobject Java_com_vrseen_nervgear_video_PanoVideo_createMovieTexture(JNIEnv *, jclass)
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
    return NULL;
}

void Java_com_vrseen_nervgear_video_PanoVideo_onVideoSizeChanged(JNIEnv *, jclass, jint width, jint height)
{
    VVariantArray args;
    args << width << height;
    vApp->eventLoop().post("video", std::move(args));
}

void Java_com_vrseen_nervgear_video_PanoVideo_onCompletion(JNIEnv *, jclass)
{
	vInfo("nativeVideoCompletion");
    vApp->eventLoop().post( "completion" );
}

} // extern "C"



PanoVideo::PanoVideo(JNIEnv *jni, jclass activityClass, jobject activityObject)
    : VMainActivity(jni, activityClass, activityObject)
    , MainActivityClass( GlobalActivityClass )
	, VideoWasPlayingWhenPaused( false )
	, BackgroundTexId( 0 )
    , MenuState( MENU_NONE )
	, Fader( 1.0f )
	, FadeOutRate( 1.0f / 0.5f )
	, VideoMenuVisibleTime( 5.0f )
	, CurrentFadeRate( FadeOutRate )
	, CurrentFadeLevel( 1.0f )
	, VideoMenuTimeLeft( 0.0f )
	, UseSrgb( false )
	, MovieTexture( NULL )
	, CurrentVideoWidth( 0 )
	, CurrentVideoHeight( 480 )
	, BackgroundWidth( 0 )
	, BackgroundHeight( 0 )
	, FrameAvailable( false )
{
}

PanoVideo::~PanoVideo()
{
}

void PanoVideo::init(const VString &fromPackage, const VString &launchIntentJSON, const VString &launchIntentURI)
{
	vInfo("--------------- Oculus360Videos OneTimeInit ---------------");

	vApp->vrParms().colorFormat = VColor::COLOR_8888;
    vApp->vrParms().commonParameterDepth = VEyeItem::CommonParameter::DepthFormat_16;
	vApp->vrParms().multisamples = 2;

    PanoramaProgram.initShader(VGlShader::getPanoVertexShaderSource(),VGlShader::getPanoProgramShaderSource()	);

    FadedPanoramaProgram.initShader(VGlShader::getFadedPanoVertexShaderSource(),VGlShader::getFadedPanoProgramShaderSource());
    SingleColorTextureProgram.initShader(VGlShader::getSingleTextureVertexShaderSource(),VGlShader::getUniformSingleTextureProgramShaderSource());

	// always fall back to valid background
    if (BackgroundTexId == 0) {
        VTexture background(VResource("assets/background.jpg"), VTexture::UseSRGB);
        BackgroundTexId = background.id();
        vAssert(BackgroundTexId);
        BackgroundWidth = background.width();
        BackgroundHeight = background.height();
	}

	vInfo("Creating Globe");
    Globe.createSphere();

	// Stay exactly at the origin, so the panorama globe is equidistant
	// Don't clear the head model neck length, or swipe view panels feel wrong.
	VViewSettings viewParms = vApp->viewSettings();
	viewParms.eyeHeight = 0.0f;
	vApp->setViewSettings( viewParms );

	// Optimize for 16 bit depth in a modest theater size
	Scene.Znear = 0.1f;
	Scene.Zfar = 200.0f;
	MaterialParms materialParms;
	materialParms.UseSrgbTextureFormats = ( vApp->vrParms().colorFormat == VColor::COLOR_8888_sRGB );

    SetMenuState(MENU_BROWSER);
    OnVideoActivated(launchIntentURI);
}

void PanoVideo::shutdown()
{
	// This is called by the VR thread, not the java UI thread.
	vInfo("--------------- Oculus360Videos OneTimeShutdown ---------------");
	Globe.destroy();

    glDeleteTextures(1, &BackgroundTexId);

    if (MovieTexture != NULL) {
		delete MovieTexture;
		MovieTexture = NULL;
	}

    PanoramaProgram.destroy();
    FadedPanoramaProgram.destroy();
    SingleColorTextureProgram.destroy();
}

void PanoVideo::configureVrMode(VKernel* kernel)
{
	// We need very little CPU for pano browsing, but a fair amount of GPU.
	// The CPU clock should ramp up above the minimum when necessary.
	vInfo("ConfigureClocks: Oculus360Videos only needs minimal clocks");
	// All geometry is blended, so save power with no MSAA
	kernel->msaa = 1;
}

bool PanoVideo::onKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType )
{
	if ( ( ( keyCode == AKEYCODE_BACK ) && ( eventType == KeyState::KEY_EVENT_SHORT_PRESS ) ) ||
		( ( keyCode == KEYCODE_B ) && ( eventType == KeyState::KEY_EVENT_UP ) ) )
	{
        if ( MenuState == MENU_VIDEO_LOADING )
        {
            return true;
        }

		// if no video is playing (either paused or stopped), let VrLib handle the back key
    }
	else if ( keyCode == AKEYCODE_P && eventType == KeyState::KEY_EVENT_DOWN )
	{
		PauseVideo( true );
	}

	return false;
}

void PanoVideo::command(const VEvent &event )
{
	// Always include the space in MatchesHead to prevent problems
	// with commands with matching prefixes.

    if (event.name == "newVideo") {
		delete MovieTexture;
		MovieTexture = new SurfaceTexture( vApp->vrJni() );
		vInfo("RC_NEW_VIDEO texId" << MovieTexture->textureId);

        VEventLoop *receiver = static_cast<VEventLoop *>(event.data.toPointer());
        receiver->post("surfaceTexture", MovieTexture->javaObject);

		// don't draw the screen until we have the new size
		CurrentVideoWidth = 0;
		return;

    } else if (event.name == "completion") {// video complete, return to menu
		SetMenuState( MENU_BROWSER );
		return;

    } else if (event.name == "video") {
        CurrentVideoWidth = event.data.at(0).toInt();
        CurrentVideoHeight = event.data.at(1).toInt();

		if ( MenuState != MENU_VIDEO_PLAYING ) // If video is already being played dont change the state to video ready
		{
			SetMenuState( MENU_VIDEO_READY );
		}

		return;
    } else if (event.name == "resume") {
		OnResume();
		return;	// allow VrLib to handle it, too
    } else if (event.name == "pause") {
		OnPause();
		return;	// allow VrLib to handle it, too
    } else if (event.name == "startError") {
		// FIXME: this needs to do some parameter magic to fix xliff tags
		VString message;
		VrLocale::GetString( vApp->vrJni(), vApp->javaObject(), "@string/playback_failed", "@string/playback_failed", message );
        VString fileName = m_videoUrl.fileName();
        message = VrLocale::GetXliffFormattedString(message, fileName.toLatin1().data());
		BitmapFont & font = vApp->defaultFont();
		font.WordWrapText( message, 1.0f );
        vApp->text.show(message, 4.5f);
		SetMenuState( MENU_BROWSER );
		return;
	}

}

VR4Matrixf	PanoVideo::TexmForVideo( const int eye )
{
    if (m_videoUrl.endsWith("_TB.mp4", false)) {
        // top / bottom stereo panorama
		return eye ?
            VR4Matrixf( 1, 0, 0, 0,
			0, 0.5, 0, 0.5,
			0, 0, 1, 0,
			0, 0, 0, 1 )
			:
            VR4Matrixf( 1, 0, 0, 0,
			0, 0.5, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 );
	}
    if (m_videoUrl.endsWith("_BT.mp4", false)) {
        // top / bottom stereo panorama
		return ( !eye ) ?
            VR4Matrixf( 1, 0, 0, 0,
			0, 0.5, 0, 0.5,
			0, 0, 1, 0,
			0, 0, 0, 1 )
			:
            VR4Matrixf( 1, 0, 0, 0,
			0, 0.5, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 );
	}
    if (m_videoUrl.endsWith("_LR.mp4", false)) {
        // left / right stereo panorama
		return eye ?
            VR4Matrixf( 0.5, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 )
			:
            VR4Matrixf( 0.5, 0, 0, 0.5,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 );
	}
    if (m_videoUrl.endsWith("_RL.mp4", false)) {
        // left / right stereo panorama
		return ( !eye ) ?
            VR4Matrixf( 0.5, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 )
			:
            VR4Matrixf( 0.5, 0, 0, 0.5,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 );
	}

	// default to top / bottom stereo
	if ( CurrentVideoWidth == CurrentVideoHeight )
	{	// top / bottom stereo panorama
		return eye ?
            VR4Matrixf( 1, 0, 0, 0,
			0, 0.5, 0, 0.5,
			0, 0, 1, 0,
			0, 0, 0, 1 )
			:
            VR4Matrixf( 1, 0, 0, 0,
			0, 0.5, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 );

		// We may want to support swapping top/bottom
	}
    return VR4Matrixf::Identity();
}

VR4Matrixf	PanoVideo::TexmForBackground( const int eye )
{
	if ( BackgroundWidth == BackgroundHeight )
	{	// top / bottom stereo panorama
		return eye ?
            VR4Matrixf(
			1, 0, 0, 0,
			0, 0.5, 0, 0.5,
			0, 0, 1, 0,
			0, 0, 0, 1 )
			:
            VR4Matrixf(
			1, 0, 0, 0,
			0, 0.5, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1 );

		// We may want to support swapping top/bottom
	}
    return VR4Matrixf::Identity();
}

VR4Matrixf PanoVideo::drawEyeView( const int eye, const float fovDegrees )
{
    VR4Matrixf mvp = Scene.MvpForEye( eye, fovDegrees );

    if ( ( MenuState == MENU_VIDEO_PLAYING ) && ( MovieTexture != NULL ) )
	{
		// draw animated movie panorama
		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_EXTERNAL_OES, MovieTexture->textureId );

		glActiveTexture( GL_TEXTURE1 );
		glBindTexture( GL_TEXTURE_2D, BackgroundTexId );

		glDisable( GL_DEPTH_TEST );
		glDisable( GL_CULL_FACE );

		VGlShader & prog = ( BackgroundWidth == BackgroundHeight ) ? FadedPanoramaProgram : PanoramaProgram;

		glUseProgram( prog.program );
		glUniform4f( prog.uniformColor, 1.0f, 1.0f, 1.0f, 1.0f );

		// Videos have center as initial focal point - need to rotate 90 degrees to start there
        const VR4Matrixf view = Scene.ViewMatrixForEye( 0 ) * VR4Matrixf::RotationY( M_PI / 2 );
        const VR4Matrixf proj = Scene.ProjectionMatrixForEye( 0, fovDegrees );

        glUniformMatrix4fv( prog.uniformTexMatrix, 1, GL_FALSE, TexmForVideo( eye ).Transposed().M[ 0 ] );
        glUniformMatrix4fv( prog.uniformModelViewProMatrix, 1, GL_FALSE, ( proj * view ).Transposed().M[ 0 ] );
		Globe.drawElements();

		glBindTexture( GL_TEXTURE_EXTERNAL_OES, 0 );	// don't leave it bound
	}

	return mvp;
}

bool PanoVideo::IsVideoPlaying() const
{
	jmethodID methodId = vApp->vrJni()->GetMethodID( MainActivityClass, "isPlaying", "()Z" );
	if ( !methodId )
	{
		vInfo("Couldn't find isPlaying methodID");
		return false;
	}

	bool isPlaying = vApp->vrJni()->CallBooleanMethod( vApp->javaObject(), methodId );
	return isPlaying;
}

void PanoVideo::PauseVideo( bool const force )
{
	vInfo("PauseVideo()");

	jmethodID methodId = vApp->vrJni()->GetMethodID( MainActivityClass,
		"pauseMovie", "()V" );
	if ( !methodId )
	{
		vInfo("Couldn't find pauseMovie methodID");
		return;
	}

	vApp->vrJni()->CallVoidMethod( vApp->javaObject(), methodId );
}

void PanoVideo::StopVideo()
{
	vInfo("StopVideo()");

	jmethodID methodId = vApp->vrJni()->GetMethodID( MainActivityClass,
		"stopMovie", "()V" );
	if ( !methodId )
	{
		vInfo("Couldn't find stopMovie methodID");
		return;
	}

	vApp->vrJni()->CallVoidMethod( vApp->javaObject(), methodId );

	delete MovieTexture;
	MovieTexture = NULL;
}

void PanoVideo::ResumeVideo()
{
	vInfo("ResumeVideo()");

	jmethodID methodId = vApp->vrJni()->GetMethodID( MainActivityClass,
		"resumeMovie", "()V" );
	if ( !methodId )
	{
		vInfo("Couldn't find resumeMovie methodID");
		return;
	}

	vApp->vrJni()->CallVoidMethod( vApp->javaObject(), methodId );
}

void PanoVideo::StartVideo( const double nowTime )
{
    if (!m_videoUrl.isEmpty()) {
		SetMenuState( MENU_VIDEO_LOADING );
        vInfo("StartVideo(" << m_videoUrl << ")");
		vApp->playSound( "sv_select" );

		jmethodID startMovieMethodId = vApp->vrJni()->GetMethodID( MainActivityClass,
			"startMovieFromNative", "(Ljava/lang/String;)V" );

		if ( !startMovieMethodId )
		{
			vInfo("Couldn't find startMovie methodID");
			return;
		}

        vInfo("moviePath = '" << m_videoUrl << "'");
        jstring jstr = JniUtils::Convert(vApp->vrJni(), m_videoUrl);
		vApp->vrJni()->CallVoidMethod( vApp->javaObject(), startMovieMethodId, jstr );
		vApp->vrJni()->DeleteLocalRef( jstr );

		vInfo("StartVideo done");
	}
}

void PanoVideo::SeekTo( const int seekPos )
{
    if (!m_videoUrl.isEmpty()) {
		jmethodID seekToMethodId = vApp->vrJni()->GetMethodID( MainActivityClass,
			"seekToFromNative", "(I)V" );

		if ( !seekToMethodId )
		{
			vInfo("Couldn't find seekToMethodId methodID");
			return;
		}

		vApp->vrJni()->CallVoidMethod( vApp->javaObject(), seekToMethodId, seekPos );

		vInfo("SeekTo" << seekPos << "done");
	}
}

void PanoVideo::SetMenuState( const OvrMenuState state )
{
    MenuState = state;
	switch ( MenuState )
	{
	case MENU_NONE:
		break;
	case MENU_BROWSER:
		Fader.forceFinish();
        Fader.reset();
        if (!m_videoUrl.isEmpty()) {
			StopVideo();
            m_videoUrl.clear();
		}
		break;
	case MENU_VIDEO_LOADING:
		if ( MovieTexture != NULL )
		{
			delete MovieTexture;
			MovieTexture = NULL;
        }
		Fader.startFadeOut();
		break;
	case MENU_VIDEO_READY:
		break;
	case MENU_VIDEO_PLAYING:
		Fader.reset();
		VideoMenuTimeLeft = VideoMenuVisibleTime;
		break;
	default:
		vInfo("Oculus360Videos::SetMenuState unknown state:" << static_cast< int >( state ));
		vAssert( false );
		break;
	}
}

void PanoVideo::OnVideoActivated(const VString &url)
{
    m_videoUrl = url;
    StartVideo(VTimer::Seconds());
}

VR4Matrixf PanoVideo::onNewFrame( const VFrame vrFrame )
{
	// Disallow player foot movement, but we still want the head model
	// movement for the swipe view.
	VFrame vrFrameWithoutMove = vrFrame;
	vrFrameWithoutMove.input.sticks[ 0 ][ 0 ] = 0.0f;
	vrFrameWithoutMove.input.sticks[ 0 ][ 1 ] = 0.0f;
    Scene.Frame( vApp->viewSettings(), vrFrameWithoutMove, vApp->kernel()->m_externalVelocity );

	// Check for new video frames
	// latch the latest movie frame to the texture.
	if ( MovieTexture && CurrentVideoWidth ) {
		glActiveTexture( GL_TEXTURE0 );
		MovieTexture->Update();
		glBindTexture( GL_TEXTURE_EXTERNAL_OES, 0 );
		FrameAvailable = false;
	}

	if ( MenuState != MENU_BROWSER && MenuState != MENU_VIDEO_LOADING )
	{
		if ( vrFrame.input.buttonReleased & ( BUTTON_TOUCH | BUTTON_A ) )
		{
			vApp->playSound( "sv_release_active" );
			if ( IsVideoPlaying() )
            {
				PauseVideo( false );
			}
			else
            {
				ResumeVideo();
			}
		}
	}

	// State transitions
	if ( Fader.fadeState() != Fader::FADE_NONE )
	{
		Fader.update( CurrentFadeRate, vrFrame.deltaSeconds );
	}
	else if ( ( MenuState == MENU_VIDEO_READY ) &&
		( Fader.fadeAlpha() == 0.0f ) &&
		( MovieTexture != NULL ) )
	{
		SetMenuState( MENU_VIDEO_PLAYING );
		vApp->recenterYaw( true );
	}
	CurrentFadeLevel = Fader.finalAlpha();

	// We could disable the srgb convert on the FBO. but this is easier
	vApp->vrParms().colorFormat = UseSrgb ? VColor::COLOR_8888_sRGB : VColor::COLOR_8888;

	// Draw both eyes
	vApp->drawEyeViewsPostDistorted( Scene.CenterViewMatrix() );

	return Scene.CenterViewMatrix();
}

void PanoVideo::OnResume()
{
	vInfo("Oculus360Videos::OnResume");
	if ( VideoWasPlayingWhenPaused )
    {
		PauseVideo( false );
	}
}

void PanoVideo::OnPause()
{
	vInfo("Oculus360Videos::OnPause");
	VideoWasPlayingWhenPaused = IsVideoPlaying();
	if ( VideoWasPlayingWhenPaused )
	{
		PauseVideo( false );
	}
}

}
