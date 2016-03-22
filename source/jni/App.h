#pragma once

#include "vglobal.h"

#include "VrApi.h"
#include "VrApi_Android.h"
#include "KeyState.h"
#include "EyeBuffers.h"
#include "Input.h"
#include "VEventLoop.h"
#include "VSoundManager.h"

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

    EyeParms eyeParms();
    void setEyeParms(const EyeParms parms);

    OvrGuiSys &guiSys();
    OvrGazeCursor  &gazeCursor();
    BitmapFont &defaultFont();
    BitmapFontSurface &worldFontSurface();
    BitmapFontSurface &menuFontSurface();
    OvrVRMenuMgr &vrMenuMgr();
    OvrDebugLines &debugLines();
    const VStandardPath &storagePaths();
    VSoundManager &soundMgr();

    int wifiSignalLevel() const;
    eWifiState wifiState() const;
    int cellularSignalLevel() const;
    eCellularState cellularState() const;

    bool isAsynchronousTimeWarp() const;
    bool hasHeadphones() const;
    bool framebufferIsSrgb() const;
    bool framebufferIsProtected() const;
    bool renderMonoMode() const;
    void setRenderMonoMode(bool const mono);

    const VString &packageCodePath() const;

    VR4Matrixf const &lastViewMatrix() const;
    void setLastViewMatrix( VR4Matrixf const & m );

    EyeParms &vrParms();
    ovrModeParms vrModeParms();
    void setVrModeParms( ovrModeParms parms );

    const VrViewParms &vrViewParms() const;
    void setVrViewParms( VrViewParms const & parms );

    void setPopupDistance( float const d );
    float popupDistance() const;
    void setPopupScale( float const s );
    float popupScale() const;

    int cpuLevel() const;
    int gpuLevel() const;

    bool isPowerSaveActive() const;

    int batteryLevel() const;
    eBatteryStatus batteryStatus() const;

    bool isGuiOpen() const;

    KeyState &backKeyState();

    ovrMobile *getOvrMobile();

    void setShowVolumePopup( bool const show );
    bool showVolumePopup() const;

    JavaVM *javaVM();
    JNIEnv *uiJni();
    JNIEnv *vrJni();
    jobject &javaObject();
    jclass &vrActivityClass();

    SurfaceTexture *dialogTexture();

    const ovrTimeWarpParms &swapParms() const;
    ovrTimeWarpParms &swapParms();

    const ovrSensorState &sensorForNextWarp() const;

    void drawScreenMask( const ovrMatrix4f & mvp, const float fadeFracX, const float fadeFracY );
    void drawScreenDirect( const GLuint texid, const ovrMatrix4f & mvp );

    void setShowFPS( bool const show );
    bool showFPS() const;

    void showInfoText( float const duration, const char * fmt, ... );
    void showInfoText( float const duration, V3Vectf const & offset, V4Vectf const & color, const char * fmt, ... );

    VR4Matrixf matrixInterpolation( const VR4Matrixf & startMatrix, const VR4Matrixf & endMatrix, double t );

    void drawDialog( const VR4Matrixf & mvp );
    void drawPanel( const GLuint externalTextureId, const VR4Matrixf & dialogMvp, const float alpha );

    void drawBounds( const V3Vectf &mins, const V3Vectf &maxs, const VR4Matrixf &mvp, const V3Vectf &color );

    void startVrThread();
    void stopVrThread();
    void syncVrThread();

    volatile bool oneTimeInitCalled;
    ovrModeParms VrModeParms;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(App)
};

extern App *NervGearAppInstance;
#define vApp NervGearAppInstance

NV_NAMESPACE_END
