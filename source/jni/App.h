#pragma once

#include "vglobal.h"

#include "VrApi.h"
#include "VrApi_Android.h"
#include "KeyState.h"
#include "EyeBuffers.h"
#include "Input.h"
#include "VMessageQueue.h"
#include "TalkToJava.h"
#include "SoundManager.h"

NV_NAMESPACE_BEGIN

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

class VrAppInterface
{
public:
							 VrAppInterface();
	virtual					~VrAppInterface();

	// Each onCreate in java will allocate a new java object, but
	// we may reuse a single C++ object for them to make repeated
	// opening of the platformUI faster, and to not require full
	// cleanup on destruction.
	jlong SetActivity( JNIEnv * jni, jclass clazz, jobject activity, 
			jstring javaFromPackageNameString, jstring javaCommandString, 
			jstring javaUriString );

	// All subclasses communicate with App through this member.
	App *	app;				// Allocated in the first call to SetActivity()
	jclass	ActivityClass;		// global reference to clazz passed to SetActivity

	// This will be called one time only, before SetActivity()
	// returns from the first creation.
	//
	// It is called from the VR thread with an OpenGL context current.
	//
	// If the app was launched without a specific intent, launchIntent
	// will be an empty string.
    virtual void OneTimeInit(const VString &fromPackage, const VString &launchIntentJSON, const VString &launchIntentURI) = 0;

	// This will be called one time only, when the app is about to shut down.
	//
	// It is called from the VR thread before the OpenGL context is destroyed.
	//
	// If the app needs to free OpenGL resources this is the place to do so.
	virtual void OneTimeShutdown();

	// Frame will only be called if the window surfaces have been created.
	//
	// The application should call DrawEyeViewsPostDistorted() to get
	// the DrawEyeView() callbacks.
	//
	// Any GPU operations that are relevant for both eye views, like
	// buffer updates or dynamic texture rendering, should be done first.
	//
	// Return the view matrix the framework should use for positioning
	// new pop up dialogs.
	virtual Matrix4f Frame( VrFrame vrFrame );

	// Called by DrawEyeViewsPostDisorted().
	//
	// The color buffer will have already been cleared or discarded, as
	// appropriate for the GPU.
	//
	// The viewport and scissor will already be set.
	//
	// Return the MVP matrix for the framework to use for drawing the
	// pop up dialog and debug graphics.
	//
	// fovDegrees may be different on different devices.
	virtual Matrix4f DrawEyeView( const int eye, const float fovDegrees );

	// If the app receives a new intent after launch, it will be sent to
	// this function.
	virtual void NewIntent( const char * fromPackageName, const char * command, const char * uri );

	// This is called on each resume, before VR Mode has been entered, to allow
	// the application to make changes.
	virtual void ConfigureVrMode( ovrModeParms & modeParms );

	// The window has been created and OpenGL is available.
	// This happens every time the user switches back to the app.
	virtual void WindowCreated();

	// The window is about to be destroyed, due to the user switching away.
	// The app will go to sleep until another message arrives.
	// Applications can use this to save game state, etc.
	virtual void WindowDestroyed();

	// The app is about to be paused.
	virtual void Paused();

	// The app is about to be resumed.
	virtual void Resumed();

	// Handle generic commands forwarded from other threads.
	// Commands can be processed even when the window surfaces
	// are not setup and the app would otherwise be sleeping.
	// The msg string will be freed by the framework after
	// command processing.
	virtual void Command( const char * msg );

	// The VrApp should return true if it consumes the key.
	virtual bool onKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType );

	// Called by the application framework when the VR warning message is closed by the user.
	virtual bool OnVrWarningDismissed( const bool accepted );

	// Overload this and return false to skip showing the spinning loading icon.
	virtual bool ShouldShowLoadingIcon() const;

	// Overload and return true to have these attributes added to the window surface:
	// EGL_GL_COLORSPACE_KHR,  EGL_GL_COLORSPACE_SRGB_KHR
	virtual bool wantSrgbFramebuffer() const;

	// Overload and return true to have these attributes added to the window surface:
	// EGL_PROTECTED_CONTENT_EXT, EGL_TRUE
	virtual bool GetWantProtectedFramebuffer() const;
};

class OvrGuiSys;
class GazeCursor;
class OvrVolumePopup;

//==============================================================
// AppLocal
//
// NOTE: do not define any of the functions in here inline (inside of the class
// definition).  When AppLocal.h is included in multiple files (App.cpp and AppRender.cpp)
// this causes bugs with accessor functions.
class App : public TalkToJavaInterface
{
public:
                                App(JNIEnv *jni, jobject activityObject,
                                        VrAppInterface & interface );
    virtual						~App();

    VMessageQueue &		messageQueue();

    VrAppInterface *	appInterface();

    void 				drawEyeViewsPostDistorted( Matrix4f const & viewMatrix, const int numPresents = 1);

    void				createToast( const char * fmt, ... );

