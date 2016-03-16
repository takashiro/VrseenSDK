#include "App.h"

#include <android/keycodes.h>
#include <math.h>
#include <jni.h>
#include <sstream>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include <3rdparty/stb/stb_image_write.h>

#include "android/GlUtils.h"
#include "android/JniUtils.h"
#include "android/VOsBuild.h"

#include "Alg.h"
#include "BitmapFont.h"
#include "Console.h"
#include "DebugLines.h"
#include "EyePostRender.h"
#include "GazeCursor.h"
#include "GazeCursorLocal.h"		// necessary to instantiate the gaze cursor
#include "GlSetup.h"
#include "GlTexture.h"
#include "GuiSys.h"
#include "GuiSysLocal.h"		// necessary to instantiate the gui system
#include "LocalPreferences.h"		// FIXME:VRAPI move to VrApi_Android.h?
#include "ModelView.h"
#include "PointTracker.h"
#include "SurfaceTexture.h"
#include "System.h"
#include "TypesafeNumber.h"
#include "VMath.h"
#include "VolumePopup.h"
#include "VrApi.h"
#include "VrApi_Android.h"
#include "VrApi_Helpers.h"
#include "VrCommon.h"
#include "VrLocale.h"
#include "VRMenuMgr.h"

#include "VApkFile.h"
#include "VLog.h"
#include "VMainActivity.h"
#include "VJson.h"
#include "VUserProfile.h"

//#define TEST_TIMEWARP_WATCHDOG

NV_NAMESPACE_BEGIN

static const char * activityClassName = "com/vrseen/nervgear/VrActivity";
static const char * vrLibClassName = "com/vrseen/nervgear/VrLib";

// some parameters from the intent can be empty strings, which cannot be represented as empty strings for sscanf
// so we encode them as EMPTY_INTENT_STR.
// Because the message queue handling uses sscanf() to parse the message, the JSON text is
// always placed at the end of the message because it can contain spaces while the package
// name and URI cannot. The handler will use sscanf() to parse the first two strings, then
// assume the JSON text is everything immediately following the space after the URI string.
static const char * EMPTY_INTENT_STR = "<EMPTY>";

//@to-do: remove this
static VString ComposeIntentMessage(const VString &packageName, const VString &uri, const VString &jsonText)
{
    VString out = "intent ";
    out.append(packageName);
    out.append(' ');
    out.append(uri);
    out.append(' ');
    out.append(jsonText);
    return out;
}

//=======================================================================================
// Default handlers for VrAppInterface

VrAppInterface::VrAppInterface() :
    app(nullptr),
    ActivityClass(nullptr)
{
}

VrAppInterface::~VrAppInterface()
{
    if (ActivityClass != nullptr)
	{
		// FIXME:
        //jni->DeleteGlobalRef(ActivityClass);
        //ActivityClass = nullptr;
	}
}

jlong VrAppInterface::SetActivity(JNIEnv * jni, jclass clazz, jobject activity, jstring javaFromPackageNameString,
        jstring javaCommandString, jstring javaUriString)
{
	// Make a permanent global reference for the class
    if (ActivityClass != nullptr)
	{
        jni->DeleteGlobalRef(ActivityClass);
	}
    ActivityClass = (jclass)jni->NewGlobalRef(clazz);

    VString utfFromPackageString = JniUtils::Convert(jni, javaFromPackageNameString);
    VString utfJsonString = JniUtils::Convert(jni, javaCommandString);
    VString utfUriString = JniUtils::Convert(jni, javaUriString);
    vInfo("VrAppInterface::SetActivity:" << utfFromPackageString << utfJsonString << utfUriString);

    if (app == nullptr)
	{	// First time initialization
		// This will set the VrAppInterface app pointer directly,
		// so it is set when OneTimeInit is called.
        LOG("new AppLocal(%p %p %p)", jni, activity, this);
        new App(jni, activity, *this);

		// Start the VrThread and wait for it to have initialized.
        app->startVrThread();
        app->syncVrThread();
	}
	else
	{	// Just update the activity object.
        LOG("Update AppLocal(%p %p %p)", jni, activity, this);
        if (app->javaObject() != nullptr)
		{
            jni->DeleteGlobalRef(app->javaObject());
		}
        app->javaObject() = jni->NewGlobalRef(activity);
        app->VrModeParms.ActivityObject = app->javaObject();
	}

	// Send the intent and wait for it to complete.
    VString intentMessage = ComposeIntentMessage(utfFromPackageString, utfUriString, utfJsonString);
    VByteArray utf8Intent = intentMessage.toUtf8();
    app->messageQueue().PostPrintf(utf8Intent.data());
    app->syncVrThread();

	return (jlong)app;
}

void VrAppInterface::OneTimeShutdown()
{
}

void VrAppInterface::WindowCreated()
{
    LOG("VrAppInterface::WindowCreated - default handler called");
}

void VrAppInterface::WindowDestroyed()
{
    LOG("VrAppInterface::WindowDestroyed - default handler called");
}

void VrAppInterface::Paused()
{
    LOG("VrAppInterface::Paused - default handler called");
}

void VrAppInterface::Resumed()
{
    LOG("VrAppInterface::Resumed - default handler called");
}

void VrAppInterface::Command(const char * msg)
{
    LOG("VrAppInterface::Command - default handler called, msg = '%s'", msg);
}

void VrAppInterface::NewIntent(const char * fromPackageName, const char * command, const char * uri)
{
    LOG("VrAppInterface::NewIntent - default handler called - %s %s %s", fromPackageName, command, uri);
}

Matrix4f VrAppInterface::Frame(VrFrame vrFrame)
{
    LOG("VrAppInterface::Frame - default handler called");
	return Matrix4f();
}

void VrAppInterface::ConfigureVrMode(ovrModeParms & modeParms)
{
    LOG("VrAppInterface::ConfigureVrMode - default handler called");
}

Matrix4f VrAppInterface::DrawEyeView(const int eye, const float fovDegrees)
{
    LOG("VrAppInterface::DrawEyeView - default handler called");
	return Matrix4f();
}

bool VrAppInterface::onKeyEvent(const int keyCode, const KeyState::eKeyEventType eventType)
{
    LOG("VrAppInterface::OnKeyEvent - default handler called");
	return false;
}

bool VrAppInterface::OnVrWarningDismissed(const bool accepted)
{
    LOG("VrAppInterface::OnVrWarningDismissed - default handler called");
	return false;
}

bool VrAppInterface::ShouldShowLoadingIcon() const
{
	return true;
}

bool VrAppInterface::wantSrgbFramebuffer() const
{
	return false;
}

bool VrAppInterface::GetWantProtectedFramebuffer() const
{
	return false;
}

//==============================
// WaitForDebuggerToAttach
//
// wait on the debugger... once it is attached, change waitForDebugger to false
void WaitForDebuggerToAttach()
{
	static volatile bool waitForDebugger = true;
    while (waitForDebugger)
	{
		// put your breakpoint on the usleep to wait
        usleep(100000);
	}
}

//=======================================================================================

extern void DebugMenuBounds(void * appPtr, const char * cmd);
extern void DebugMenuHierarchy(void * appPtr, const char * cmd);
extern void DebugMenuPoses(void * appPtr, const char * cmd);
extern void ShowFPS(void * appPtr, const char * cmd);


struct App::Private
{
    App *self;
    // Primary apps will exit(0) when they get an onDestroy() so we
    // never leave any cpu-sucking process running, but the platformUI
    // needs to just return to the primary activity.
    volatile bool	vrThreadSynced;
    volatile bool	createdSurface;
    volatile bool	readyToExit;		// start exit procedure

    // Most calls in from java should communicate through this.
    VMessageQueue	vrMessageQueue;

    // From EnterVrMode, used for WarpSwap and LeaveVrMode
    ovrMobile *		OvrMobile;

    // Egl context and surface for rendering
    eglSetup_t		eglr;

    // Handles creating, destroying, and re-configuring the buffers
    // for drawing the eye views, which might be in different texture
    // configurations for CPU warping, etc.
    EyeBuffers *	eyeTargets;

    GLuint			loadingIconTexId;

    JavaVM *		javaVM;

    JNIEnv *		uiJni;			// for use by the Java UI thread
    JNIEnv *		vrJni;			// for use by the VR thread

    jclass			vrActivityClass;		// must be looked up from main thread or FindClass() will fail
    jclass			vrLibClass;				// must be looked up from main thread or FindClass() will fail

    jmethodID		finishActivityMethodId;
    jmethodID		createVrToastMethodId;
    jmethodID		clearVrToastsMethodId;
    jmethodID		playSoundPoolSoundMethodId;
    jmethodID		gazeEventMethodId;

    VString			launchIntentURI;			// URI app was launched with
    VString			launchIntentJSON;			// extra JSON data app was launched with
    VString			launchIntentFromPackage;	// package that sent us the launch intent

    VString			packageCodePath;	// path to apk to open as zip and load resources
    VString			packageName;		// package name

    bool			paused;				// set/cleared by onPause / onResume

    float 			popupDistance;
    float 			popupScale;


    // Every application gets a basic dialog surface.
    SurfaceTexture * dialogTexture;

    // Current joypad state, without pressed / released calculation
    VrInput			joypad;

    // drawing parameters
    int				dialogWidth;
    int				dialogHeight;
    float			dialogStopSeconds;

    // Dialogs will be oriented base down in the view when they
    // were generated.
    Matrix4f		dialogMatrix;

    Matrix4f		lastViewMatrix;

    ANativeWindow * nativeWindow;
    EGLSurface 		windowSurface;

    bool			drawCalibrationLines;	// currently toggled by right trigger
    bool			calibrationLinesDrawn;	// after draw, go to static time warp test
    bool			showVignette;			// render the vignette

    ovrHmdInfo		hmdInfo;

    bool			framebufferIsSrgb;			// requires KHR_gl_colorspace
    bool			framebufferIsProtected;		// requires GPU trust zone extension

    // Only render a single eye view, which will get warped for both
    // screen eyes.
    bool			renderMonoMode;

    VrFrame			vrFrame;
    VrFrame			lastVrFrame;

    EyeParms		vrParms;

    ovrTimeWarpParms	swapParms;			// passed to TimeWarp->WarpSwap()

    GlProgram		externalTextureProgram2;
    GlProgram		untexturedMvpProgram;
    GlProgram		untexturedScreenSpaceProgram;
    GlProgram		overlayScreenFadeMaskProgram;
    GlProgram		overlayScreenDirectProgram;

    GlGeometry		unitCubeLines;		// 12 lines that outline a 0 to 1 unit cube, intended to be scaled to cover bounds.
    GlGeometry		panelGeometry;		// used for dialogs
    GlGeometry		unitSquare;			// -1 to 1 in x and Y, 0 to 1 in texcoords
    GlGeometry		fadedScreenMaskSquare;// faded screen mask for overlay rendering

    EyePostRender	eyeDecorations;

    ovrSensorState	sensorForNextWarp;

    pthread_t		vrThread;			// posix pthread
    int				vrThreadTid;		// linux tid

    // For running java commands on another thread to
    // avoid hitches.
    TalkToJava		ttj;

    int				batteryLevel;		// charge level of the batter as reported from Java
    eBatteryStatus	batteryStatus;		// battery status as reported from Java

    bool			showFPS;			// true to show FPS on screen
    bool			showVolumePopup;	// true to show volume popup when volume changes

    VrViewParms		viewParms;

    VString			infoText;			// informative text to show in front of the view
    Vector4f		infoTextColor;		// color of info text
    Vector3f		infoTextOffset;		// offset from center of screen in view space
    long long		infoTextEndFrame;	// time to stop showing text
    OvrPointTracker	infoTextPointTracker;	// smoothly tracks to text ideal location
    OvrPointTracker	fpsPointTracker;		// smoothly tracks to ideal FPS text location

    float 			touchpadTimer;
    Vector2f		touchOrigin;
    float 			lastTouchpadTime;
    bool 			lastTouchDown;
    int 			touchState;

    bool			enableDebugOptions;	// enable debug key-commands for development testing

    long long 		recenterYawFrameStart;	// Enables reorient before sensor data is read.  Allows apps to reorient without having invalid orientation information for that frame.

