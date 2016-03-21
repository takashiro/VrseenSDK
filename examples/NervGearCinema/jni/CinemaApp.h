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

#include "VMainActivity.h"

using namespace NervGear;

namespace OculusCinema {

class CinemaApp : public NervGear::VMainActivity
{
public:
    CinemaApp(JNIEnv *jni, jclass activityClass, jobject activityObject);

    void init(const VString &fromPackage, const VString &launchIntentJSON, const VString &launchIntentURI ) override;
    void shutdown() override;

    Matrix4f drawEyeView( const int eye, const float fovDegrees ) override;

    void ConfigureVrMode(ovrModeParms & modeParms) override;

    void Command( const char * msg ) override;
    bool onKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType ) override;

	// Called by App loop
    Matrix4f onNewFrame( const VrFrame m_vrFrame ) override;

    void			    	setPlaylist( const VArray<const MovieDef *> &playList, const int nextMovie );
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

    VString retailDir(const VString &dir) const;
    VString externalRetailDir(const VString &dir) const;
    VString sdcardDir(const VString &dir) const;
    VString externalSDCardDir(const VString &dir) const;
    VString externalCacheDir(const VString &dir) const;
    bool isExternalSDCardDir(const VString &dir ) const;
    bool fileExists(const VString &filename ) const;

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
    VArray<const MovieDef *> m_playList;

    bool					m_shouldResumeMovie;
    bool					m_movieFinishedPlaying;
};

} // namespace OculusCinema
