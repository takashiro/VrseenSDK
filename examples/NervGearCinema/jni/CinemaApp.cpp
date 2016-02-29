/************************************************************************************

Filename    :   CinemaApp.cpp
Content     :
Created     :	6/17/2014
Authors     :   Jim Dos�

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "String_Utils.h"
#include "CinemaApp.h"
#include "Native.h"
#include "CinemaStrings.h"


//=======================================================================================

namespace OculusCinema {

CinemaApp::CinemaApp() :
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
void CinemaApp::OneTimeInit( const char * fromPackage, const char * launchIntentJSON, const char * launchIntentURI ) {
	LOG( "--------------- CinemaApp OneTimeInit ---------------");

	startTime = ovr_GetTimeInSeconds();

	app->GetVrParms().colorFormat = COLOR_8888;
	//app->GetVrParms().depthFormat = DEPTH_16;
	app->GetVrParms().multisamples = 2;

	Native::OneTimeInit( app, ActivityClass );
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

void CinemaApp::OneTimeShutdown()
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

const char * CinemaApp::retailDir( const char *dir ) const
{
	static char subDir[ 256 ];
    StringUtils::SPrintf( subDir, "%s/%s", sdcardDir( "RetailMedia" ), dir );
	return subDir;
}

const char * CinemaApp::externalRetailDir( const char *dir ) const
{
	static char subDir[ 256 ];
    StringUtils::SPrintf( subDir, "%s/%s", externalSDCardDir( "RetailMedia" ), dir );
	return subDir;
}

const char * CinemaApp::sdcardDir( const char *dir ) const
{
	static char subDir[ 256 ];
	StringUtils::SPrintf( subDir, "/sdcard/%s", dir );
	return subDir;
}

const char * CinemaApp::externalSDCardDir( const char *dir ) const
{
	static char subDir[ 256 ];
	StringUtils::SPrintf( subDir, "/storage/extSdCard/%s", dir );
	return subDir;
}

const char * CinemaApp::externalCacheDir( const char *dir ) const
{
	static char subDir[ 256 ];
    StringUtils::SPrintf( subDir, "%s/%s", Native::GetExternalCacheDirectory( app ).toCString(), dir );
	return subDir;
}

bool CinemaApp::isExternalSDCardDir( const char *dir ) const
{
    const char * sdcardDir = externalSDCardDir( "" );
	const int l = strlen( sdcardDir );
	return ( 0 == strncmp( sdcardDir, dir, l ) );
}

bool CinemaApp::fileExists( const char *filename ) const
{
	FILE *f = fopen( filename, "r" );
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

void CinemaApp::setPlaylist( const Array<const MovieDef *> &playList, const int nextMovie )
{
	m_playList = playList;

    assert( nextMovie < m_playList.sizeInt() );
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
    if ( m_playList.sizeInt() != 0 )
	{
        for( int i = 0; i < m_playList.sizeInt() - 1; i++ )
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
    if ( m_playList.sizeInt() != 0 )
	{
        for( int i = 0; i < m_playList.sizeInt(); i++ )
		{
			if ( m_playList[ i ] == m_currentMovie )
			{
				break;
			}
			previous = m_playList[ i ];
		}

		if ( previous == NULL )
		{
            previous = m_playList[ m_playList.sizeInt() - 1 ];
		}
	}

	return previous;
}

void CinemaApp::startMoviePlayback()
{
	if ( m_currentMovie != NULL )
	{
		m_movieFinishedPlaying = false;
        Native::StartMovie( app, m_currentMovie->Filename.toCString(), m_shouldResumeMovie, m_currentMovie->IsEncrypted, false );
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
    if ( Native::CheckForMovieResume( app, m_currentMovie->Filename.toCString() ) )
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
	inLobby = inLobby;
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
Matrix4f CinemaApp::DrawEyeView( const int eye, const float fovDegrees ) {
	return m_viewMgr.drawEyeView( eye, fovDegrees );
}

void CinemaApp::ConfigureVrMode( ovrModeParms & modeParms ) {
	// We need very little CPU for movie playing, but a fair amount of GPU.
	// The CPU clock should ramp up above the minimum when necessary.
	LOG( "ConfigureClocks: Cinema only needs minimal clocks" );

	modeParms.CpuLevel = 1;
	modeParms.GpuLevel = 2;

	// when the app is throttled, go to the platform UI and display a
	// dismissable warning. On return to the app, force 30hz timewarp.
	modeParms.AllowPowerSave = true;

	// Always use 2x MSAA for now
	app->GetVrParms().multisamples = 2;
}

/*
 * Command
 *
 * Actions that need to be performed on the render thread.
 */
void CinemaApp::Command( const char * msg )
{
	if ( modelMgr.Command( msg ) )
	{
		return;
	}

	if ( m_viewMgr.command( msg ) )
	{
		return;
	}

	if ( sceneMgr.Command( msg ) )
	{
		return;
	}
}

/*
 * Frame()
 *
 * App override
 */
Matrix4f CinemaApp::Frame( const VrFrame vrFrame )
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