    // Manages sound assets
    OvrSoundManager	soundManager;

    OvrGuiSys *         guiSys;
    OvrGazeCursor *     gazeCursor;
    BitmapFont *        defaultFont;
    BitmapFontSurface * worldFontSurface;
    BitmapFontSurface * menuFontSurface;
    OvrVRMenuMgr *      vrMenuMgr;
    OvrVolumePopup *	volumePopup;
    OvrDebugLines *     debugLines;
    KeyState backKeyState;
    VStandardPath *storagePaths;

    GlTexture errorTexture;
    int errorTextureSize;
    double errorMessageEndTime;

    jobject javaObject;
    VrAppInterface *appInterface;

    VMainActivity *activity;

    Private(App *self)
        : self(self)
        , vrThreadSynced(false)
        , createdSurface(false)
        , readyToExit(false)
        , vrMessageQueue(100)
        , OvrMobile(nullptr)
        , eyeTargets(nullptr)
        , loadingIconTexId(0)
        , javaVM(VrLibJavaVM)
        , uiJni(nullptr)
        , vrJni(nullptr)
        , vrActivityClass(nullptr)
        , vrLibClass(nullptr)
        , finishActivityMethodId(nullptr)
        , createVrToastMethodId(nullptr)
        , clearVrToastsMethodId(nullptr)
        , playSoundPoolSoundMethodId(nullptr)
        , gazeEventMethodId(nullptr)
        , paused(true)
        , popupDistance(2.0f)
        , popupScale(1.0f)
        , dialogTexture(nullptr)
        , dialogWidth(0)
        , dialogHeight(0)
        , dialogStopSeconds(0.0f)
        , nativeWindow(nullptr)
        , windowSurface(EGL_NO_SURFACE)
        , drawCalibrationLines(false)
        , calibrationLinesDrawn(false)
        , showVignette(true)
        , framebufferIsSrgb(false)
        , framebufferIsProtected(false)
        , renderMonoMode(false)
        , vrThreadTid(0)
        , batteryLevel(0)
        , batteryStatus(BATTERY_STATUS_UNKNOWN)
        , showFPS(false)
        , showVolumePopup(true)
        , infoTextColor(1.0f)
        , infoTextOffset(0.0f)
        , infoTextEndFrame(-1)
        , touchpadTimer(0.0f)
        , lastTouchpadTime(0.0f)
        , lastTouchDown(false)
        , touchState(0)
        , enableDebugOptions(false)
        , recenterYawFrameStart(0)
        , guiSys(nullptr)
        , gazeCursor(nullptr)
        , defaultFont(nullptr)
        , worldFontSurface(nullptr)
        , menuFontSurface(nullptr)
        , vrMenuMgr(nullptr)
        , volumePopup(nullptr)
        , debugLines(nullptr)
        , backKeyState(0.25f, 0.75f)
        , storagePaths(nullptr)
        , errorTextureSize(0)
        , errorMessageEndTime(-1.0)
        , javaObject(nullptr)
        , appInterface(nullptr)
        , activity(nullptr)
    {
    }

    void initFonts()
    {
        defaultFont = BitmapFont::Create();

        VString fontName;
        VrLocale::GetString(vrJni, javaObject, "@string/font_name", "efigs.fnt", fontName);
        fontName.prepend("res/raw/");
        if (!defaultFont->Load(packageCodePath, fontName))
        {
            // reset the locale to english and try to load the font again
            jmethodID setDefaultLocaleId = vrJni->GetMethodID(vrActivityClass, "setDefaultLocale", "()V");
            if (setDefaultLocaleId != nullptr)
            {
                LOG("AppLocal::Init CallObjectMethod");
                vrJni->CallVoidMethod(javaObject, setDefaultLocaleId);
                if (vrJni->ExceptionOccurred())
                {
                    vrJni->ExceptionClear();
                    WARN("Exception occurred in setDefaultLocale");
                }
                // re-get the font name for the new locale
                VrLocale::GetString(vrJni, javaObject, "@string/font_name", "efigs.fnt", fontName);
                fontName.prepend("res/raw/");
                // try to load the font
                if (!defaultFont->Load(packageCodePath, fontName))
                {
                    FAIL("Failed to load font for default locale!");
                }
            }
        }

        worldFontSurface = BitmapFontSurface::Create();
        menuFontSurface = BitmapFontSurface::Create();

        worldFontSurface->Init(8192);
        menuFontSurface->Init(8192);
    }

    void shutdownFonts()
    {
        BitmapFont::Free(defaultFont);
        BitmapFontSurface::Free(worldFontSurface);
        BitmapFontSurface::Free(menuFontSurface);
    }

    // Error checks and exits on failure
    jmethodID GetMethodID(const char * name, const char *signature) const
    {
        jmethodID mid = uiJni->GetMethodID(vrActivityClass, name, signature);
        if (!mid) {
            vFatal("couldn't get" << name);
        }
        return mid;
    }

    jmethodID GetMethodID(jclass jClass, const char *name, const char *signature) const
    {
        jmethodID mid = uiJni->GetMethodID(jClass, name, signature);
        if (!mid) {
            vFatal("couldn't get" << name);
        }
        return mid;
    }

    jmethodID GetStaticMethodID(jclass jClass, const char *name, const char *signature) const
    {
        jmethodID mid = uiJni->GetStaticMethodID(jClass, name, signature);
        if (!mid) {
            vFatal("couldn't get" << name);
        }
        return mid;
    }
};

/*
 * AppLocal
 *
 * Called once at startup.
 *
 * ?still true?: exit() from here causes an immediate app re-launch,
 * move everything to first surface init?
 */

App *vApp = nullptr;

App::App(JNIEnv *jni, jobject activityObject, VrAppInterface &interface)
    : exitOnDestroy(true)
    , oneTimeInitCalled(false)
    , d(new Private(this))
{
    d->activity = new VMainActivity(jni, activityObject);

    d->uiJni = jni;
    vInfo("----------------- AppLocal::AppLocal() -----------------");
    vAssert(vApp == nullptr);
    vApp = this;

    d->storagePaths = new VStandardPath(jni, activityObject);

	//WaitForDebuggerToAttach();

    memset(& d->sensorForNextWarp, 0, sizeof(d->sensorForNextWarp));

    d->sensorForNextWarp.Predicted.Pose.Orientation = Quatf();

    JniUtils::LoadDevConfig(false);

	// Default time warp parms
    d->swapParms = InitTimeWarpParms();

	// Default EyeParms
    d->vrParms.resolution = 1024;
    d->vrParms.multisamples = 4;
    d->vrParms.colorFormat = COLOR_8888;
    d->vrParms.depthFormat = DEPTH_24;

	// Default ovrModeParms
	VrModeParms.AsynchronousTimeWarp = true;
	VrModeParms.AllowPowerSave = true;
    VrModeParms.DistortionFileName = nullptr;
	VrModeParms.EnableImageServer = false;
	VrModeParms.SkipWindowFullscreenReset = false;
	VrModeParms.CpuLevel = 2;
	VrModeParms.GpuLevel = 2;
	VrModeParms.GameThreadTid = 0;

    d->javaObject = d->uiJni->NewGlobalRef(activityObject);

	// A difficulty with JNI is that we can't resolve our (non-Android) package
	// classes on other threads, so lookup everything we need right now.
    d->vrActivityClass = getGlobalClassReference(activityClassName);
    d->vrLibClass = getGlobalClassReference(vrLibClassName);

    VrLocale::VrActivityClass = d->vrActivityClass;

    d->finishActivityMethodId = d->GetMethodID("finishActivity", "()V");
    d->createVrToastMethodId = d->GetMethodID("createVrToastOnUiThread", "(Ljava/lang/String;)V");
    d->clearVrToastsMethodId = d->GetMethodID("clearVrToasts", "()V");
    d->playSoundPoolSoundMethodId = d->GetMethodID("playSoundPoolSound", "(Ljava/lang/String;)V");

    jmethodID isHybridAppMethodId = d->GetStaticMethodID(d->vrLibClass, "isHybridApp", "(Landroid/app/Activity;)Z");
    bool const isHybridApp = jni->CallStaticBooleanMethod(d->vrLibClass, isHybridAppMethodId, d->javaObject);

    exitOnDestroy = !isHybridApp;

    d->gazeEventMethodId = d->GetStaticMethodID(d->vrActivityClass, "gazeEventFromNative", "(FFZZLandroid/app/Activity;)V");

	// Get the path to the .apk and package name
    openApplicationPackage();

	// Hook the App and AppInterface together
    d->appInterface = &interface;
    d->appInterface->app = this;

	// Load user profile data relevant to rendering
    VUserProfile profile;
    profile.load();
    d->viewParms.InterpupillaryDistance = profile.ipd;
    d->viewParms.EyeHeight = profile.eyeHeight;
    d->viewParms.HeadModelDepth = profile.headModelDepth;
    d->viewParms.HeadModelHeight = profile.headModelHeight;

	// Register console functions
	InitConsole();
    RegisterConsoleFunction("print", NervGear::DebugPrint);
    RegisterConsoleFunction("debugMenuBounds", NervGear::DebugMenuBounds);
    RegisterConsoleFunction("debugMenuHierarchy", NervGear::DebugMenuHierarchy);
    RegisterConsoleFunction("debugMenuPoses", NervGear::DebugMenuPoses);
    RegisterConsoleFunction("showFPS", NervGear::ShowFPS);
}

App::~App()
{
    LOG("---------- ~AppLocal() ----------");

	UnRegisterConsoleFunctions();
	ShutdownConsole();

    if (d->javaObject != 0)
	{
        d->uiJni->DeleteGlobalRef(d->javaObject);
	}

    if (d->storagePaths != nullptr)
	{
        delete d->storagePaths;
        d->storagePaths = nullptr;
	}

    delete d->activity;
    delete d;
}

void App::startVrThread()
{
    LOG("StartVrThread");

    const int createErr = pthread_create(&d->vrThread, nullptr /* default attributes */, &ThreadStarter, this);
    if (createErr != 0)
	{
        FAIL("pthread_create returned %i", createErr);
	}
}

void App::stopVrThread()
{
    LOG("StopVrThread");

    d->vrMessageQueue.PostPrintf("quit ");
    const int ret = pthread_join(d->vrThread, nullptr);
    if (ret != 0)
	{
        WARN("failed to join VrThread (%i)", ret);
	}
}

void App::syncVrThread()
{
    d->vrMessageQueue.SendPrintf("sync ");
    d->vrThreadSynced = true;
}

VMessageQueue & App::messageQueue()
{
    return d->vrMessageQueue;
}

// This callback happens from the java thread, after a string has been
// pulled off the message queue
void App::TtjCommand(JNIEnv *jni, const char * commandString)
{
    if (MatchesHead("sound ", commandString))
	{
        jstring cmdString = JniUtils::Convert(jni, commandString + 6);
        jni->CallVoidMethod(d->javaObject, d->playSoundPoolSoundMethodId, cmdString);
        jni->DeleteLocalRef(cmdString);
		return;
	}

    if (MatchesHead("toast ", commandString))
	{
        jstring cmdString = JniUtils::Convert(jni, commandString + 6);
        jni->CallVoidMethod(d->javaObject, d->createVrToastMethodId, cmdString);
        jni->DeleteLocalRef(cmdString);
	    return;
	}

    if (MatchesHead("finish ", commandString)) {
        jni->CallVoidMethod(d->javaObject, d->finishActivityMethodId);
	}
}

void App::createToast(const char * fmt, ...)
{
	char bigBuffer[4096];
	va_list	args;
    va_start(args, fmt);
    vsnprintf(bigBuffer, sizeof(bigBuffer), fmt, args);
    va_end(args);

	LOG("CreateToast %s", bigBuffer);

    d->ttj.GetMessageQueue().PostPrintf("toast %s", bigBuffer);
}

void App::playSound(const char * name)
{
	// Get sound from SoundManager
	VString soundFile;

    if (d->soundManager.GetSound(name, soundFile))
	{
		// Run on the talk to java thread
        d->ttj.GetMessageQueue().PostPrintf("sound %s", soundFile.toCString());
	}
	else
	{
        WARN("AppLocal::PlaySound called with non SoundManager defined sound: %s", name);
		// Run on the talk to java thread
        d->ttj.GetMessageQueue().PostPrintf("sound %s", name);
	}
}

