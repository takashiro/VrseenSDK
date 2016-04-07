#pragma once

#include "VEyeBuffer.h"
#include "vglobal.h"
#include "api/VKernel.h"
#include "KeyState.h"
#include "Input.h"
#include "VEventLoop.h"
#include "VSoundManager.h"
#include "gui/VText.h"
#include "gui/VPanel.h"
#include "gui/VDialog.h"

NV_NAMESPACE_BEGIN

class VMainActivity;

struct MaterialParms;
class VRMenuObjectParms;
class OvrGuiSys;
class OvrGazeCursor;
class BitmapFont;
class BitmapFontSurface;
class OvrVRMenuMgr;
class OvrDebugLines;
class App;
class VrViewParms;
class VStandardPath;
class SurfaceTexture;
class OvrGuiSys;
class GazeCursor;
class OvrVolumePopup;

class App
{
public:
    App(JNIEnv *jni, jobject activityObject, VMainActivity *activity);
    virtual ~App();

    VEventLoop &eventLoop();

    VMainActivity *appInterface();

    void drawEyeViewsPostDistorted( VR4Matrixf const & viewMatrix, const int numPresents = 1);

    void createToast( const char * fmt, ... );

    void playSound( const char * name );

    void recenterYaw( const bool showBlack );
    void setRecenterYawFrameStart( const long long frameNumber );
    long long recenterYawFrameStart() const;

    VEyeBuffer::EyeParms eyeParms();
    void setEyeParms(const VEyeBuffer::EyeParms parms);

    OvrGuiSys &guiSys();
    OvrGazeCursor  &gazeCursor();
    BitmapFont &defaultFont();
    BitmapFontSurface &worldFontSurface();
    BitmapFontSurface &menuFontSurface();
    OvrVRMenuMgr &vrMenuMgr();
    OvrDebugLines &debugLines();
    const VStandardPath &storagePaths();
    VSoundManager &soundMgr();

    bool hasHeadphones() const;
    bool framebufferIsSrgb() const;
    bool framebufferIsProtected() const;
    bool renderMonoMode() const;
    void setRenderMonoMode(bool const mono);

    const VString &packageCodePath() const;

    VR4Matrixf const &lastViewMatrix() const;
    void setLastViewMatrix( VR4Matrixf const & m );

    VEyeBuffer::EyeParms &vrParms();

    const VrViewParms &vrViewParms() const;
    void setVrViewParms( VrViewParms const & parms );

    void setPopupDistance( float const d );
    float popupDistance() const;
    void setPopupScale( float const s );
    float popupScale() const;

    bool isGuiOpen() const;

    KeyState &backKeyState();

    void setShowVolumePopup( bool const show );
    bool showVolumePopup() const;

    JavaVM *javaVM();
    JNIEnv *uiJni();
    JNIEnv *vrJni();
    jobject &javaObject();
    jclass &vrActivityClass();

    VKernel* kernel();
    SurfaceTexture *dialogTexture();


    const ovrSensorState &sensorForNextWarp() const;

    void drawScreenMask( const VR4Matrixf & mvp, const float fadeFracX, const float fadeFracY );
    void drawBounds( const V3Vectf &mins, const V3Vectf &maxs, const VR4Matrixf &mvp, const V3Vectf &color );

    void execute();
    void quit();

    VText text;
    VPanel panel;
    VDialog dialog;
    volatile bool oneTimeInitCalled;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(App)
};

extern App *NervGearAppInstance;
#define vApp NervGearAppInstance

NV_NAMESPACE_END
