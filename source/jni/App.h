#pragma once

#include "VViewSettings.h"

#include "scene/VEyeItem.h"
#include "api/VKernel.h"
#include "KeyState.h"
#include "VFrame.h"
#include "VEventLoop.h"
#include "gui/VText.h"
#include "gui/VPanel.h"
#include "gui/VDialog.h"

NV_NAMESPACE_BEGIN

class VMainActivity;
class VZipFile;
class VRMenuObjectParms;
class OvrGazeCursor;
class BitmapFont;
class BitmapFontSurface;
class VViewSettings;
class VStandardPath;
class SurfaceTexture;

class App
{
public:

    App(JNIEnv *jni, jobject activityObject, VMainActivity *activity);
    virtual ~App();

    VEventLoop &eventLoop();

    VMainActivity *appInterface();

    void drawEyeViewsPostDistorted( VR4Matrixf const & viewMatrix, const int numPresents = 1);

    void recenterYaw( const bool showBlack );
    void setRecenterYawFrameStart( const long long frameNumber );
    long long recenterYawFrameStart() const;

    VEyeItem::Settings &eyeSettings();

    OvrGazeCursor  &gazeCursor();
    BitmapFont &defaultFont();
    BitmapFontSurface &worldFontSurface();
    const VStandardPath &storagePaths();

    bool hasHeadphones() const;
    bool framebufferIsSrgb() const;
    bool framebufferIsProtected() const;
    bool renderMonoMode() const;
    void setRenderMonoMode(bool const mono);

    const VString &packageCodePath() const;

    VR4Matrixf const &lastViewMatrix() const;
    void setLastViewMatrix( VR4Matrixf const & m );

    VEyeItem::Settings &vrParms();

    const VViewSettings &viewSettings() const;
    void setViewSettings(const VViewSettings &settings);

    void setPopupDistance( float const d );
    float popupDistance() const;
    void setPopupScale( float const s );
    float popupScale() const;

    KeyState &backKeyState();

    JavaVM *javaVM();
    JNIEnv *vrJni();
    jobject &javaObject();

    VKernel* kernel();
    SurfaceTexture *dialogTexture();


    const VRotationState &sensorForNextWarp() const;

    void drawScreenMask( const VR4Matrixf & mvp, const float fadeFracX, const float fadeFracY );
    void drawBounds( const V3Vectf &mins, const V3Vectf &maxs, const VR4Matrixf &mvp, const V3Vectf &color );

    void execute();
    void quit();

    bool isRunning() const;
    bool isShowFPS() const;
    void showFPS(bool const show);

    const VZipFile &apkFile() const;

    VText text;
    VPanel panel;
    VDialog dialog;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(App)
};

extern App *vAppInstance;
#define vApp vAppInstance

NV_NAMESPACE_END