void App::readFileFromApplicationPackage(const char * nameInZip, uint &length, void * & buffer)
{
    const VApkFile &apk = VApkFile::CurrentApkFile();
    apk.read(nameInZip, buffer, length);
}

void App::openApplicationPackage()
{
    d->packageCodePath = JniUtils::GetPackageCodePath(d->uiJni, d->vrActivityClass, d->javaObject);
    d->packageName = JniUtils::GetCurrentPackageName(d->uiJni, d->javaObject);
}

VString App::getInstalledPackagePath(const char * packageName) const
{
    jmethodID getInstalledPackagePathId = d->GetMethodID("getInstalledPackagePath", "(Ljava/lang/String;)Ljava/lang/String;");
    if (getInstalledPackagePathId != nullptr)
	{
        JavaString packageNameObj(d->uiJni, packageName);
        VString resultStr = JniUtils::Convert(d->uiJni, static_cast< jstring >(d->uiJni->CallObjectMethod(d->javaObject, getInstalledPackagePathId, packageNameObj.toJString())));
        if (!d->uiJni->ExceptionOccurred()) {
            return resultStr;
		}
	}
	return VString();
}

jclass App::getGlobalClassReference(const char * className) const
{
    jclass lc = d->uiJni->FindClass(className);
    if (lc == 0)
	{
        FAIL("FindClass(%s) failed", className);
	}
	// Turn it into a global ref, so we can safely use it in the VR thread
    jclass gc = (jclass)d->uiJni->NewGlobalRef(lc);

    d->uiJni->DeleteLocalRef(lc);

	return gc;
}

static EyeParms DefaultVrParmsForRenderer(const eglSetup_t & eglr)
{
	EyeParms vrParms;

	vrParms.resolution = 1024;
    vrParms.multisamples = (eglr.gpuType == GPU_TYPE_ADRENO_330) ? 2 : 4;
	vrParms.colorFormat = COLOR_8888;
	vrParms.depthFormat = DEPTH_24;

	return vrParms;
}

static bool ChromaticAberrationCorrection(const eglSetup_t & eglr)
{
    return (eglr.gpuType & GPU_TYPE_ADRENO) != 0 && (eglr.gpuType >= GPU_TYPE_ADRENO_420);
}

static const char* vertexShaderSource =
		"uniform mat4 Mvpm;\n"
		"uniform mat4 Texm;\n"
		"attribute vec4 Position;\n"
		"attribute vec4 VertexColor;\n"
		"attribute vec2 TexCoord;\n"
		"uniform mediump vec4 UniformColor;\n"
		"varying  highp vec2 oTexCoord;\n"
		"varying  lowp vec4 oColor;\n"
		"void main()\n"
		"{\n"
		"   gl_Position = Mvpm * Position;\n"
        "   oTexCoord = vec2(Texm * vec4(TexCoord,1,1));\n"
		"   oColor = VertexColor * UniformColor;\n"
		"}\n";


/*
 * InitGlObjects
 *
 * Call once a GL context is created, either by us or a host engine.
 * The Java VM must be attached to this thread to allow SurfaceTexture
 * creation.
 */
void App::initGlObjects()
{
    d->vrParms = DefaultVrParmsForRenderer(d->eglr);

    d->swapParms.WarpProgram = ChromaticAberrationCorrection(d->eglr) ? WP_CHROMATIC : WP_SIMPLE;

	// Let glUtils look up extensions
	GL_FindExtensions();

    d->externalTextureProgram2 = BuildProgram(vertexShaderSource, externalFragmentShaderSource);
    d->untexturedMvpProgram = BuildProgram(
		"uniform mat4 Mvpm;\n"
		"attribute vec4 Position;\n"
		"uniform mediump vec4 UniformColor;\n"
		"varying  lowp vec4 oColor;\n"
		"void main()\n"
		"{\n"
			"   gl_Position = Mvpm * Position;\n"
			"   oColor = UniformColor;\n"
		"}\n"
	,
		"varying lowp vec4	oColor;\n"
		"void main()\n"
		"{\n"
		"	gl_FragColor = oColor;\n"
		"}\n"
    );
    d->untexturedScreenSpaceProgram = BuildProgram(identityVertexShaderSource, untexturedFragmentShaderSource);
    d->overlayScreenFadeMaskProgram = BuildProgram(
			"uniform mat4 Mvpm;\n"
			"attribute vec4 VertexColor;\n"
			"attribute vec4 Position;\n"
			"varying  lowp vec4 oColor;\n"
			"void main()\n"
			"{\n"
			"   gl_Position = Mvpm * Position;\n"
            "   oColor = vec4(1.0, 1.0, 1.0, 1.0 - VertexColor.x);\n"
			"}\n"
		,
			"varying lowp vec4	oColor;\n"
			"void main()\n"
			"{\n"
			"	gl_FragColor = oColor;\n"
			"}\n"
		);
    d->overlayScreenDirectProgram = BuildProgram(
			"uniform mat4 Mvpm;\n"
			"attribute vec4 Position;\n"
			"attribute vec2 TexCoord;\n"
			"varying  highp vec2 oTexCoord;\n"
			"void main()\n"
			"{\n"
			"   gl_Position = Mvpm * Position;\n"
			"   oTexCoord = TexCoord;\n"
			"}\n"
		,
			"uniform sampler2D Texture0;\n"
			"varying highp vec2 oTexCoord;\n"
			"void main()\n"
			"{\n"
            "	gl_FragColor = texture2D(Texture0, oTexCoord);\n"
			"}\n"
		);

	// Build some geometries we need
    d->panelGeometry = BuildTesselatedQuad(32, 16);	// must be large to get faded edge
    d->unitSquare = BuildTesselatedQuad(1, 1);
    d->unitCubeLines = BuildUnitCubeLines();
    //FadedScreenMaskSquare = BuildFadedScreenMask(0.0f, 0.0f);	// TODO: clean up: app-specific values are being passed in on DrawScreenMask

    d->eyeDecorations.Init();
}

void App::ShutdownGlObjects()
{
    DeleteProgram(d->externalTextureProgram2);
    DeleteProgram(d->untexturedMvpProgram);
    DeleteProgram(d->untexturedScreenSpaceProgram);
    DeleteProgram(d->overlayScreenFadeMaskProgram);
    DeleteProgram(d->overlayScreenDirectProgram);

    d->panelGeometry.Free();
    d->unitSquare.Free();
    d->unitCubeLines.Free();
    d->fadedScreenMaskSquare.Free();

    d->eyeDecorations.Shutdown();
}

Vector3f ViewOrigin(const Matrix4f & view)
{
    return Vector3f(view.M[0][3], view.M[1][3], view.M[2][3]);
}

Vector3f ViewForward(const Matrix4f & view)
{
    return Vector3f(-view.M[0][2], -view.M[1][2], -view.M[2][2]);
}

Vector3f ViewUp(const Matrix4f & view)
{
    return Vector3f(view.M[0][1], view.M[1][1], view.M[2][1]);
}

Vector3f ViewRight(const Matrix4f & view)
{
    return Vector3f(view.M[0][0], view.M[1][0], view.M[2][0]);
}

void App::setVrModeParms(ovrModeParms parms)
{
    if (d->OvrMobile)
	{
        ovr_LeaveVrMode(d->OvrMobile);
		VrModeParms = parms;
        d->OvrMobile = ovr_EnterVrMode(VrModeParms, &d->hmdInfo);
	}
	else
	{
		VrModeParms = parms;
	}
}

void App::pause()
{
    d->appInterface->Paused();

    ovr_LeaveVrMode(d->OvrMobile);
}

/*
 * On startup, the resume message happens before our window is created, so
 * it needs to be deferred until after the create so we know the window
 * dimensions for HmdInfo.
 *
 * On pressing the power button, pause/resume happens without destroying
 * the window.
 */
void App::resume()
{
    DROIDLOG("OVRTimer", "AppLocal::Resume");

	// always reload the dev config on a resume
    JniUtils::LoadDevConfig(true);

	// Make sure the window surface is current, which it won't be
	// if we were previously in async mode
	// (Not needed now?)
    if (eglMakeCurrent(d->eglr.display, d->windowSurface, d->windowSurface, d->eglr.context) == EGL_FALSE)
	{
        FAIL("eglMakeCurrent failed: %s", EglErrorString());
	}

    VrModeParms.ActivityObject = d->javaObject;

	// Allow the app to override
    d->appInterface->ConfigureVrMode(VrModeParms);

	// Reload local preferences, in case we are coming back from a
	// switch to the dashboard that changed them.
	ovr_UpdateLocalPreferences();

	// Check for values that effect our mode settings
	{
        const char * imageServerStr = ovr_GetLocalPreferenceValueForKey(LOCAL_PREF_IMAGE_SERVER, "0");
        VrModeParms.EnableImageServer = (atoi(imageServerStr) > 0);

        const char * cpuLevelStr = ovr_GetLocalPreferenceValueForKey(LOCAL_PREF_DEV_CPU_LEVEL, "-1");
        const int cpuLevel = atoi(cpuLevelStr);
        if (cpuLevel >= 0)
		{
			VrModeParms.CpuLevel = cpuLevel;
            LOG("Local Preferences: Setting cpuLevel %d", VrModeParms.CpuLevel);
		}
        const char * gpuLevelStr = ovr_GetLocalPreferenceValueForKey(LOCAL_PREF_DEV_GPU_LEVEL, "-1");
        const int gpuLevel = atoi(gpuLevelStr);
        if (gpuLevel >= 0)
		{
			VrModeParms.GpuLevel = gpuLevel;
            LOG("Local Preferences: Setting gpuLevel %d", VrModeParms.GpuLevel);
		}

        const char * showVignetteStr = ovr_GetLocalPreferenceValueForKey(LOCAL_PREF_DEV_SHOW_VIGNETTE, "1");
        d->showVignette = (atoi(showVignetteStr) > 0);

        const char * enableDebugOptionsStr = ovr_GetLocalPreferenceValueForKey(LOCAL_PREF_DEV_DEBUG_OPTIONS, "0");
        d->enableDebugOptions =  (atoi(enableDebugOptionsStr) > 0);

        const char * enableGpuTimingsStr = ovr_GetLocalPreferenceValueForKey(LOCAL_PREF_DEV_GPU_TIMINGS, "0");
        SetAllowGpuTimerQueries(atoi(enableGpuTimingsStr) > 0);
	}

	// Clear cursor trails
    gazeCursor().HideCursorForFrames(10);

	// Start up TimeWarp and the various performance options
    d->OvrMobile = ovr_EnterVrMode(VrModeParms, &d->hmdInfo);

    d->appInterface->Resumed();
}

// Always make the panel upright, even if the head was tilted when created
Matrix4f PanelMatrix(const Matrix4f & lastViewMatrix, const float popupDistance,
        const float popupScale, const int width, const int height)
{
	// TODO: this won't be valid until a frame has been rendered
	const Matrix4f invView = lastViewMatrix.Inverted();
    const Vector3f forward = ViewForward(invView);
    const Vector3f levelforward = Vector3f(forward.x, 0.0f, forward.z).Normalized();
	// TODO: check degenerate case
    const Vector3f up(0.0f, 1.0f, 0.0f);
    const Vector3f right = levelforward.Cross(up);

    const Vector3f center = ViewOrigin(invView) + levelforward * popupDistance;
	const float xScale = (float)width / 768.0f * popupScale;
	const float yScale = (float)height / 768.0f * popupScale;
	const Matrix4f panelMatrix = Matrix4f(
			xScale * right.x, yScale * up.x, forward.x, center.x,
			xScale * right.y, yScale * up.y, forward.y, center.y,
			xScale * right.z, yScale * up.z, forward.z, center.z,
            0, 0, 0, 1);

//	LOG("PanelMatrix center: %f %f %f", center.x, center.y, center.z);
//	LogMatrix("PanelMatrix", panelMatrix);

	return panelMatrix;
}

/*
 * Command
 *
 * Process commands sent over the message queue for the VR thread.
 *
 */
