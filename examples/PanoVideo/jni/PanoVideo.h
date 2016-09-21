#pragma once

#include <VMainActivity.h>
#include <VPath.h>
#include "VRectangle.h"
#include <VProgressBar.h>
#include <gui/VProgressBar.h>
#include "VShadow.h"
#include "VSphere.h"


#include "ModelView.h"

NV_NAMESPACE_BEGIN

class PanoVideo : public VMainActivity
{
public:

	enum OvrMenuState
	{
		MENU_NONE,
		MENU_BROWSER,
		MENU_VIDEO_LOADING,
		MENU_VIDEO_READY,
		MENU_VIDEO_PLAYING,
		NUM_MENU_STATES
	};

    PanoVideo(JNIEnv *jni, jclass activityClass, jobject activityObject);
    ~PanoVideo();

    void init(const VString &, const VString &, const VString &) override;
    void shutdown() override;
    void configureVrMode(VKernel *kernel) override;
    VMatrix4f drawEyeView(const int eye, const float fovDegrees) override;
    VMatrix4f onNewFrame(VFrame vrFrame) override;
    void command(const VEvent &event) override;
    bool onKeyEvent(int keyCode, const KeyState::eKeyEventType eventType) override;

    void stop();

    void onStart(const VString &url);

    VMatrix4f texmForVideo(int eye);
    VMatrix4f texmForBackground(int eye);

    void setMenuState( const OvrMenuState state);
    OvrMenuState currentState() const { return m_menuState; }

    void setFrameAvailable(bool available) { m_frameAvailable = available; }

    const VPath &videoUrl() { return m_videoUrl; }

    void onResume() override;
    void onPause() override;

	void mediaPause();

	void movePos(const VMatrix4f &mvp);

	VProgressBar * progressBar;
	VShadow * shadow;
	VSphere *sphere;


	bool pause;



private:
    // shared vars
    VGlGeometry m_globe;
    VSceneView m_scene;
    bool m_videoWasPlayingWhenPaused;	// state of video when main activity was paused

    // panorama vars
    VGlShader m_panoramaProgram;
    VGlShader m_fadedPanoramaProgram;
    VGlShader m_singleColorTextureProgram;

    VPath m_videoUrl;
    OvrMenuState m_menuState;

    bool m_useSrgb;

    // video vars
    SurfaceTexture	*m_movieTexture;

	// Set when MediaPlayer knows what the stream size is.
	// current is the aspect size, texture may be twice as wide or high for 3D content.
    int m_videoWidth;	// set to 0 when a new movie is started, don't render until non-0
    int m_videoHeight;

    GLuint m_backgroundTexId;
    int m_backgroundWidth;
    int m_backgroundHeight;

    bool m_frameAvailable;
	MovieFormat  m_movieFormat;
};

NV_NAMESPACE_END
