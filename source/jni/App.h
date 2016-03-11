#pragma once

#include "vglobal.h"

#include <pthread.h>
#include "OVR.h"
#include "android/GlUtils.h"
#include "android/LogUtils.h"
#include "api/VrApi.h"
#include "api/VrApi_Android.h"
#include "GlProgram.h"
#include "GlTexture.h"
#include "GlGeometry.h"
#include "SurfaceTexture.h"
#include "EyeBuffers.h"
#include "EyePostRender.h"
#include "VrCommon.h"
#include "VMessageQueue.h"
#include "Input.h"
#include "TalkToJava.h"
#include "KeyState.h"
#include "SoundManager.h"
#include "GlSetup.h"
#include "PointTracker.h"

// Avoid including this header file as much as possible in VrLib,
// so individual components are not tied to our native application
// framework, and can more easily be reused by Unity or other
// hosting applications.

NV_NAMESPACE_BEGIN

//==============================================================
// forward declarations
class EyeBuffers;
struct MaterialParms;
class GlGeometry;
struct GlProgram;
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
class OvrSoundManager;

//==============================================================
// All of these virtual interfaces will be called by the VR application thread.
// Applications that don't care about particular interfaces are not
// required to implement them.
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
    virtual void OneTimeInit( const char * fromPackage, const char * launchIntentJSON, const char * launchIntentURI ) = 0;

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
                                App(JNIEnv & jni_, jobject activityObject,
                                        VrAppInterface & interface );
    virtual						~App();

    VMessageQueue &		GetMessageQueue();

    VrAppInterface *	GetAppInterface();

    void 				DrawEyeViewsPostDistorted( Matrix4f const & viewMatrix, const int numPresents = 1);

    void				CreateToast( const char * fmt, ... );

    void				PlaySound( const char * name );

    void				StartSystemActivity( const char * command );

    void				RecenterYaw( const bool showBlack );
    void				SetRecenterYawFrameStart( const long long frameNumber );
    long long			GetRecenterYawFrameStart() const;


    //-----------------------------------------------------------------

    EyeParms			GetEyeParms();
    void				SetEyeParms( const EyeParms parms );

    //-----------------------------------------------------------------
    // interfaces
    //-----------------------------------------------------------------
    OvrGuiSys &         	GetGuiSys();
    OvrGazeCursor  &    	GetGazeCursor();
    BitmapFont &        	GetDefaultFont();
    BitmapFontSurface & 	GetWorldFontSurface();
    BitmapFontSurface & 	GetMenuFontSurface();
    OvrVRMenuMgr &      	GetVRMenuMgr();
    OvrDebugLines &     	GetDebugLines();
    const VStandardPath & GetStoragePaths();
    OvrSoundManager &		GetSoundMgr();

    //-----------------------------------------------------------------
    // system settings
    //-----------------------------------------------------------------
    int					GetSystemBrightness() const;
    void				SetSystemBrightness( int const v );

    bool				GetComfortModeEnabled() const;
    void				SetComfortModeEnabled( bool const enabled );

    int					GetWifiSignalLevel() const;
    eWifiState			GetWifiState() const;
    int					GetCellularSignalLevel() const;
    eCellularState		GetCellularState() const;

    bool				IsAirplaneModeEnabled() const;
    bool				IsTime24HourFormat() const;

    void				SetDoNotDisturbMode( bool const enable );
    bool				GetDoNotDisturbMode() const;

    bool				GetBluetoothEnabled() const;

    //-----------------------------------------------------------------
    // accessors
    //-----------------------------------------------------------------

    bool				IsAsynchronousTimeWarp() const;
    bool				GetHasHeadphones() const;
    bool				GetFramebufferIsSrgb() const;
    bool				GetFramebufferIsProtected() const;
    bool				GetRenderMonoMode() const;
    void				SetRenderMonoMode( bool const mono );

    const VString &packageCodePath() const;

    Matrix4f const &	GetLastViewMatrix() const;
    void				SetLastViewMatrix( Matrix4f const & m );

    EyeParms &			GetVrParms();
    ovrModeParms 		GetVrModeParms();
    void				SetVrModeParms( ovrModeParms parms );

    VrViewParms const &	GetVrViewParms() const;
    void				SetVrViewParms( VrViewParms const & parms );

    void				SetPopupDistance( float const d );
    float				GetPopupDistance() const;
    void				SetPopupScale( float const s );
    float				GetPopupScale() const;

    int					GetCpuLevel() const;
    int					GetGpuLevel() const;

    bool				GetPowerSaveActive() const;

    int					GetBatteryLevel() const;
    eBatteryStatus		GetBatteryStatus() const;

    bool				IsGuiOpen() const;

    KeyState &          GetBackKeyState();

    ovrMobile *			GetOvrMobile();

    void				SetShowVolumePopup( bool const show );
    bool				GetShowVolumePopup() const;

    const VString &packageName() const;

    bool				IsWifiConnected() const;

    //-----------------------------------------------------------------
    // Java accessors
    //-----------------------------------------------------------------

    JavaVM	*			GetJavaVM();
    JNIEnv	*			GetUiJni();
    JNIEnv	*			GetVrJni();
    jobject	&			GetJavaObject();
    jclass	&			GetVrActivityClass();

    //-----------------------------------------------------------------

    // Every application gets a basic dialog surface.
    SurfaceTexture *	GetDialogTexture();

    //-----------------------------------------------------------------
    // TimeWarp
    //-----------------------------------------------------------------
    ovrTimeWarpParms const & GetSwapParms() const;
    ovrTimeWarpParms &		GetSwapParms();

    ovrSensorState const & GetSensorForNextWarp() const;

    // Draw a zero to destination alpha
    void				DrawScreenMask( const ovrMatrix4f & mvp, const float fadeFracX, const float fadeFracY );
    // Draw a screen to an eye buffer the same way it would be drawn as a
    // time warp overlay.
    void				DrawScreenDirect( const GLuint texid, const ovrMatrix4f & mvp );

    //-----------------------------------------------------------------
    // debugging
    //-----------------------------------------------------------------
    void				SetShowFPS( bool const show );
    bool				GetShowFPS() const;

    void				ShowInfoText( float const duration, const char * fmt, ... );
    void				ShowInfoText( float const duration, Vector3f const & offset, Vector4f const & color, const char * fmt, ... );

    //-----------------------------------------------------------------

    // Read a file from the apk zip file.  Free buffer with free() when done.
    // Files put in the eclipse res/raw directory will be found as "res/raw/<NAME>"
    // Files put in the eclipse assets directory will be found as "assets/<name>"
    // The filename comparison is case insensitive.
    void 			ReadFileFromApplicationPackage(const char * nameInZip, uint &length, void * & buffer );

    //-----------------------------------------------------------------

    Matrix4f		 MatrixInterpolation( const Matrix4f & startMatrix, const Matrix4f & endMatrix, double t );

    // Handle development input options
    void 			FrameworkButtonProcessing( const VrInput & input );

    // Called by jni functions
    void			KeyEvent( const int keyCode, const bool down, const int keyCount );

    // One time init of GL objects.
    void			InitGlObjects();
    void			ShutdownGlObjects();

    void			DrawDialog( const Matrix4f & mvp );
    void			DrawPanel( const GLuint externalTextureId, const Matrix4f & dialogMvp, const float alpha );

    void			DrawBounds( const Vector3f &mins, const Vector3f &maxs, const Matrix4f &mvp, const Vector3f &color );

    static void *	ThreadStarter( void * parm );
    void 			VrThreadFunction();

    // Process commands forwarded from other threads.
    // Commands can be processed even when the window surfaces
    // are not setup.
    //
    // The msg string will be freed by the framework after
    // command processing.
    void    		Command( const char * msg );

    // Shut down various subsystems
    void			Pause();

    // Restart various subsusyems, possibly with new parameters
    // Called on surface creation and on resume from pause
    void			Resume();

    // Start, stop and sync the VrThread.
    void			StartVrThread();
    void			StopVrThread();
    void			SyncVrThread();

    // Open the apk on startup for resource loading
    void			OpenApplicationPackage();

    void			InterpretTouchpad( VrInput & input );

    jmethodID 		GetMethodID( const char * name, const char * signature ) const;
    jmethodID		GetMethodID( jclass clazz, const char * name, const char * signature ) const;
    jmethodID		GetStaticMethodID( jclass cls, const char * name, const char * signature ) const;

    // TalkToJavaInterface
    void TtjCommand(JNIEnv *jni, const char * commandString) override;

    jclass			GetGlobalClassReference( const char * className ) const;

    VString			GetInstalledPackagePath( const char * packageName ) const;

    bool			exitOnDestroy;
    volatile bool	oneTimeInitCalled;
    jobject			javaObject;
    VrAppInterface * appInterface;
    ovrModeParms	VrModeParms;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(App)
};

extern App *vApp;

NV_NAMESPACE_END