void App::command(const char *msg)
{
	// Always include the space in MatchesHead to prevent problems
	// with commands that have matching prefixes.

    if (MatchesHead("joy ", msg))
	{
        sscanf(msg, "joy %f %f %f %f",
                &d->joypad.sticks[0][0],
                &d->joypad.sticks[0][1],
                &d->joypad.sticks[1][0],
                &d->joypad.sticks[1][1]);
		return;
	}

    if (MatchesHead("touch ", msg))
	{
		int	action;
        sscanf(msg, "touch %i %f %f",
				&action,
                &d->joypad.touch[0],
                &d->joypad.touch[1]);
        if (action == 0)
		{
            d->joypad.buttonState |= BUTTON_TOUCH;
		}
        if (action == 1)
		{
            d->joypad.buttonState &= ~BUTTON_TOUCH;
		}
		return;
	}

    if (MatchesHead("key ", msg))
	{
		int	key, down, repeatCount;
        sscanf(msg, "key %i %i %i", &key, &down, &repeatCount);
        keyEvent(key, down, repeatCount);
		// We simply return because KeyEvent will call VrAppInterface->OnKeyEvent to give the app a
		// chance to handle and consume the key before VrLib gets it. VrAppInterface needs to get the
		// key first and have a chance to consume it completely because keys are context sensitive
		// and only the app interface can know the current context the key should apply to. For
		// instance, the back key backs out of some current state in the app -- but only the app knows
		// whether or not there is a state to back out of at all. If we were to fall through here, the
		// "key " message will be sent to VrAppInterface->Command() but only after VrLib has handled
		// and or consumed the key first, which would break the back key behavior, and probably anything
		// else, like input into an edit control.
		return;
	}

    if (MatchesHead("surfaceChanged ", msg))
	{
        LOG("%s", msg);
        if (d->windowSurface != EGL_NO_SURFACE)
		{	// Samsung says this is an Android problem, where surfaces are reported as
			// created multiple times.
            WARN("Skipping create work because window hasn't been destroyed.");
			return;
		}
        sscanf(msg, "surfaceChanged %p", &d->nativeWindow);

		// Optionally force the window to a different resolution, which
		// will be automatically scaled up by the HWComposer.
		//
        //ANativeWindow_setBuffersGeometry(nativeWindow, 1920, 1080, 0);

		EGLint attribs[100];
		int		numAttribs = 0;

		// Set the colorspace on the window
        d->windowSurface = EGL_NO_SURFACE;
        if (d->appInterface->wantSrgbFramebuffer())
		{
			attribs[numAttribs++] = EGL_GL_COLORSPACE_KHR;
			attribs[numAttribs++] = EGL_GL_COLORSPACE_SRGB_KHR;
		}
		// Ask for TrustZone rendering support
        if (d->appInterface->GetWantProtectedFramebuffer())
		{
			attribs[numAttribs++] = EGL_PROTECTED_CONTENT_EXT;
			attribs[numAttribs++] = EGL_TRUE;
		}
		attribs[numAttribs++] = EGL_NONE;

		// Android doesn't let the non-standard extensions show up in the
		// extension string, so we need to try it blind.
        d->windowSurface = eglCreateWindowSurface(d->eglr.display, d->eglr.config,
                d->nativeWindow, attribs);

        if (d->windowSurface == EGL_NO_SURFACE)
		{
			const EGLint attribs2[] =
			{
				EGL_NONE
			};
            d->windowSurface = eglCreateWindowSurface(d->eglr.display, d->eglr.config,
                    d->nativeWindow, attribs2);
            if (d->windowSurface == EGL_NO_SURFACE)
			{
                FAIL("eglCreateWindowSurface failed: %s", EglErrorString());
			}
            d->framebufferIsSrgb = false;
            d->framebufferIsProtected = false;
		}
		else
		{
            d->framebufferIsSrgb = d->appInterface->wantSrgbFramebuffer();
            d->framebufferIsProtected = d->appInterface->GetWantProtectedFramebuffer();
		}
        LOG("NativeWindow %p gives surface %p", d->nativeWindow, d->windowSurface);
        LOG("FramebufferIsSrgb: %s", d->framebufferIsSrgb ? "true" : "false");
        LOG("FramebufferIsProtected: %s", d->framebufferIsProtected ? "true" : "false");

        if (eglMakeCurrent(d->eglr.display, d->windowSurface, d->windowSurface, d->eglr.context) == EGL_FALSE)
		{
            FAIL("eglMakeCurrent failed: %s", EglErrorString());
		}

        d->createdSurface = true;

		// Let the client app setup now
        d->appInterface->WindowCreated();

		// Resume
        if (!d->paused)
		{
            resume();
		}
		return;
	}

    if (MatchesHead("surfaceDestroyed ", msg))
	{
        LOG("surfaceDestroyed");

		// Let the client app shutdown first.
        d->appInterface->WindowDestroyed();

		// Handle it ourselves.
        if (eglMakeCurrent(d->eglr.display, d->eglr.pbufferSurface, d->eglr.pbufferSurface,
                d->eglr.context) == EGL_FALSE)
		{
            FAIL("RC_SURFACE_DESTROYED: eglMakeCurrent pbuffer failed");
		}

        if (d->windowSurface != EGL_NO_SURFACE)
		{
            eglDestroySurface(d->eglr.display, d->windowSurface);
            d->windowSurface = EGL_NO_SURFACE;
		}
        if (d->nativeWindow != nullptr)
		{
            ANativeWindow_release(d->nativeWindow);
            d->nativeWindow = nullptr;
		}
		return;
	}

    if (MatchesHead("pause ", msg))
	{
        LOG("pause");
        if (!d->paused)
		{
            d->paused = true;
            pause();
		}
	}

    if (MatchesHead("resume ", msg))
	{
        LOG("resume");
        d->paused = false;
		// Don't actually do the resume operations if we don't have
		// a window yet.  They will be done when the window is created.
        if (d->windowSurface != EGL_NO_SURFACE)
		{
            resume();
		}
		else
		{
            LOG("Skipping resume because windowSurface not set yet");
		}
	}

    if (MatchesHead("intent ", msg))
	{
		char fromPackageName[512];
		char uri[1024];
		// since the package name and URI cannot contain spaces, but JSON can,
		// the JSON string is at the end and will come after the third space.
        sscanf(msg, "intent %s %s", fromPackageName, uri);
        char const * jsonStart = nullptr;
        size_t msgLen = strlen(msg);
		int spaceCount = 0;
        for (size_t i = 0; i < msgLen; ++i) {
            if (msg[i] == ' ') {
				spaceCount++;
                if (spaceCount == 3) {
					jsonStart = &msg[i+1];
					break;
				}
			}
		}

        if (strcmp(fromPackageName, EMPTY_INTENT_STR) == 0)
		{
			fromPackageName[0] = '\0';
		}
        if (strcmp(uri, EMPTY_INTENT_STR) == 0)
		{
			uri[0] = '\0';
		}

		// assign launchIntent to the intent command
        d->launchIntentFromPackage = fromPackageName;
        d->launchIntentJSON = jsonStart;
        d->launchIntentURI = uri;

		// when the PlatformActivity is launched, this is how it gets its command to start
		// a particular UI.
        d->appInterface->NewIntent(fromPackageName, jsonStart, uri);

		return;
	}

    if (MatchesHead("popup ", msg))
	{
		int width, height;
		float seconds;
        sscanf(msg, "popup %i %i %f", &width, &height, &seconds);

        d->dialogWidth = width;
        d->dialogHeight = height;
        d->dialogStopSeconds = ovr_GetTimeInSeconds() + seconds;

        d->dialogMatrix = PanelMatrix(d->lastViewMatrix, d->popupDistance, d->popupScale, width, height);

        glActiveTexture(GL_TEXTURE0);
        LOG("RC_UPDATE_POPUP dialogTexture %i", d->dialogTexture->textureId);
        d->dialogTexture->Update();
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

		return;
	}

    if (MatchesHead("sync ", msg))
	{
		return;
	}

    if (MatchesHead("quit ", msg))
	{
        ovr_LeaveVrMode(d->OvrMobile);
        d->readyToExit = true;
        LOG("VrThreadSynced=%d CreatedSurface=%d ReadyToExit=%d", d->vrThreadSynced, d->createdSurface, d->readyToExit);
	}

	// Pass it on to the client app.
    d->appInterface->Command(msg);
}

