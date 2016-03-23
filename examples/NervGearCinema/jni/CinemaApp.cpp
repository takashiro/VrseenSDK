#include "CinemaApp.h"
#include "Native.h"
#include "CinemaStrings.h"


//=======================================================================================

namespace OculusCinema {

CinemaApp::CinemaApp(JNIEnv *jni, jclass activityClass, jobject activityObject)
    : VMainActivity(jni, activityClass, activityObject)
    ,
    startTime( 0 ),
	sceneMgr( *this ),
	shaderMgr( *this ),
	modelMgr( *this ),
	movieMgr( *this ),
	inLobby( true ),
	allowDebugControls( false ),
	m_viewMgr(),
	m_moviePlayer( *this ),
	m_movieSelectionMenu( *this ),
	m_theaterSelectionMenu( *this ),
	m_resumeMovieMenu( *this ),
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
	LOG( "--------------- CinemaApp OneTimeInit ---------------");

	startTime = ovr_GetTimeInSeconds();

    vApp->vrParms().colorFormat = COLOR_8888;
	//app->GetVrParms().depthFormat = DEPTH_16;
    vApp->vrParms().multisamples = 2;

    Native::OneTimeInit( vApp, javaClass() );
	CinemaStrings::OneTimeInit( *this );
    shaderMgr.OneTimeInit( launchIntentURI );
	modelMgr.OneTimeInit( launchIntentURI );
	sceneMgr.OneTimeInit( launchIntentURI );
	movieMgr.OneTimeInit( launchIntentURI );
	m_moviePlayer.OneTimeInit( launchIntentURI );
	m_movieSelectionMenu.OneTimeInit( launchIntentURI );
	m_theaterSelectionMenu.OneTimeInit( launchIntentURI );
	m_resumeMovieMenu.OneTimeInit( launchIntentURI );

	m_viewMgr.addView( &m_moviePlayer );
	m_viewMgr.addView( &m_movieSelectionMenu );
	m_viewMgr.addView( &m_theaterSelectionMenu );

    setMovieSelection( true );

	LOG( "CinemaApp::OneTimeInit: %3.1f seconds", ovr_GetTimeInSeconds() - startTime );
}

void CinemaApp::shutdown()
{
	LOG( "--------------- CinemaApp OneTimeShutdown ---------------");

	Native::OneTimeShutdown();
	shaderMgr.OneTimeShutdown();
	modelMgr.OneTimeShutdown();
	sceneMgr.OneTimeShutdown();
	movieMgr.OneTimeShutdown();
	m_moviePlayer.OneTimeShutdown();
	m_movieSelectionMenu.OneTimeShutdown();
	m_theaterSelectionMenu.OneTimeShutdown();
	m_resumeMovieMenu.OneTimeShutdown();
}

VString CinemaApp::retailDir(const VString &dir) const
{
    return sdcardDir("RetailMedia") + '/' + dir;
}

VString CinemaApp::externalRetailDir(const VString &dir) const
{
    return externalSDCardDir("RetailMedia") + '/' + dir;
}

VString CinemaApp::sdcardDir(const VString &dir ) const
{
    return "/sdcard/" + dir;
}

VString CinemaApp::externalSDCardDir(const VString &dir) const
{
    return "/storage/extSdCard/" + dir;
}

VString CinemaApp::externalCacheDir(const VString &dir) const
{
    return Native::GetExternalCacheDirectory(vApp) + '/' + dir;
}

bool CinemaApp::isExternalSDCardDir(const VString &dir) const
{
    VString sdcardDir = externalSDCardDir("");
    return dir.startsWith(sdcardDir);
}

bool CinemaApp::fileExists(const VString &filename) const
{
    FILE *f = fopen( filename.toCString(), "r" );
	if ( !f )
	{
		return false;
	}
	else
	{
		fclose( f );
		return true;
	}
}

void CinemaApp::setPlaylist( const VArray<const MovieDef *> &playList, const int nextMovie )
{
	m_playList = playList;

    assert( nextMovie < m_playList.length() );
	setMovie( m_playList[ nextMovie ] );
}

void CinemaApp::setMovie( const MovieDef *movie )
{
    LOG( "SetMovie( %s )", movie->Filename.toCString() );
	m_currentMovie = movie;
	m_movieFinishedPlaying = false;
}

void CinemaApp::movieLoaded( const int width, const int height, const int duration )
{
	m_moviePlayer.MovieLoaded( width, height, duration );
}

const MovieDef *CinemaApp::nextMovie() const
{
	const MovieDef *next = NULL;
    if ( m_playList.length() != 0 )
	{
        for( int i = 0; i < m_playList.length() - 1; i++ )
		{
			if ( m_playList[ i ] == m_currentMovie )
			{
				next = m_playList[ i + 1 ];
				break;
			}
		}

		if ( next == NULL )
		{
			next = m_playList[ 0 ];
		}
	}

	return next;
}

