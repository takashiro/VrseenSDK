#pragma once

#include <VMainActivity.h>
#include <VPath.h>

#include "ModelView.h"

NV_NAMESPACE_BEGIN

class ArCamera : public VMainActivity
{
public:

	GLuint program;
	GLuint vao;

    ArCamera(JNIEnv *jni, jclass activityClass, jobject activityObject);
    ~ArCamera();

    void init(const VString &, const VString &, const VString &) override;
    void shutdown() override;
    void configureVrMode(VKernel *kernel) override;
    VMatrix4f drawEyeView(const int eye, const float fovDegrees) override;
    VMatrix4f onNewFrame(VFrame vrFrame) override;
    void command(const VEvent &event) override;
    bool onKeyEvent(int keyCode, const KeyState::eKeyEventType eventType) override;

    void stop();

    void onResume() override;
    void onPause() override;

private:
    // shared vars
	VSceneView m_scene;
    bool m_videoWasPlayingWhenPaused;	// state of video when main activity was paused

    bool m_useSrgb;

    // video vars
    SurfaceTexture	*m_movieTexture;
};

NV_NAMESPACE_END