void ToggleScreenColor()
{
	static int	color;

	color ^= 1;

    glEnable(GL_WRITEONLY_RENDERING_QCOM);
    glClearColor(color, 1-color, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

	// The Adreno driver has an unfortunate optimization so it doesn't
	// actually flush if all that was done was a clear.
	GL_Finish();
    glDisable(GL_WRITEONLY_RENDERING_QCOM);
}

void App::interpretTouchpad(VrInput & input)
{
	// 1) Down -> Up w/ Motion = Slide
	// 2) Down -> Up w/out Motion -> Timeout = Single Tap
	// 3) Down -> Up w/out Motion -> Down -> Timeout = Nothing
	// 4) Down -> Up w/out Motion -> Down -> Up = Double Tap
	static const float timer_finger_down = 0.3f;
	static const float timer_finger_up = 0.3f;
	static const float min_swipe_distance = 100.0f;

	float currentTime = ovr_GetTimeInSeconds();
    float deltaTime = currentTime - d->lastTouchpadTime;
    d->lastTouchpadTime = currentTime;
    d->touchpadTimer = d->touchpadTimer + deltaTime;

	bool down = false, up = false;
	bool currentTouchDown = input.buttonState & BUTTON_TOUCH;

    if (currentTouchDown && !d->lastTouchDown)
	{
		//CreateToast("DOWN");
		down = true;
        d->touchOrigin = input.touch;
	}

    if (!currentTouchDown && d->lastTouchDown)
	{
		//CreateToast("UP");
		up = true;
	}

    d->lastTouchDown = currentTouchDown;

    input.touchRelative = input.touch - d->touchOrigin;
	float touchMagnitude = input.touchRelative.Length();
	input.swipeFraction = touchMagnitude / min_swipe_distance;

    switch (d->touchState)
	{
	case 0:
		//CreateToast("0 - %f", touchpadTimer);
        if (down)
		{
            d->touchState = 1;
            d->touchpadTimer = 0.0f;
		}
		break;
	case 1:
		//CreateToast("1 - %f", touchpadTimer);
		//CreateToast("1 - %f", touchMagnitude);
        if (touchMagnitude >= min_swipe_distance)
		{
			int dir = 0;
            if (fabs(input.touchRelative[0]) > fabs(input.touchRelative[1]))
			{
                if (input.touchRelative[0] < 0)
				{
					//CreateToast("SWIPE FORWARD");
					dir = BUTTON_SWIPE_FORWARD | BUTTON_TOUCH_WAS_SWIPE;
				}
				else
				{
					//CreateToast("SWIPE BACK");
					dir = BUTTON_SWIPE_BACK | BUTTON_TOUCH_WAS_SWIPE;
				}
			}
			else
			{
                if (input.touchRelative[1] > 0)
				{
					//CreateToast("SWIPE DOWN");
					dir = BUTTON_SWIPE_DOWN | BUTTON_TOUCH_WAS_SWIPE;
				}
				else
				{
					//CreateToast("SWIPE UP");
					dir = BUTTON_SWIPE_UP | BUTTON_TOUCH_WAS_SWIPE;
				}
			}
			input.buttonPressed |= dir;
			input.buttonReleased |= dir & ~BUTTON_TOUCH_WAS_SWIPE;
			input.buttonState |= dir;
            d->touchState = 0;
            d->touchpadTimer = 0.0f;
		}
        else if (up)
		{
            if (d->touchpadTimer < timer_finger_down)
			{
                d->touchState = 2;
                d->touchpadTimer = 0.0f;
			}
			else
			{
				//CreateToast("SINGLE TOUCH");
				input.buttonPressed |= BUTTON_TOUCH_SINGLE;
				input.buttonReleased |= BUTTON_TOUCH_SINGLE;
				input.buttonState |= BUTTON_TOUCH_SINGLE;
                d->touchState = 0;
                d->touchpadTimer = 0.0f;
			}
		}
		break;
	case 2:
		//CreateToast("2 - %f", touchpadTimer);
        if (d->touchpadTimer >= timer_finger_up)
		{
			//CreateToast("SINGLE TOUCH");
			input.buttonPressed |= BUTTON_TOUCH_SINGLE;
			input.buttonReleased |= BUTTON_TOUCH_SINGLE;
			input.buttonState |= BUTTON_TOUCH_SINGLE;
            d->touchState = 0;
            d->touchpadTimer = 0.0f;
		}
        else if (down)
		{
            d->touchState = 3;
            d->touchpadTimer = 0.0f;
		}
		break;
	case 3:
		//CreateToast("3 - %f", touchpadTimer);
        if (d->touchpadTimer >= timer_finger_down)
		{
            d->touchState = 0;
            d->touchpadTimer = 0.0f;
		}
        else if (up)
		{
			//CreateToast("DOUBLE TOUCH");
			input.buttonPressed |= BUTTON_TOUCH_DOUBLE;
			input.buttonReleased |= BUTTON_TOUCH_DOUBLE;
			input.buttonState |= BUTTON_TOUCH_DOUBLE;
            d->touchState = 0;
            d->touchpadTimer = 0.0f;
		}
		break;
	}
}

void App::frameworkButtonProcessing(const VrInput & input)
{
	// Toggle calibration lines
	bool const rightTrigger = input.buttonState & BUTTON_RIGHT_TRIGGER;
	bool const leftTrigger = input.buttonState & BUTTON_LEFT_TRIGGER;
    if (leftTrigger && rightTrigger && (input.buttonPressed & BUTTON_START) != 0)
	{
		time_t rawTime;
        time(&rawTime);
        struct tm * timeInfo = localtime(&rawTime);
		char timeStr[128];
        strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeInfo);
        DROIDLOG("QAEvent", "%s (%.3f) - QA event occurred", timeStr, ovr_GetTimeInSeconds());
	}

	// Display tweak testing, only when holding right trigger
    if (d->enableDebugOptions && rightTrigger)
	{
#if 0
		// Cycle debug options
		static int debugMode = 0;
		static int debugValue = 0;
		static const char * modeNames[] = {
			"OFF",
			"RUNNING",
			"FROZEN"
		};
		static const char * valueNames[] = {
			"VALUE_DRAW",
			"VALUE_LATENCY"
		};
        if (input.buttonPressed & BUTTON_DPAD_UP)
		{
            debugMode = (debugMode + 1) % DEBUG_PERF_MAX;
			SwapParms.DebugGraphMode = (ovrTimeWarpDebugPerfMode)debugMode;
            CreateToast("debug graph %s: %s", modeNames[ debugMode ], valueNames[ debugValue ]);
		}
#endif

#if 0
        if (input.buttonPressed & BUTTON_DPAD_RIGHT)
		{
			SwapParms.MinimumVsyncs = SwapParms.MinimumVsyncs > 3 ? 1 : SwapParms.MinimumVsyncs + 1;
            CreateToast("MinimumVsyncs: %i", SwapParms.MinimumVsyncs);
		}
#endif

#if 0
        if (input.buttonPressed & BUTTON_DPAD_RIGHT)
		{
            debugValue = (debugValue + 1) % DEBUG_VALUE_MAX;
			SwapParms.DebugGraphValue = (ovrTimeWarpDebugPerfValue)debugValue;
            CreateToast("debug graph %s: %s", modeNames[ debugMode ], valueNames[ debugValue ]);
		}
#endif
        if (input.buttonPressed & BUTTON_DPAD_RIGHT)
		{
            jclass vmDebugClass = d->vrJni->FindClass("dalvik/system/VMDebug");
            jmethodID dumpId = d->vrJni->GetStaticMethodID(vmDebugClass, "dumpReferenceTables", "()V");
            d->vrJni->CallStaticVoidMethod(vmDebugClass, dumpId);
            d->vrJni->DeleteLocalRef(vmDebugClass);
		}

        if (input.buttonPressed & BUTTON_Y)
		{	// display current scheduler state and clock rates
            //const char * str = ovr_CreateSchedulingReport(OvrMobile);
            //CreateToast("%s", str);
		}

        if (input.buttonPressed & BUTTON_B)
		{
            if (d->swapParms.WarpOptions & SWAP_OPTION_USE_SLICED_WARP)
			{
                d->swapParms.WarpOptions &= ~SWAP_OPTION_USE_SLICED_WARP;
                createToast("eye warp");
			}
			else
			{
                d->swapParms.WarpOptions |= SWAP_OPTION_USE_SLICED_WARP;
                createToast("slice warp");
			}
		}

        if (d->swapParms.WarpOptions & SWAP_OPTION_USE_SLICED_WARP)
		{
			extern float calibrateFovScale;

            if (input.buttonPressed & BUTTON_DPAD_LEFT)
			{
                d->swapParms.PreScheduleSeconds -= 0.001f;
                createToast("Schedule: %f", d->swapParms.PreScheduleSeconds);
			}
            if (input.buttonPressed & BUTTON_DPAD_RIGHT)
			{
                d->swapParms.PreScheduleSeconds += 0.001f;
                createToast("Schedule: %f", d->swapParms.PreScheduleSeconds);
			}
            if (input.buttonPressed & BUTTON_DPAD_UP)
			{
				calibrateFovScale -= 0.01f;
                createToast("calibrateFovScale: %f", calibrateFovScale);
                pause();
                resume();
			}
            if (input.buttonPressed & BUTTON_DPAD_DOWN)
			{
				calibrateFovScale += 0.01f;
                createToast("calibrateFovScale: %f", calibrateFovScale);
                pause();
                resume();
			}
		}
	}
}

/*
 * VrThreadFunction
 *
 * Continuously renders frames when active, checking for commands
 * from the main thread between frames.
 */
