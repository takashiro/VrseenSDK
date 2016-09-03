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
class VGazeCursor;
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

    void recenterYaw( const bool showBlack );
    void setRecenterYawFrameStart( const long long frameNumber );
    long long recenterYawFrameStart() const;

    VEyeItem::Settings &eyeSettings();

    VGazeCursor  &gazeCursor();
    BitmapFont &defaultFont();
    BitmapFontSurface &worldFontSurface();
    const VStandardPath &storagePaths();

    bool hasHeadphones() const;
    bool framebufferIsSrgb() const;
    bool framebufferIsProtected() const;
    bool renderMonoMode() const;
    void setRenderMonoMode(bool const mono);

    const VString &packageCodePath() const;

    VMatrix4f const &lastViewMatrix() const;
    void setLastViewMatrix( VMatrix4f const & m );

    VEyeItem::Settings &vrParms();

    const VViewSettings &viewSettings() const;
    void setViewSettings(const VViewSettings &settings);

    void setPopupDistance( float const d );
    float popupDistance() const;
    void setPopupScale( float const s );
    float popupScale() const;

    KeyState &backKeyState();

    JNIEnv *vrJni();
    jobject &javaObject();

    VKernel* kernel();
    SurfaceTexture *dialogTexture();

    const VTimeWarpParms &swapParms() const;
    VTimeWarpParms &swapParms();

    const VRotationState &sensorForNextWarp() const;

    void drawScreenMask( const VMatrix4f & mvp, const float fadeFracX, const float fadeFracY );
    void drawBounds( const VVect3f &mins, const VVect3f &maxs, const VMatrix4f &mvp, const VVect3f &color );

    void execute();
    void quit();

    bool isRunning() const;
    bool isShowFPS() const;
    void showFPS(bool const show);

    //SurfaceTexture * GetCameraTexture();

/*    float           cameraFovHorizontal;
    float           cameraFovVertical;*/

    const VZipFile &apkFile() const;

    VMatrix4f drawEyeViewsFirst( VMatrix4f const & centerViewMatrix);
    void drawEyeViewsSecond(VMatrix4f mvp, const int numPresents = 1);

    VText text;
    VPanel panel;
    VDialog dialog;

    SurfaceTexture * cameraTexture;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(App)
};

extern App *vAppInstance;
#define vApp vAppInstance

NV_NAMESPACE_END