const MovieDef *CinemaApp::previousMovie() const
{
	const MovieDef *previous = NULL;
    if ( m_playList.length() != 0 )
	{
        for( int i = 0; i < m_playList.length(); i++ )
		{
			if ( m_playList[ i ] == m_currentMovie )
			{
				break;
			}
			previous = m_playList[ i ];
		}

		if ( previous == NULL )
		{
            previous = m_playList[ m_playList.length() - 1 ];
		}
	}

	return previous;
}

void CinemaApp::startMoviePlayback()
{
	if ( m_currentMovie != NULL )
	{
		m_movieFinishedPlaying = false;
        Native::StartMovie( vApp, m_currentMovie->Filename.toCString(), m_shouldResumeMovie, m_currentMovie->IsEncrypted, false );
		m_shouldResumeMovie = false;
	}
}

void CinemaApp::resumeMovieFromSavedLocation()
{
	LOG( "ResumeMovie");
	inLobby = false;
	m_shouldResumeMovie = true;
	m_viewMgr.openView( m_moviePlayer );
}

void CinemaApp::playMovieFromBeginning()
{
	LOG( "PlayMovieFromBeginning");
	inLobby = false;
	m_shouldResumeMovie = false;
	m_viewMgr.openView( m_moviePlayer );
}

void CinemaApp::resumeOrRestartMovie()
{
	LOG( "StartMovie");
    if ( Native::CheckForMovieResume( vApp, m_currentMovie->Filename.toCString() ) )
	{
		LOG( "Open ResumeMovieMenu");
		m_viewMgr.openView( m_resumeMovieMenu );
	}
	else
	{
        playMovieFromBeginning();
	}
}

void CinemaApp::movieFinished()
{
	inLobby = false;
	m_movieFinishedPlaying = true;
    m_movieSelectionMenu.SetMovieList( m_playList, nextMovie() );
	m_viewMgr.openView( m_movieSelectionMenu );
}

void CinemaApp::unableToPlayMovie()
{
	inLobby = false;
    m_movieSelectionMenu.SetError( CinemaStrings::Error_UnableToPlayMovie.toCString(), false, true );
	m_viewMgr.openView( m_movieSelectionMenu );
}

void CinemaApp::theaterSelection()
{
	m_viewMgr.openView( m_theaterSelectionMenu );
}

void CinemaApp::setMovieSelection( bool inLobby )
{
	this->inLobby = inLobby;
	m_viewMgr.openView( m_movieSelectionMenu );
}

bool CinemaApp::allowTheaterSelection() const
{
	if ( m_currentMovie != NULL )
	{
		return m_currentMovie->AllowTheaterSelection;
	}

	return true;
}

bool CinemaApp::isMovieFinished() const
{
	return m_movieFinishedPlaying;
}


const SceneDef & CinemaApp::currentTheater() const
{
	return modelMgr.GetTheater( m_theaterSelectionMenu.GetSelectedTheater() );
}

/*
 * DrawEyeView
 */
VR4Matrixf CinemaApp::drawEyeView( const int eye, const float fovDegrees ) {
	return m_viewMgr.drawEyeView( eye, fovDegrees );
}

void CinemaApp::configureVrMode(ovrModeParms &modeParms)
{
	// We need very little CPU for movie playing, but a fair amount of GPU.
	// The CPU clock should ramp up above the minimum when necessary.
	LOG( "ConfigureClocks: Cinema only needs minimal clocks" );

	modeParms.CpuLevel = 1;
	modeParms.GpuLevel = 2;

	// when the app is throttled, go to the platform UI and display a
	// dismissable warning. On return to the app, force 30hz timewarp.
	modeParms.AllowPowerSave = true;

	// Always use 2x MSAA for now
    vApp->vrParms().multisamples = 2;
}

/*
 * Command
 *
 * Actions that need to be performed on the render thread.
 */
void CinemaApp::command(const VEvent &event )
{
    if (modelMgr.Command(event))
	{
		return;
	}

    if (m_viewMgr.command(event)) {
		return;
	}

    if (sceneMgr.Command(event)) {
		return;
	}
}

/*
 * Frame()
 *
 * App override
 */
VR4Matrixf CinemaApp::onNewFrame( const VrFrame vrFrame )
{
	m_frameCount++;
	this->m_vrFrame = vrFrame;

	return m_viewMgr.frame( vrFrame );
}

bool CinemaApp::onKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType )
{
	return m_viewMgr.onKeyEvent( keyCode, eventType );
}

} // namespace OculusCinema