void App::vrThreadFunction()
{
	// Set the name that will show up in systrace
    pthread_setname_np(pthread_self(), "NervGear::VrThread");

	// Initialize the VR thread
	{
        LOG("AppLocal::VrThreadFunction - init");

		// Get the tid for setting the scheduler
        d->vrThreadTid = gettid();

		// The Java VM needs to be attached on each thread that will use
		// it.  We need it to call UpdateTexture on surfaceTextures, which
		// must be done on the thread with the openGL context that created
		// the associated texture object current.
        LOG("javaVM->AttachCurrentThread");
        const jint rtn = d->javaVM->AttachCurrentThread(&d->vrJni, 0);
        if (rtn != JNI_OK)
		{
            FAIL("javaVM->AttachCurrentThread returned %i", rtn);
		}

		// Set up another thread for making longer-running java calls
		// to avoid hitches.
        d->ttj.Init(*d->javaVM, *this);

		// Create a new context and pbuffer surface
		const int windowDepth = 0;
		const int windowSamples = 0;
		const GLuint contextPriority = EGL_CONTEXT_PRIORITY_MEDIUM_IMG;
        d->eglr = EglSetup(EGL_NO_CONTEXT, GL_ES_VERSION,	// no share context,
				8,8,8, windowDepth, windowSamples, // r g b
                contextPriority);

		// Create our GL data objects
        initGlObjects();

        d->eyeTargets = new EyeBuffers;
        d->guiSys = new OvrGuiSysLocal;
        d->gazeCursor = new OvrGazeCursorLocal;
        d->vrMenuMgr = OvrVRMenuMgr::Create();
        d->debugLines = OvrDebugLines::Create();

		int w = 0;
		int h = 0;
        d->loadingIconTexId = LoadTextureFromApplicationPackage("res/raw/loading_indicator.png",
                                        TextureFlags_t(TEXTUREFLAG_NO_MIPMAPS), w, h);

		// Create the SurfaceTexture for dialog rendering.
        d->dialogTexture = new SurfaceTexture(d->vrJni);

        d->initFonts();

        d->soundManager.LoadSoundAssets();

        debugLines().Init();

        gazeCursor().Init();

        vrMenuMgr().init();

        guiSys().init(this, vrMenuMgr(), *d->defaultFont);

        d->volumePopup = OvrVolumePopup::Create(this, vrMenuMgr(), *d->defaultFont);

        ovr_InitLocalPreferences(d->vrJni, d->javaObject);

        d->lastTouchpadTime = ovr_GetTimeInSeconds();
	}

	// FPS counter information
	int countApplicationFrames = 0;
    double lastReportTime = ceil(ovr_GetTimeInSeconds());

    while(!(d->vrThreadSynced && d->createdSurface && d->readyToExit))
	{
        //SPAM("FRAME START");

        gazeCursor().BeginFrame();

		// Process incoming messages until queue is empty
        for (; ;)
		{
            const char * msg = d->vrMessageQueue.nextMessage();
            if (!msg)
			{
				break;
			}
            command(msg);
            free((void *)msg);
		}

		// handle any pending system activity events
		size_t const MAX_EVENT_SIZE = 4096;
		char eventBuffer[MAX_EVENT_SIZE];

        for (eVrApiEventStatus status = ovr_nextPendingEvent(eventBuffer, MAX_EVENT_SIZE);
			status >= VRAPI_EVENT_PENDING;
            status = ovr_nextPendingEvent(eventBuffer, MAX_EVENT_SIZE))
		{
            if (status != VRAPI_EVENT_PENDING)
			{
                if (status != VRAPI_EVENT_CONSUMED)
				{
                    WARN("Error %i handing System Activities Event", status);
				}
				continue;
			}

			std::stringstream s;
			s << eventBuffer;
			Json jsonObj;
			s >> jsonObj;
			if (jsonObj.type() == Json::Object)
			{
				std::string command = jsonObj["Command"].toString();
				if (command == SYSTEM_ACTIVITY_EVENT_REORIENT)
				{
					// for reorient, we recenter yaw natively, then pass the event along so that the client
					// application can also handle the event (for instance, to reposition menus)
                    LOG("VtThreadFunction: Acting on System Activity reorient event.");
                    recenterYaw(false);
				}
				else
				{
					// In the case of the returnToLauncher event, we always handler it internally and pass
					// along an empty buffer so that any remaining events still get processed by the client.
					LOG("Unhandled System Activity event: '%s'", command.c_str());
				}
			}
			else
			{
				// a malformed event string was pushed! This implies an error in the native code somewhere.
				WARN("Error parsing System Activities Event: %s");
			}
		}

		// update volume popup
        if (d->showVolumePopup)
		{
            d->volumePopup->checkForVolumeChange(this);
		}

		// If we don't have a surface yet, or we are paused, sleep until
		// something shows up on the message queue.
        if (d->windowSurface == EGL_NO_SURFACE || d->paused)
		{
            if (!(d->vrThreadSynced && d->createdSurface && d->readyToExit))
			{
                d->vrMessageQueue.SleepUntilMessage();
			}
			continue;
		}

		// if there is an error condition, warp swap and nothing else
        if (d->errorTexture != 0)
		{
            if (ovr_GetTimeInSeconds() >= d->errorMessageEndTime)
			{
                ovr_ReturnToHome(d->OvrMobile);
			}
			else
			{
                ovrTimeWarpParms warpSwapMessageParms = InitTimeWarpParms(WARP_INIT_MESSAGE, d->errorTexture.texture);
				warpSwapMessageParms.ProgramParms[0] = 0.0f;						// rotation in radians
                warpSwapMessageParms.ProgramParms[1] = 1024.0f / d->errorTextureSize;	// message size factor
                ovr_WarpSwap(d->OvrMobile, &warpSwapMessageParms);
			}
			continue;
		}

		// Let the client app initialize only once by calling OneTimeInit() when the windowSurface is valid.
        if (!oneTimeInitCalled)
		{
            if (d->appInterface->ShouldShowLoadingIcon())
			{
                const ovrTimeWarpParms warpSwapLoadingIconParms = InitTimeWarpParms(WARP_INIT_LOADING_ICON, d->loadingIconTexId);
                ovr_WarpSwap(d->OvrMobile, &warpSwapLoadingIconParms);
			}
            vInfo("launchIntentJSON:" << d->launchIntentJSON);
            vInfo("launchIntentURI:" << d->launchIntentURI);

            d->appInterface->OneTimeInit(d->launchIntentFromPackage.toCString(), d->launchIntentJSON.toCString(), d->launchIntentURI.toCString());
            oneTimeInitCalled = true;
		}

		// check updated battery status
		{
			batteryState_t state = ovr_GetBatteryState();
            d->batteryStatus = static_cast< eBatteryStatus >(state.status);
            d->batteryLevel = state.level;
		}

		// latch the current joypad state and note transitions
        d->vrFrame.Input = d->joypad;
        d->vrFrame.Input.buttonPressed = d->joypad.buttonState & (~d->lastVrFrame.Input.buttonState);
        d->vrFrame.Input.buttonReleased = ~d->joypad.buttonState & (d->lastVrFrame.Input.buttonState & ~BUTTON_TOUCH_WAS_SWIPE);

        if (d->lastVrFrame.Input.buttonState & BUTTON_TOUCH_WAS_SWIPE)
		{
            if (d->lastVrFrame.Input.buttonReleased & BUTTON_TOUCH)
			{
                d->vrFrame.Input.buttonReleased |= BUTTON_TOUCH_WAS_SWIPE;
			}
			else
			{
				// keep it around this frame
                d->vrFrame.Input.buttonState |= BUTTON_TOUCH_WAS_SWIPE;
			}
		}

		// Synthesize swipe gestures
        interpretTouchpad(d->vrFrame.Input);

        if (d->recenterYawFrameStart != 0)
		{
			// Perform a reorient before sensor data is read.  Allows apps to reorient without having invalid orientation information for that frame.
			// Do a warp swap black on the frame the recenter started.
            recenterYaw(d->recenterYawFrameStart == (d->vrFrame.FrameNumber + 1));  // vrFrame.FrameNumber hasn't been incremented yet, so add 1.
		}

		// Get the latest head tracking state, predicted ahead to the midpoint of the time
		// it will be displayed.  It will always be corrected to the real values by
		// time warp, but the closer we get, the less black will be pulled in at the edges.
		const double now = ovr_GetTimeInSeconds();
		static double prev;
		const double rawDelta = now - prev;
		prev = now;
        const double clampedPrediction = Alg::Min(0.1, rawDelta * 2);
        d->sensorForNextWarp = ovr_GetPredictedSensorState(d->OvrMobile, now + clampedPrediction);

        d->vrFrame.PoseState = d->sensorForNextWarp.Predicted;
        d->vrFrame.OvrStatus = d->sensorForNextWarp.Status;
        d->vrFrame.DeltaSeconds   = Alg::Min(0.1, rawDelta);
        d->vrFrame.FrameNumber++;

		// Don't allow this to be excessively large, which can cause application problems.
        if (d->vrFrame.DeltaSeconds > 0.1f)
		{
            d->vrFrame.DeltaSeconds = 0.1f;
		}

        d->lastVrFrame = d->vrFrame;

		// resend any debug lines that have expired
        debugLines().BeginFrame(d->vrFrame.FrameNumber);

		// reset any VR menu submissions from previous frame
        vrMenuMgr().beginFrame();

#if 0
		// Joypad latency test
		// When this is enabled, each tap of a button will toggle the screen
		// color, allowing the tap-to-photons latency to be measured.
        if (0)
		{
            if (vrFrame.Input.buttonPressed)
			{
                LOG("Input.buttonPressed");
				static bool shut;
                if (!shut)
				{
					// shut down timewarp, then go back into frontbuffer mode
					shut = true;
                    ovr_LeaveVrMode(OvrMobile);
					static DirectRender	dr;
                    dr.InitForCurrentSurface(VrJni, true, 19);
				}
				ToggleScreenColor();
			}
			vrMessageQueue.SleepUntilMessage();
			continue;
		}
		// IMU latency test
        if (0)
		{
			static bool buttonDown;
            const ovrSensorState sensor = ovr_GetPredictedSensorState(OvrMobile, now + clampedPrediction);
            const float acc = Vector3f(sensor.Predicted.AngularVelocity).Length();
//			const float acc = fabs(Vector3f(sensor.Predicted.LinearAcceleration).Length() - 9.8f);

			static int count;
            if (++count % 10 == 0)
			{
                LOG("acc: %f", acc);
			}
            const bool buttonNow = (acc > 0.1f);
            if (buttonNow != buttonDown)
			{
                LOG("accel button");
				buttonDown = buttonNow;
				static bool shut;
                if (!shut)
				{
					// shut down timewarp, then go back into frontbuffer mode
					shut = true;
                    ovr_LeaveVrMode(OvrMobile);
					static DirectRender	dr;
                    dr.InitForCurrentSurface(VrJni, true, 19);
				}
				ToggleScreenColor();
			}
            usleep(1000);
			continue;
		}
#endif

        frameworkButtonProcessing(d->vrFrame.Input);

        KeyState::eKeyEventType event = d->backKeyState.Update(ovr_GetTimeInSeconds());
        if (event != KeyState::KEY_EVENT_NONE)
		{
            //LOG("BackKey: event %s", KeyState::EventNames[ event ]);
			// always allow the gaze cursor to peek at the event so it can start the gaze timer if necessary
            if (JniUtils::GetCurrentActivityName(d->vrJni, d->javaObject).icompare(PUI_CLASS_NAME) != 0) {
				// update the gaze cursor timer
                if (event == KeyState::KEY_EVENT_DOWN)
				{
                    gazeCursor().StartTimer(backKeyState().GetLongPressTime(), backKeyState().GetDoubleTapTime());
				}
                else if (event == KeyState::KEY_EVENT_DOUBLE_TAP || event == KeyState::KEY_EVENT_SHORT_PRESS)
				{
                    gazeCursor().CancelTimer();
				}
                else if (event == KeyState::KEY_EVENT_LONG_PRESS)
				{
                    //StartSystemActivity(PUI_GLOBAL_MENU);
                    ovr_ExitActivity(d->OvrMobile, EXIT_TYPE_FINISH);
				}
			}

			// let the menu handle it if it's open
            bool consumedKey = guiSys().onKeyEvent(this, AKEYCODE_BACK, event);

			// pass to the app if nothing handled it before this
            if (!consumedKey)
			{
                consumedKey = d->appInterface->onKeyEvent(AKEYCODE_BACK, event);
			}
			// if nothing consumed the key and it's a short-press, exit the application to OculusHome
            if (!consumedKey)
			{
                if (event == KeyState::KEY_EVENT_SHORT_PRESS)
				{
					consumedKey = true;
                    LOG("BUTTON_BACK: confirming quit in platformUI");
                    ovr_ExitActivity(d->OvrMobile, EXIT_TYPE_FINISH);
				}
			}
		}

        if (d->showFPS)
		{
			const int FPS_NUM_FRAMES_TO_AVERAGE = 30;
			static double  LastFrameTime = ovr_GetTimeInSeconds();
			static double  AccumulatedFrameInterval = 0.0;
			static int   NumAccumulatedFrames = 0;
			static float LastFrameRate = 60.0f;

			double currentFrameTime = ovr_GetTimeInSeconds();
			double frameInterval = currentFrameTime - LastFrameTime;
			AccumulatedFrameInterval += frameInterval;
			NumAccumulatedFrames++;
            if (NumAccumulatedFrames > FPS_NUM_FRAMES_TO_AVERAGE) {
                double interval = (AccumulatedFrameInterval / NumAccumulatedFrames);  // averaged
				AccumulatedFrameInterval = 0.0;
				NumAccumulatedFrames = 0;
                LastFrameRate = 1.0f / float(interval > 0.000001 ? interval : 0.00001);
			}

            Vector3f viewPos = GetViewMatrixPosition(d->lastViewMatrix);
            Vector3f viewFwd = GetViewMatrixForward(d->lastViewMatrix);
			Vector3f newPos = viewPos + viewFwd * 1.5f;
            d->fpsPointTracker.Update(ovr_GetTimeInSeconds(), newPos);

			fontParms_t fp;
			fp.AlignHoriz = HORIZONTAL_CENTER;
			fp.Billboard = true;
			fp.TrackRoll = false;
            worldFontSurface().DrawTextBillboarded3Df(defaultFont(), fp, d->fpsPointTracker.GetCurPosition(),
                    0.8f, Vector4f(1.0f, 0.0f, 0.0f, 1.0f), "%.1f fps", LastFrameRate);
			LastFrameTime = currentFrameTime;
		}


		// draw info text
        if (d->infoTextEndFrame >= d->vrFrame.FrameNumber)
		{
            Vector3f viewPos = GetViewMatrixPosition(d->lastViewMatrix);
            Vector3f viewFwd = GetViewMatrixForward(d->lastViewMatrix);
            Vector3f viewUp(0.0f, 1.0f, 0.0f);
            Vector3f viewLeft = viewUp.Cross(viewFwd);
            Vector3f newPos = viewPos + viewFwd * d->infoTextOffset.z + viewUp * d->infoTextOffset.y + viewLeft * d->infoTextOffset.x;
            d->infoTextPointTracker.Update(ovr_GetTimeInSeconds(), newPos);

			fontParms_t fp;
			fp.AlignHoriz = HORIZONTAL_CENTER;
			fp.AlignVert = VERTICAL_CENTER;
			fp.Billboard = true;
			fp.TrackRoll = false;
            worldFontSurface().DrawTextBillboarded3Df(defaultFont(), fp, d->infoTextPointTracker.GetCurPosition(),
                    1.0f, d->infoTextColor, d->infoText.toCString());
		}

		// Main loop logic / draw code
        if (!d->readyToExit)
		{
            this->d->lastViewMatrix = d->appInterface->Frame(d->vrFrame);
		}

        ovr_HandleDeviceStateChanges(d->OvrMobile);

		// MWC demo hack to allow keyboard swipes
        d->joypad.buttonState &= ~(BUTTON_SWIPE_FORWARD|BUTTON_SWIPE_BACK);

		// Report frame counts once a second
		countApplicationFrames++;
        const double timeNow = floor(ovr_GetTimeInSeconds());
        if (timeNow > lastReportTime)
		{
#if 1	// it is sometimes handy to remove this spam from the log
            LOG("FPS: %i GPU time: %3.1f ms ", countApplicationFrames, d->eyeTargets->LogEyeSceneGpuTime.GetTotalTime());
#endif
			countApplicationFrames = 0;
			lastReportTime = timeNow;
		}

        //SPAM("FRAME END");
	}

	// Shutdown the VR thread
	{
        LOG("AppLocal::VrThreadFunction - shutdown");

		// Shut down the message queue so it cannot overflow.
        d->vrMessageQueue.Shutdown();

        if (d->errorTexture != 0)
		{
            FreeTexture(d->errorTexture);
		}

        d->appInterface->OneTimeShutdown();

        guiSys().shutdown(vrMenuMgr());

        vrMenuMgr().shutdown();

        gazeCursor().Shutdown();

        debugLines().Shutdown();

        d->shutdownFonts();

        delete d->dialogTexture;
        d->dialogTexture = nullptr;

        delete d->eyeTargets;
        d->eyeTargets = nullptr;

        delete d->guiSys;
        d->guiSys = nullptr;

        delete d->gazeCursor;
        d->gazeCursor = nullptr;

        OvrVRMenuMgr::Free(d->vrMenuMgr);
        OvrDebugLines::Free(d->debugLines);

		ShutdownGlObjects();

        EglShutdown(d->eglr);

		// Detach from the Java VM before exiting.
        LOG("javaVM->DetachCurrentThread");
        const jint rtn = d->javaVM->DetachCurrentThread();
        if (rtn != JNI_OK)
		{
            LOG("javaVM->DetachCurrentThread returned %i", rtn);
		}
	}
}

// Shim to call a C++ object from a posix thread start.
void *App::ThreadStarter(void * parm)
{
    ((App *)parm)->vrThreadFunction();

    return nullptr;
}

/*
 * GetEyeParms()
 */
EyeParms App::eyeParms()
{
    return d->vrParms;
}

/*
 * SetVrParms()
 */
void App::setEyeParms(const EyeParms parms)
{
    d->vrParms = parms;
}

/*
 * KeyEvent
 */