    void				playSound( const char * name );

    void				recenterYaw( const bool showBlack );
    void				setRecenterYawFrameStart( const long long frameNumber );
    long long			recenterYawFrameStart() const;


    //-----------------------------------------------------------------

    EyeParms			eyeParms();
    void				setEyeParms( const EyeParms parms );

    //-----------------------------------------------------------------
    // interfaces
    //-----------------------------------------------------------------
    OvrGuiSys &         	guiSys();
    OvrGazeCursor  &    	gazeCursor();
    BitmapFont &        	defaultFont();
    BitmapFontSurface & 	worldFontSurface();
    BitmapFontSurface & 	menuFontSurface();
    OvrVRMenuMgr &      	vrMenuMgr();
    OvrDebugLines &     	debugLines();
    const VStandardPath & storagePaths();
    OvrSoundManager &		soundMgr();

    //-----------------------------------------------------------------
    // system settings
    //-----------------------------------------------------------------

    int					wifiSignalLevel() const;
    eWifiState			wifiState() const;
    int					cellularSignalLevel() const;
    eCellularState		cellularState() const;

    //-----------------------------------------------------------------
    // accessors
    //-----------------------------------------------------------------

    bool				isAsynchronousTimeWarp() const;
    bool				hasHeadphones() const;
    bool				framebufferIsSrgb() const;
    bool				framebufferIsProtected() const;
    bool				renderMonoMode() const;
    void				setRenderMonoMode( bool const mono );

    const VString &packageCodePath() const;

    Matrix4f const &	lastViewMatrix() const;
    void				setLastViewMatrix( Matrix4f const & m );

    EyeParms &			vrParms();
    ovrModeParms 		vrModeParms();
    void				setVrModeParms( ovrModeParms parms );

    VrViewParms const &	vrViewParms() const;
    void				setVrViewParms( VrViewParms const & parms );

    void				setPopupDistance( float const d );
    float				popupDistance() const;
    void				setPopupScale( float const s );
    float				popupScale() const;

    int					cpuLevel() const;
    int					gpuLevel() const;

    bool				isPowerSaveActive() const;

    int					batteryLevel() const;
    eBatteryStatus		batteryStatus() const;

    bool				isGuiOpen() const;

    KeyState &          backKeyState();

    ovrMobile *			getOvrMobile();

    void				setShowVolumePopup( bool const show );
    bool				showVolumePopup() const;

    const VString &packageName() const;

    //-----------------------------------------------------------------
    // Java accessors
    //-----------------------------------------------------------------

    JavaVM	*			javaVM();
    JNIEnv	*			uiJni();
    JNIEnv	*			vrJni();
    jobject	&			javaObject();
    jclass	&			vrActivityClass();

    //-----------------------------------------------------------------

    // Every application gets a basic dialog surface.
    SurfaceTexture *	dialogTexture();

    //-----------------------------------------------------------------
    // TimeWarp
    //-----------------------------------------------------------------
    ovrTimeWarpParms const & swapParms() const;
    ovrTimeWarpParms &		swapParms();

    ovrSensorState const & sensorForNextWarp() const;

    // Draw a zero to destination alpha
    void				drawScreenMask( const ovrMatrix4f & mvp, const float fadeFracX, const float fadeFracY );
    // Draw a screen to an eye buffer the same way it would be drawn as a
    // time warp overlay.
    void				drawScreenDirect( const GLuint texid, const ovrMatrix4f & mvp );

    //-----------------------------------------------------------------
    // debugging
    //-----------------------------------------------------------------
    void				setShowFPS( bool const show );
    bool				showFPS() const;

    void				showInfoText( float const duration, const char * fmt, ... );
    void				showInfoText( float const duration, Vector3f const & offset, Vector4f const & color, const char * fmt, ... );

    //-----------------------------------------------------------------

    Matrix4f		 matrixInterpolation( const Matrix4f & startMatrix, const Matrix4f & endMatrix, double t );

    // Called by jni functions
    void			keyEvent( const int keyCode, const bool down, const int keyCount );

    void			drawDialog( const Matrix4f & mvp );
    void			drawPanel( const GLuint externalTextureId, const Matrix4f & dialogMvp, const float alpha );

    void			drawBounds( const Vector3f &mins, const Vector3f &maxs, const Matrix4f &mvp, const Vector3f &color );

    // Shut down various subsystems
    void			pause();

    // Restart various subsusyems, possibly with new parameters
    // Called on surface creation and on resume from pause
    void			resume();

    // Start, stop and sync the VrThread.
    void			startVrThread();
    void			stopVrThread();
    void			syncVrThread();

    // TalkToJavaInterface
    void TtjCommand(JNIEnv *jni, const char * commandString) override;

    jclass			getGlobalClassReference( const char * className ) const;

    volatile bool	oneTimeInitCalled;
    ovrModeParms	VrModeParms;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(App)
};

extern App *vApp;

NV_NAMESPACE_END
