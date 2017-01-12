#include "PanoCinema.h"
#include "VColor.h"
#include "core/VTimer.h"
#include <android/JniUtils.h>
#include <io/VResource.h>
#include <gui/VTileButton.h>
#include <VGui.h>

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

    std::function<void()> hideLoading = [=](){
        vApp->eventLoop().post("activityInitCompleted");

        // We might want to save the view state and position for perfect recall
        VString assets = "assets/";
        const char *buttonImages[4] = {"game1.jpg", "game2.jpg", "video1.jpg", "video2.jpg"};
        VTileButton *buttons[4];

        VGui *gui = vApp->gui();
        VRect3f buttonSize(-0.8f, -0.6f, 0.0f, 0.8f, 0.6f, 0.0f);
        for (int i = 0; i < 4; i++){
            VTileButton *button = new VTileButton;
            buttons[i] = button;
            VResource image(assets + buttonImages[i]);
            button->setRect(buttonSize);
            button->setImage(image);
            gui->addItem(button);
        }

        VMatrix4f matrix = VMatrix4f::Translation(40.0f,60.0f,15.0f);
        buttons[0]->setPos(matrix.transform(VVect3f(-0.85f, 0.65f, -3.0f)));
        buttons[1]->setPos(matrix.transform(VVect3f(-0.85f, -0.65f, -3.0f)));
        buttons[2]->setPos(matrix.transform(VVect3f(0.85f, 0.65f, -3.0f)));
        buttons[3]->setPos(matrix.transform(VVect3f(0.85f, -0.65f, -3.0f)));


        auto showGuiLoading = [=](){
            gui->showLoading(0);
        };
        auto hideGuiLoading = [=](){
            gui->removeLoading();
        };
        for (VTileButton *button : buttons) {
            button->setOnBlurListener(hideGuiLoading);
            button->setOnFocusListener(showGuiLoading);
        }

        VString sdcard = "/storage/emulated/0/VRSeen/SDK/";
        JNIEnv *jni = vApp->vrJni();
        jobject activity = vApp->javaObject();
        jmethodID startApp = jni->GetMethodID(jni->GetObjectClass(activity), "startApp", "(Ljava/lang/String;Ljava/lang/String;)V");

        buttons[2]->setOnStareListener([=](){
            jni->CallVoidMethod(activity, startApp, JniUtils::Convert(jni, "com.vrseen.panovideo"), JniUtils::Convert(jni, sdcard + "360Videos/[Samsung] 360 video demo.mp4"));
        });

        buttons[3]->setOnStareListener([=](){
            jni->CallVoidMethod(activity, startApp, JniUtils::Convert(jni, "com.vrseen.panovideo"), JniUtils::Convert(jni, sdcard + "360Videos/1.mp4"));
        });
    };

    VVariantArray args;
    args << "/mnt/sdcard/VRSeen/SDK/models/158/MG158_52.obj" << hideLoading;
    vApp->eventLoop().post("loadModel", args);

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