static int buttonMappings[] = {
	96, 	// BUTTON_A
	97,		// BUTTON_B
	99, 	// BUTTON_X
	100,	// BUTTON_Y
	108, 	// BUTTON_START
	4,		// BUTTON_BACK
	109, 	// BUTTON_SELECT
	82, 	// BUTTON_MENU
	103, 	// BUTTON_RIGHT_TRIGGER
	102, 	// BUTTON_LEFT_TRIGGER
	19,		// BUTTON_DPAD_UP
	20,		// BUTTON_DPAD_DOWN
	21,		// BUTTON_DPAD_LEFT
	22,		// BUTTON_DPAD_RIGHT
	200,	// BUTTON_LSTICK_UP
	201,	// BUTTON_LSTICK_DOWN
	202,	// BUTTON_LSTICK_LEFT
	203,	// BUTTON_LSTICK_RIGHT
	204,	// BUTTON_RSTICK_UP
	205,	// BUTTON_RSTICK_DOWN
	206,	// BUTTON_RSTICK_LEFT
	207,	// BUTTON_RSTICK_RIGHT
	9999,	// touch is handled separately

	-1
};

#if defined(TEST_TIMEWARP_WATCHDOG)
static float test = 0.0f;
#endif

void App::keyEvent(const int keyCode, const bool down, const int repeatCount)
{
	// the back key is special because it has to handle long-press and double-tap
    if (keyCode == AKEYCODE_BACK)
	{
        //DROIDLOG("BackKey", "BACK KEY PRESSED");
		// back key events, because of special handling for double-tap, short-press and long-press,
		// are handled in AppLocal::VrThreadFunction.
        d->backKeyState.HandleEvent(ovr_GetTimeInSeconds(), down, repeatCount);
		return;
	}

	// the app menu is always the first consumer so it cannot be usurped
	bool consumedKey = false;
    if (repeatCount == 0)
	{
        consumedKey = guiSys().onKeyEvent(this, keyCode, down ? KeyState::KEY_EVENT_DOWN : KeyState::KEY_EVENT_UP);
	}

	// for all other keys, allow VrAppInterface the chance to handle and consume the key first
    if (!consumedKey)
	{
        consumedKey = d->appInterface->onKeyEvent(keyCode, down ? KeyState::KEY_EVENT_DOWN : KeyState::KEY_EVENT_UP);
	}

	// ALL VRLIB KEY HANDLING OTHER THAN APP MENU SHOULD GO HERE
    if (!consumedKey && d->enableDebugOptions)
	{
		float const IPD_STEP = 0.001f;

		// FIXME: this should set consumedKey = true now that we pass key events via appInterface first,
		// but this would likely break some apps right now that rely on the old behavior
		// consumedKey = true;

        if (down && keyCode == AKEYCODE_RIGHT_BRACKET)
		{
            LOG("BUTTON_SWIPE_FORWARD");
            d->joypad.buttonState |= BUTTON_SWIPE_FORWARD;
			return;
		}
        else if (down && keyCode == AKEYCODE_LEFT_BRACKET)
		{
            LOG("BUTTON_SWIPE_BACK");
            d->joypad.buttonState |= BUTTON_SWIPE_BACK;
			return;
		}
        else if (keyCode == AKEYCODE_S)
		{
            if (repeatCount == 0 && down) // first down only
			{
                d->eyeTargets->ScreenShot();
                createToast("screenshot");
				return;
			}
		}
        else if (keyCode == AKEYCODE_F && down && repeatCount == 0)
		{
            setShowFPS(!showFPS());
			return;
		}
        else if (keyCode == AKEYCODE_COMMA && down && repeatCount == 0)
		{
			float const IPD_MIN_CM = 0.0f;
            d->viewParms.InterpupillaryDistance = Alg::Max(IPD_MIN_CM * 0.01f, d->viewParms.InterpupillaryDistance - IPD_STEP);
            showInfoText(1.0f, "%.3f", d->viewParms.InterpupillaryDistance);
			return;
		}
        else if (keyCode == AKEYCODE_PERIOD && down && repeatCount == 0)
		{
			float const IPD_MAX_CM = 8.0f;
            d->viewParms.InterpupillaryDistance = Alg::Min(IPD_MAX_CM * 0.01f, d->viewParms.InterpupillaryDistance + IPD_STEP);
            showInfoText(1.0f, "%.3f", d->viewParms.InterpupillaryDistance);
			return;
        }
#if defined(TEST_TIMEWARP_WATCHDOG)	// test TimeWarp sched_fifo watchdog
        else if (keyCode == AKEYCODE_T && down && repeatCount == 0)
		{
			const double SPIN_TIME = 60.0;
            DROIDLOG("TimeWarp", "Spinning on VrThread for %f seconds...", SPIN_TIME);
			double start = ovr_GetTimeInSeconds();
            while (ovr_GetTimeInSeconds() < start + SPIN_TIME)
			{
                test = cos(test);
			}
		}
#endif
	}

	// Keys always map to joystick buttons right now even if consumed otherwise.
	// This probably should change but it's unclear what this might break right now.
    for (int i = 0 ; buttonMappings[i] != -1 ; i++)
	{
		// joypad buttons come in with 0x10000 as a flag
        if ((keyCode & ~0x10000) == buttonMappings[i])
		{
            if (down)
			{
                d->joypad.buttonState |= 1<<i;
			}
			else
			{
                d->joypad.buttonState &= ~(1<<i);
			}
			return;
		}
	}
}

Matrix4f App::matrixInterpolation(const Matrix4f & startMatrix, const Matrix4f & endMatrix, double t)
{
	Matrix4f result;
	Quat<float> startQuat = (Quat<float>) startMatrix ;
	Quat<float> endQuat = (Quat<float>) endMatrix ;
	Quat<float> quatResult;

	double cosHalfTheta = startQuat.w * endQuat.w +
						  startQuat.x * endQuat.x +
						  startQuat.y * endQuat.y +
						  startQuat.z * endQuat.z;

    if(fabs(cosHalfTheta) >= 1.0)
	{
		result = startMatrix;

        Vector3<float> startTrans(startMatrix.M[0][3], startMatrix.M[1][3], startMatrix.M[2][3]);
        Vector3<float> endTrans(endMatrix.M[0][3], endMatrix.M[1][3], endMatrix.M[2][3]);

        Vector3<float> interpolationVector = startTrans.Lerp(endTrans, t);

		result.M[0][3] = interpolationVector.x;
		result.M[1][3] = interpolationVector.y;
		result.M[2][3] = interpolationVector.z;
		return result;
	}


	bool reverse_q1 = false;
    if (cosHalfTheta < 0)
	{
		reverse_q1 = true;
		cosHalfTheta = -cosHalfTheta;
	}

	// Calculate intermediate rotation.
    const double halfTheta = acos(cosHalfTheta);
    const double sinHalfTheta = sqrt(1.0 - (cosHalfTheta * cosHalfTheta));

	// if theta = 180 degrees then result is not fully defined
	// we could rotate around any axis normal to qa or qb
    if(fabs(sinHalfTheta) < 0.001)
	{
		if (!reverse_q1)
		{
            quatResult.w = (1 - t) * startQuat.w + t * endQuat.w;
            quatResult.x = (1 - t) * startQuat.x + t * endQuat.x;
            quatResult.y = (1 - t) * startQuat.y + t * endQuat.y;
            quatResult.z = (1 - t) * startQuat.z + t * endQuat.z;
		}
		else
		{
            quatResult.w = (1 - t) * startQuat.w - t * endQuat.w;
            quatResult.x = (1 - t) * startQuat.x - t * endQuat.x;
            quatResult.y = (1 - t) * startQuat.y - t * endQuat.y;
            quatResult.z = (1 - t) * startQuat.z - t * endQuat.z;
		}
		result = (Matrix4f) quatResult;
	}
	else
	{

        const double A = sin((1 - t) * halfTheta) / sinHalfTheta;
        const double B = sin(t *halfTheta) / sinHalfTheta;
		if (!reverse_q1)
		{
			quatResult.w =  A * startQuat.w + B * endQuat.w;
			quatResult.x =  A * startQuat.x + B * endQuat.x;
			quatResult.y =  A * startQuat.y + B * endQuat.y;
			quatResult.z =  A * startQuat.z + B * endQuat.z;
		}
		else
		{
			quatResult.w =  A * startQuat.w - B * endQuat.w;
			quatResult.x =  A * startQuat.x - B * endQuat.x;
			quatResult.y =  A * startQuat.y - B * endQuat.y;
			quatResult.z =  A * startQuat.z - B * endQuat.z;
		}

		result = (Matrix4f) quatResult;
	}

    Vector3<float> startTrans(startMatrix.M[0][3], startMatrix.M[1][3], startMatrix.M[2][3]);
    Vector3<float> endTrans(endMatrix.M[0][3], endMatrix.M[1][3], endMatrix.M[2][3]);

    Vector3<float> interpolationVector = startTrans.Lerp(endTrans, t);

	result.M[0][3] = interpolationVector.x;
	result.M[1][3] = interpolationVector.y;
	result.M[2][3] = interpolationVector.z;

	return result;
}

OvrGuiSys & App::guiSys()
{
    return *d->guiSys;
}
OvrGazeCursor & App::gazeCursor()
{
    return *d->gazeCursor;
}
BitmapFont & App::defaultFont()
{
    return *d->defaultFont;
}
BitmapFontSurface & App::worldFontSurface()
{
    return *d->worldFontSurface;
}
BitmapFontSurface & App::menuFontSurface()
{
    return *d->menuFontSurface;
}   // TODO: this could be in the menu system now

OvrVRMenuMgr & App::vrMenuMgr()
{
    return *d->vrMenuMgr;
}
OvrDebugLines & App::debugLines()
{
    return *d->debugLines;
}
const VStandardPath & App::storagePaths()
{
    return *d->storagePaths;
}
OvrSoundManager & App::soundMgr()
{
    return d->soundManager;
}

bool App::isGuiOpen() const
{
    return d->guiSys->isAnyMenuOpen();
}

int App::wifiSignalLevel() const
{
	int level = ovr_GetWifiSignalLevel();
	return level;
}

eWifiState App::wifiState() const
{
	return ovr_GetWifiState();
}

int App::cellularSignalLevel() const
{
	int level = ovr_GetCellularSignalLevel();
	return level;
}

eCellularState App::cellularState() const
{
	return ovr_GetCellularState();
}

bool App::isAsynchronousTimeWarp() const
{
	return VrModeParms.AsynchronousTimeWarp;
}

bool App::hasHeadphones() const {
	return ovr_GetHeadsetPluggedState();
}

bool App::framebufferIsSrgb() const
{
    return d->framebufferIsSrgb;
}

bool App::framebufferIsProtected() const
{
    return d->framebufferIsProtected;
}

bool App::renderMonoMode() const
{
    return d->renderMonoMode;
}

void App::setRenderMonoMode(bool const mono)
{
    d->renderMonoMode = mono;
}

const VString &App::packageCodePath() const
{
    return d->packageCodePath;
}

Matrix4f const & App::lastViewMatrix() const
{
    return d->lastViewMatrix;
}

void App::setLastViewMatrix(Matrix4f const & m)
{
    d->lastViewMatrix = m;
}

EyeParms & App::vrParms()
{
    return d->vrParms;
}

ovrModeParms App::vrModeParms()
{
	return VrModeParms;
}

void App::setPopupDistance(float const distance)
{
    d->popupDistance = distance;
}

float App::popupDistance() const
{
    return d->popupDistance;
}

void App::setPopupScale(float const s)
{
    d->popupScale = s;
}

float App::popupScale() const
{
    return d->popupScale;
}

bool App::isPowerSaveActive() const
{
	return ovr_GetPowerLevelStateThrottled();
}

int App::cpuLevel() const
{
	return VrModeParms.CpuLevel;
}

int App::gpuLevel() const
{
	return VrModeParms.GpuLevel;
}

int App::batteryLevel() const
{
    return d->batteryLevel;
}

eBatteryStatus App::batteryStatus() const
{
    return d->batteryStatus;
}

JavaVM	* App::javaVM()
{
    return d->javaVM;
}

JNIEnv * App::uiJni()
{
    return d->uiJni;
}

JNIEnv * App::vrJni()
{
    return d->vrJni;
}

jobject	& App::javaObject()
{
    return d->javaObject;
}

jclass & App::vrActivityClass()
{
    return d->vrActivityClass;
}

