#include "PanoCinema.h"
#include "VColor.h"
#include "core/VTimer.h"
#include <android/JniUtils.h>

NV_USING_NAMESPACE

extern "C" {

void Java_com_vrseen_panocinema_PanoCinema_construct(JNIEnv *jni, jclass, jobject activity)
{
    (new PanoCinema(jni, jni->GetObjectClass(activity), activity))->onCreate(nullptr, nullptr, nullptr);
}

void Java_com_vrseen_panocinema_PanoCinema_onStart(JNIEnv *jni, jclass, jstring jpath)
{
    PanoCinema *cinema = (PanoCinema *) vApp->appInterface();
    cinema->onStart(JniUtils::Convert(jni, jpath));
}

void Java_com_vrseen_panocinema_PanoCinema_onFrameAvailable(JNIEnv *, jclass)
{
//    PanoCinema *cinema = (PanoCinema *) vApp->appInterface();
//    cinema->setFrameAvailable(true);
}

jobject Java_com_vrseen_panocinema_PanoCinema_createMovieTexture(JNIEnv *, jclass)
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

void Java_com_vrseen_panocinema_PanoCinema_onVideoSizeChanged(JNIEnv *, jclass, jint width, jint height)
{
    VVariantArray args;
    args << width << height;
    vApp->eventLoop().post("video", std::move(args));
}

void Java_com_vrseen_panocinema_PanoCinema_onCompletion(JNIEnv *, jclass)
{
	vInfo("nativeVideoCompletion");
    vApp->eventLoop().post( "completion" );
}

} // extern "C"


PanoCinema::PanoCinema(JNIEnv *jni, jclass activityClass, jobject activityObject)
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
	m_movieFinishedPlaying( true )
{
}

/*
 * OneTimeInit
 *
 */
void PanoCinema::init(const VString &, const VString &, const VString &launchIntentURI)
{
	vInfo("--------------- PanoCinema OneTimeInit ---------------");

    startTime = VTimer::Seconds();

    vApp->vrParms().colorFormat = VColor::COLOR_8888;
	//app->GetVrParms().depthFormat = DEPTH_16;
    vApp->vrParms().multisamples = 2;

    shaderMgr.OneTimeInit( launchIntentURI );
    sceneMgr.OneTimeInit(launchIntentURI);

    vInfo("PanoCinema::OneTimeInit:" << (VTimer::Seconds() - startTime) << "seconds");
}

void PanoCinema::onStart(const VString &url)
{
    if (!url.isEmpty()) {
        vInfo("StartVideo(" << url << ")");

        inLobby = false;
        MovieDef* movieDef = new MovieDef;
        movieDef->Filename = url;
        movieDef->Is3D = movieDef->Filename.contains("/3D/");
        setMovie(movieDef);
	}
}

void PanoCinema::stop()
{
    sceneMgr.ClearMovie();
}

void PanoCinema::shutdown()
{
	vInfo("--------------- PanoCinema OneTimeShutdown ---------------");

	shaderMgr.OneTimeShutdown();
	sceneMgr.OneTimeShutdown();
}

void PanoCinema::setMovie( const MovieDef *movie )
{
    vInfo("SetMovie(" << movie->Filename << ")");
	m_currentMovie = movie;
	m_movieFinishedPlaying = false;
}

bool PanoCinema::isMovieFinished() const
{
	return m_movieFinishedPlaying;
}

/*
 * DrawEyeView
 */
VMatrix4f PanoCinema::drawEyeView( const int eye, const float fovDegrees ) {
    return sceneMgr.DrawEyeView( eye, fovDegrees );
}

void PanoCinema::configureVrMode(VKernel* kernel)
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
void PanoCinema::command(const VEvent &event )
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
VMatrix4f PanoCinema::onNewFrame( const VFrame vrFrame )
{
	m_frameCount++;
	this->m_vrFrame = vrFrame;

    return sceneMgr.Frame( vrFrame );
}
