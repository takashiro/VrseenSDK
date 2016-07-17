#include "CinemaApp.h"
#include "Native.h"
#include "VColor.h"
#include "core/VTimer.h"
//=======================================================================================

NV_NAMESPACE_BEGIN

namespace OculusCinema {

CinemaApp::CinemaApp(JNIEnv *jni, jclass activityClass, jobject activityObject)
    : VMainActivity(jni, activityClass, activityObject)
    ,
    startTime( 0 ),
	sceneMgr( *this ),
	shaderMgr( *this ),
	inLobby( true ),
	allowDebugControls( false ),
	m_vrFrame(),
	m_frameCount( 0 ),
	m_currentMovie( NULL ),
	m_playList(),
	m_shouldResumeMovie( false ),
	m_movieFinishedPlaying( false )
{
}

/*
 * OneTimeInit
 *
 */
void CinemaApp::init(const VString &fromPackage, const VString &launchIntentJSON, const VString &launchIntentURI)
{
	vInfo("--------------- CinemaApp OneTimeInit ---------------");

    startTime = VTimer::Seconds();

    vApp->vrParms().colorFormat = VColor::COLOR_8888;
	//app->GetVrParms().depthFormat = DEPTH_16;
    vApp->vrParms().multisamples = 2;

    Native::OneTimeInit( vApp, javaClass() );
    shaderMgr.OneTimeInit( launchIntentURI );

    if(!launchIntentURI.isEmpty())
    {
        MovieDef* movieDef = new MovieDef;
        movieDef->Filename = launchIntentURI;
        setMovie(movieDef);
        startMoviePlayback();
    }

    vInfo("CinemaApp::OneTimeInit:" << (VTimer::Seconds() - startTime) << "seconds");
}

void CinemaApp::shutdown()
{
	vInfo("--------------- CinemaApp OneTimeShutdown ---------------");

	Native::OneTimeShutdown();
	shaderMgr.OneTimeShutdown();
	sceneMgr.OneTimeShutdown();
}

void CinemaApp::setMovie( const MovieDef *movie )
{
    vInfo("SetMovie(" << movie->Filename << ")");
	m_currentMovie = movie;
	m_movieFinishedPlaying = false;
}

void CinemaApp::startMoviePlayback()
{
	if ( m_currentMovie != NULL )
	{
		m_movieFinishedPlaying = false;
        Native::StartMovie(m_currentMovie->Filename, m_shouldResumeMovie, m_currentMovie->IsEncrypted, false );
		m_shouldResumeMovie = false;
	}
}

bool CinemaApp::isMovieFinished() const
{
	return m_movieFinishedPlaying;
}

/*
 * DrawEyeView
 */
VR4Matrixf CinemaApp::drawEyeView( const int eye, const float fovDegrees ) {
    return sceneMgr.DrawEyeView( eye, fovDegrees );
}

void CinemaApp::configureVrMode(VKernel* kernel)
{
	// We need very little CPU for movie playing, but a fair amount of GPU.
	// The CPU clock should ramp up above the minimum when necessary.
	vInfo("ConfigureClocks: Cinema only needs minimal clocks");
	// Always use 2x MSAA for now
    kernel->msaa= 2;
}

/*
 * Command
 *
 * Actions that need to be performed on the render thread.
 */
void CinemaApp::command(const VEvent &event )
{
    if (sceneMgr.Command(event)) {
		return;
	}
}

/*
 * Frame()
 *
 * App override
 */
VR4Matrixf CinemaApp::onNewFrame( const VFrame vrFrame )
{
	m_frameCount++;
	this->m_vrFrame = vrFrame;

    return sceneMgr.Frame( vrFrame );
}

} // namespace OculusCinema