SurfaceTexture * App::dialogTexture()
{
    return d->dialogTexture;
}

ovrTimeWarpParms const & App::swapParms() const
{
    return d->swapParms;
}

ovrTimeWarpParms & App::swapParms()
{
    return d->swapParms;
}

ovrSensorState const & App::sensorForNextWarp() const
{
    return d->sensorForNextWarp;
}

void App::setShowFPS(bool const show)
{
    bool wasShowing = d->showFPS;
    d->showFPS = show;
    if (!wasShowing && d->showFPS)
	{
        d->fpsPointTracker.Reset();
	}
}

bool App::showFPS() const
{
    return d->showFPS;
}

VrAppInterface * App::appInterface()
{
    return d->appInterface;
}

VrViewParms const &	App::vrViewParms() const
{
    return d->viewParms;
}

void App::setVrViewParms(VrViewParms const & parms)
{
    d->viewParms = parms;
}

void App::showInfoText(float const duration, const char * fmt, ...)
{
	char buffer[1024];
	va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    d->infoText = buffer;
    d->infoTextColor = Vector4f(1.0f);
    d->infoTextOffset = Vector3f(0.0f, 0.0f, 1.5f);
    d->infoTextPointTracker.Reset();
    d->infoTextEndFrame = d->vrFrame.FrameNumber + (long long)(duration * 60.0f) + 1;
}

void App::showInfoText(float const duration, Vector3f const & offset, Vector4f const & color, const char * fmt, ...)
{
	char buffer[1024];
	va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    d->infoText = buffer;
    d->infoTextColor = color;
    if (offset != d->infoTextOffset || d->infoTextEndFrame < d->vrFrame.FrameNumber)
	{
        d->infoTextPointTracker.Reset();
	}
    d->infoTextOffset = offset;
    d->infoTextEndFrame = d->vrFrame.FrameNumber + (long long)(duration * 60.0f) + 1;
}

KeyState & App::backKeyState()
{
    return d->backKeyState;
}

ovrMobile * App::getOvrMobile()
{
    return d->OvrMobile;
}

void App::setShowVolumePopup(bool const show)
{
    d->showVolumePopup = show;
}

bool App::showVolumePopup() const
{
    return d->showVolumePopup;
}

const VString &App::packageName() const
{
    return d->packageName;
}

bool App::isWifiConnected() const
{
    jmethodID isWififConnectedMethodId = JniUtils::GetStaticMethodID(d->vrJni, d->vrLibClass, "isWifiConnected", "(Landroid/app/Activity;)Z");
    return d->vrJni->CallStaticBooleanMethod(d->vrLibClass, isWififConnectedMethodId, d->javaObject);
}

void App::recenterYaw(const bool showBlack)
{
    LOG("AppLocal::RecenterYaw");
    if (showBlack)
	{
        const ovrTimeWarpParms warpSwapBlackParms = InitTimeWarpParms(WARP_INIT_BLACK);
        ovr_WarpSwap(d->OvrMobile, &warpSwapBlackParms);
	}
    ovr_RecenterYaw(d->OvrMobile);

	// Change lastViewMatrix to mirror what is done to the sensor orientation by ovr_RecenterYaw.
	// Get the current yaw rotation and cancel it out. This is necessary so that subsystems that
	// rely on lastViewMatrix do not end up using the orientation from before the recenter if they
	// are called before the beginning of the next frame.
	float yaw;
	float pitch;
	float roll;
    d->lastViewMatrix.ToEulerAngles< Axis_Y, Axis_X, Axis_Z, Rotate_CCW, Handed_R >(&yaw, &pitch, &roll);

	// undo the yaw
    Matrix4f unrotYawMatrix(Quatf(Axis_Y, -yaw));
    d->lastViewMatrix = d->lastViewMatrix * unrotYawMatrix;

    guiSys().resetMenuOrientations(this, d->lastViewMatrix);
}

void App::setRecenterYawFrameStart(const long long frameNumber)
{
    d->recenterYawFrameStart = frameNumber;
}

long long App::recenterYawFrameStart() const
{
    return d->recenterYawFrameStart;
}

void ShowFPS(void * appPtr, const char * cmd) {
	int show = 0;
    sscanf(cmd, "%i", &show);
    OVR_ASSERT(appPtr != nullptr);	// something changed / broke in the OvrConsole code if this is nullptr
    ((App*)appPtr)->setShowFPS(show != 0);
}

// Debug tool to draw outlines of a 3D bounds
void App::drawBounds( const Vector3f &mins, const Vector3f &maxs, const Matrix4f &mvp, const Vector3f &color )
{
    Matrix4f	scaled = mvp * Matrix4f::Translation( mins ) * Matrix4f::Scaling( maxs - mins );
    const GlProgram & prog = d->untexturedMvpProgram;
    glUseProgram(prog.program);
    glLineWidth( 1.0f );
    glUniform4f(prog.uColor, color.x, color.y, color.z, 1);
    glUniformMatrix4fv(prog.uMvp, 1, GL_FALSE /* not transposed */,
            scaled.Transposed().M[0] );
    glBindVertexArrayOES_( d->unitCubeLines.vertexArrayObject );
    glDrawElements(GL_LINES, d->unitCubeLines.indexCount, GL_UNSIGNED_SHORT, NULL);
    glBindVertexArrayOES_( 0 );
}

void App::drawDialog( const Matrix4f & mvp )
{
    // draw the pop-up dialog
    const float now = ovr_GetTimeInSeconds();
    if ( now >= d->dialogStopSeconds )
    {
        return;
    }
    const Matrix4f dialogMvp = mvp * d->dialogMatrix;

    const float fadeSeconds = 0.5f;
    const float f = now - ( d->dialogStopSeconds - fadeSeconds );
    const float clampF = f < 0.0f ? 0.0f : f;
    const float alpha = 1.0f - clampF;

    drawPanel( d->dialogTexture->textureId, dialogMvp, alpha );
}

void App::drawPanel( const GLuint externalTextureId, const Matrix4f & dialogMvp, const float alpha )
{
    const GlProgram & prog = d->externalTextureProgram2;
    glUseProgram( prog.program );
    glUniform4f(prog.uColor, 1, 1, 1, alpha );

    glUniformMatrix4fv(prog.uTexm, 1, GL_FALSE, Matrix4f::Identity().Transposed().M[0]);
    glUniformMatrix4fv(prog.uMvp, 1, GL_FALSE, dialogMvp.Transposed().M[0] );

    // It is important that panels write to destination alpha, or they
    // might get covered by an overlay plane/cube in TimeWarp.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, externalTextureId);
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    d->panelGeometry.Draw();
    glDisable( GL_BLEND );
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0 );	// don't leave it bound
}

void App::drawEyeViewsPostDistorted( Matrix4f const & centerViewMatrix, const int numPresents )
{
    // update vr lib systems after the app frame, but before rendering anything
    guiSys().frame( this, d->vrFrame, vrMenuMgr(), defaultFont(), menuFontSurface(), centerViewMatrix );
    gazeCursor().Frame( centerViewMatrix, d->vrFrame.DeltaSeconds );

    menuFontSurface().Finish( centerViewMatrix );
    worldFontSurface().Finish( centerViewMatrix );
    vrMenuMgr().finish( centerViewMatrix );

    // Increase the fov by about 10 degrees if we are not holding 60 fps so
    // there is less black pull-in at the edges.
    //
    // Doing this dynamically based just on time causes visible flickering at the
    // periphery when the fov is increased, so only do it if minimumVsyncs is set.
    const float fovDegrees = d->hmdInfo.SuggestedEyeFov[0] +
            ( ( ( d->swapParms.MinimumVsyncs > 1 ) || ovr_GetPowerLevelStateThrottled() ) ? 10.0f : 0.0f ) +
            ( ( !d->showVignette ) ? 5.0f : 0.0f );

    // DisplayMonoMode uses a single eye rendering for speed improvement
    // and / or high refresh rate double-scan hardware modes.
    const int numEyes = d->renderMonoMode ? 1 : 2;

    // Flush out and report any errors
    GL_CheckErrors("FrameStart");

    if ( d->drawCalibrationLines && d->calibrationLinesDrawn )
    {
        // doing a time warp test, don't generate new images
        LOG( "drawCalibrationLines && calibrationLinesDrawn" );
    }
    else
    {
        // possibly change the buffer parameters
        d->eyeTargets->BeginFrame( d->vrParms );

        for ( int eye = 0; eye < numEyes; eye++ )
        {
            d->eyeTargets->BeginRenderingEye( eye );

            // Call back to the app for drawing.
            const Matrix4f mvp = d->appInterface->DrawEyeView( eye, fovDegrees );

            vrMenuMgr().renderSubmitted( mvp.Transposed(), centerViewMatrix );
            menuFontSurface().Render3D( defaultFont(), mvp.Transposed() );
            worldFontSurface().Render3D( defaultFont(), mvp.Transposed() );

            glDisable( GL_DEPTH_TEST );
            glDisable( GL_CULL_FACE );

            // Optionally draw thick calibration lines into the texture,
            // which will be overlayed by the thinner origin cross when
            // distorted to the window.
            if ( d->drawCalibrationLines )
            {
                d->eyeDecorations.DrawEyeCalibrationLines( fovDegrees, eye );
                d->calibrationLinesDrawn = true;
            }
            else
            {
                d->calibrationLinesDrawn = false;
            }

            drawDialog( mvp );

            gazeCursor().Render( eye, mvp );

            debugLines().Render( mvp.Transposed() );

            if ( d->showVignette )
            {
                // Draw a thin vignette at the edges of the view so clamping will give black
                // This will not be reflected correctly in overlay planes.
                // EyeDecorations.DrawEyeVignette();

                d->eyeDecorations.FillEdge( d->vrParms.resolution, d->vrParms.resolution );
            }

            d->eyeTargets->EndRenderingEye( eye );
        }
    }

    // This eye set is complete, use it now.
    if ( numPresents > 0 )
    {
        const CompletedEyes eyes = d->eyeTargets->GetCompletedEyes();

        for ( int eye = 0; eye < MAX_WARP_EYES; eye++ )
        {
            d->swapParms.Images[eye][0].TexCoordsFromTanAngles = TanAngleMatrixFromFov( fovDegrees );
            d->swapParms.Images[eye][0].TexId = eyes.Textures[d->renderMonoMode ? 0 : eye ];
            d->swapParms.Images[eye][0].Pose = d->sensorForNextWarp.Predicted;
        }

        ovr_WarpSwap( d->OvrMobile, &d->swapParms );
    }
}

// Draw a screen to an eye buffer the same way it would be drawn as a
// time warp overlay.
void App::drawScreenDirect( const GLuint texid, const ovrMatrix4f & mvp )
{
    const Matrix4f mvpMatrix( mvp );
    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, texid );

    glUseProgram( d->overlayScreenDirectProgram.program );

    glUniformMatrix4fv( d->overlayScreenDirectProgram.uMvp, 1, GL_FALSE, mvpMatrix.Transposed().M[0] );

    glBindVertexArrayOES_( d->unitSquare.vertexArrayObject );
    glDrawElements( GL_TRIANGLES, d->unitSquare.indexCount, GL_UNSIGNED_SHORT, NULL );

    glBindTexture( GL_TEXTURE_2D, 0 );	// don't leave it bound
}

// draw a zero to destination alpha
void App::drawScreenMask( const ovrMatrix4f & mvp, const float fadeFracX, const float fadeFracY )
{
    Matrix4f mvpMatrix( mvp );

    glUseProgram( d->overlayScreenFadeMaskProgram.program );

    glUniformMatrix4fv( d->overlayScreenFadeMaskProgram.uMvp, 1, GL_FALSE, mvpMatrix.Transposed().M[0] );

    if ( d->fadedScreenMaskSquare.vertexArrayObject == 0 )
    {
        d->fadedScreenMaskSquare = BuildFadedScreenMask( fadeFracX, fadeFracY );
    }

    glColorMask( 0.0f, 0.0f, 0.0f, 1.0f );
    d->fadedScreenMaskSquare.Draw();
    glColorMask( 1.0f, 1.0f, 1.0f, 1.0f );
}

NV_NAMESPACE_END
