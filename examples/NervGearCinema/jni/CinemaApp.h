#include "App.h"
#include "ShaderManager.h"
#include "ModelManager.h"
#include "SceneManager.h"
#include "ViewManager.h"
#include "MovieManager.h"
#include "MoviePlayerView.h"
#include "MovieSelectionView.h"
#include "TheaterSelectionView.h"
#include "ResumeMovieView.h"

using namespace NervGear;

namespace OculusCinema {

class CinemaApp : public NervGear::VrAppInterface
{
public:
							CinemaApp();

    void OneTimeInit( const char * fromPackage, const char * launchIntentJSON, const char * launchIntentURI ) override;
    void OneTimeShutdown() override;

    Matrix4f DrawEyeView( const int eye, const float fovDegrees ) override;

    void ConfigureVrMode(ovrModeParms & modeParms) override;

    void Command( const char * msg ) override;
    bool onKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType ) override;

	// Called by App loop
    Matrix4f Frame( const VrFrame m_vrFrame ) override;

    void			    	setPlaylist( const Array<const MovieDef *> &playList, const int nextMovie );
    void			    	setMovie( const MovieDef * nextMovie );
    void 					movieLoaded( const int width, const int height, const int duration );

    const MovieDef *		currentMovie() const { return m_currentMovie; }
    const MovieDef *		nextMovie() const;
    const MovieDef *		previousMovie() const;

    const SceneDef & 		currentTheater() const;

    void 					startMoviePlayback();
    void 					resumeMovieFromSavedLocation();
    void					playMovieFromBeginning();
    void 					resumeOrRestartMovie();
    void 					theaterSelection();
    void 					setMovieSelection( bool inLobby );
    void					movieFinished();
    void					unableToPlayMovie();

    bool 					allowTheaterSelection() const;
    bool 					isMovieFinished() const;

    const char *			retailDir( const char *dir ) const;
    const char *			externalRetailDir( const char *dir ) const;
    const char *			sdcardDir( const char *dir ) const;
    const char * 			externalSDCardDir( const char *dir ) const;
    const char * 			externalCacheDir( const char *dir ) const;
    bool 					isExternalSDCardDir( const char *dir ) const;
    bool 					fileExists( const char *filename ) const;

public:
    double					startTime;

    jclass					mainActivityClass;	// need to look up from main thread

    SceneManager			sceneMgr;
    ShaderManager 			shaderMgr;
    ModelManager 			modelMgr;
    MovieManager 			movieMgr;

    bool					inLobby;
    bool					allowDebugControls;

private:
    ViewManager				m_viewMgr;
    MoviePlayerView			m_moviePlayer;
    MovieSelectionView		m_movieSelectionMenu;
    TheaterSelectionView	m_theaterSelectionMenu;
    ResumeMovieView			m_resumeMovieMenu;

    VrFrame					m_vrFrame;
    int						m_frameCount;

    const MovieDef *		m_currentMovie;
    Array<const MovieDef *> m_playList;

    bool					m_shouldResumeMovie;
    bool					m_movieFinishedPlaying;
};

} // namespace OculusCinema
