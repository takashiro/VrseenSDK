#include "App.h"

#include <android/keycodes.h>
#include <math.h>
#include <jni.h>
#include <sstream>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include <3rdparty/stb/stb_image_write.h>

#include "api/VGlOperation.h"
#include "android/JniUtils.h"
#include "android/VOsBuild.h"

#include "VAlgorithm.h"
#include "BitmapFont.h"
#include "DebugLines.h"
#include "EyePostRender.h"
#include "GazeCursor.h"
#include "GazeCursorLocal.h"		// necessary to instantiate the gaze cursor
#include "GlTexture.h"
#include "GuiSys.h"
#include "GuiSysLocal.h"		// necessary to instantiate the gui system
#include "ModelView.h"
#include "SurfaceTexture.h"

#include "TypesafeNumber.h"
#include "VBasicmath.h"
#include "VolumePopup.h"
#include "api/VDevice.h"
#include "api/VFrameSmooth.h"
#include "api/VKernel.h"
#include "VrLocale.h"
#include "VRMenuMgr.h"
#include "VUserSettings.h"

#include "VApkFile.h"
#include "VJson.h"
#include "VLog.h"
#include "VMainActivity.h"
#include "VThread.h"
#include "VStandardPath.h"

//#define TEST_TIMEWARP_WATCHDOG
#define EGL_PROTECTED_CONTENT_EXT 0x32c0

NV_NAMESPACE_BEGIN

class VPointTracker
{
public:
    static const int DEFAULT_FRAME_RATE = 60;

    VPointTracker( float const rate = 0.1f ) :
        LastFrameTime( 0.0 ),
        Rate( 0.1f ),
        CurPosition( 0.0f ),
        FirstFrame( true )
    {
    }

    void        Update( double const curFrameTime, V3Vectf const & newPos )
    {
        double frameDelta = curFrameTime - LastFrameTime;
        LastFrameTime = curFrameTime;
        float const rateScale = static_cast< float >( frameDelta / ( 1.0 / static_cast< double >( DEFAULT_FRAME_RATE ) ) );
        float const rate = Rate * rateScale;
        if ( FirstFrame )
        {
            CurPosition = newPos;
        }
        else
        {
            V3Vectf delta = ( newPos - CurPosition ) * rate;
            if ( delta.Length() < 0.001f )
            {
                // don't allow a denormal to propagate from multiplications of very small numbers
                delta = V3Vectf( 0.0f );
            }
            CurPosition += delta;
        }
        FirstFrame = false;
    }

    void                Reset() { FirstFrame = true; }
    void                SetRate( float const r ) { Rate = r; }

    V3Vectf const & GetCurPosition() const { return CurPosition; }

private:
    double      LastFrameTime;
    float       Rate;
    V3Vectf CurPosition;
    bool        FirstFrame;
};

static const char * activityClassName = "com/vrseen/nervgear/VrActivity";

// some parameters from the intent can be empty strings, which cannot be represented as empty strings for sscanf
// so we encode them as EMPTY_INTENT_STR.
// Because the message queue handling uses sscanf() to parse the message, the JSON text is
// always placed at the end of the message because it can contain spaces while the package
// name and URI cannot. The handler will use sscanf() to parse the first two strings, then
// assume the JSON text is everything immediately following the space after the URI string.
static const char * EMPTY_INTENT_STR = "<EMPTY>";

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

static V3Vectf ViewOrigin(const VR4Matrixf & view)
{
    return V3Vectf(view.M[0][3], view.M[1][3], view.M[2][3]);
}

static V3Vectf ViewForward(const VR4Matrixf & view)
{
    return V3Vectf(-view.M[0][2], -view.M[1][2], -view.M[2][2]);
}

// Always make the panel upright, even if the head was tilted when created
static VR4Matrixf PanelMatrix(const VR4Matrixf & lastViewMatrix, const float popupDistance,
        const float popupScale, const int width, const int height)
{
    // TODO: this won't be valid until a frame has been rendered
    const VR4Matrixf invView = lastViewMatrix.Inverted();
    const V3Vectf forward = ViewForward(invView);
    const V3Vectf levelforward = V3Vectf(forward.x, 0.0f, forward.z).Normalized();
    // TODO: check degenerate case
    const V3Vectf up(0.0f, 1.0f, 0.0f);
    const V3Vectf right = levelforward.Cross(up);

    const V3Vectf center = ViewOrigin(invView) + levelforward * popupDistance;
    const float xScale = (float)width / 768.0f * popupScale;
    const float yScale = (float)height / 768.0f * popupScale;
    const VR4Matrixf panelMatrix = VR4Matrixf(
            xScale * right.x, yScale * up.x, forward.x, center.x,
            xScale * right.y, yScale * up.y, forward.y, center.y,
            xScale * right.z, yScale * up.z, forward.z, center.z,
            0, 0, 0, 1);

//	LOG("PanelMatrix center: %f %f %f", center.x, center.y, center.z);
//	LogMatrix("PanelMatrix", panelMatrix);

    return panelMatrix;
}

extern void DebugMenuBounds(void * appPtr, const char * cmd);
extern void DebugMenuHierarchy(void * appPtr, const char * cmd);
extern void DebugMenuPoses(void * appPtr, const char * cmd);
extern void ShowFPS(void * appPtr, const char * cmd);

static EyeParms DefaultVrParmsForRenderer(const VGlOperation & glOperation)
{
    EyeParms vrParms;

    vrParms.resolution = 1024;
    vrParms.multisamples = (glOperation.gpuType == VGlOperation::GPU_TYPE_ADRENO_330) ? 2 : 4;
    vrParms.colorFormat = COLOR_8888;
    vrParms.depthFormat = DEPTH_24;

    return vrParms;
}

static bool ChromaticAberrationCorrection(const VGlOperation & glOperation)
{
    return (glOperation.gpuType & VGlOperation::GPU_TYPE_ADRENO) != 0 && (glOperation.gpuType >= VGlOperation::GPU_TYPE_ADRENO_420);
}



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
    VEventLoop	eventLoop;

    // Egl context and surface for rendering
    VGlOperation  glOperation;

    // Handles creating, destroying, and re-configuring the buffers
    // for drawing the eye views, which might be in different texture
    // configurations for CPU warping, etc.
    EyeBuffers *	eyeTargets;

    GLuint			loadingIconTexId;

    JavaVM *		javaVM;

    JNIEnv *		uiJni;			// for use by the Java UI thread
    JNIEnv *		vrJni;			// for use by the VR thread

    jclass			vrActivityClass;		// must be looked up from main thread or FindClass() will fail

    jmethodID		createVrToastMethodId;
    jmethodID		clearVrToastsMethodId;
    jmethodID		playSoundPoolSoundMethodId;

    VString			launchIntentURI;			// URI app was launched with
    VString			launchIntentJSON;			// extra JSON data app was launched with
    VString			launchIntentFromPackage;	// package that sent us the launch intent

    VString			packageCodePath;	// path to apk to open as zip and load resources

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
    VR4Matrixf		dialogMatrix;

    VR4Matrixf		lastViewMatrix;

    ANativeWindow * nativeWindow;
    EGLSurface 		windowSurface;

    bool			drawCalibrationLines;	// currently toggled by right trigger
    bool			calibrationLinesDrawn;	// after draw, go to static time warp test
    bool			showVignette;			// render the vignette

    bool			framebufferIsSrgb;			// requires KHR_gl_colorspace
    bool			framebufferIsProtected;		// requires GPU trust zone extension

    // Only render a single eye view, which will get warped for both
    // screen eyes.
    bool			renderMonoMode;

    VrFrame			vrFrame;
    VrFrame			lastVrFrame;

    EyeParms		vrParms;

    ovrTimeWarpParms	swapParms;			// passed to TimeWarp->WarpSwap()

    VGlShader		externalTextureProgram2;
    VGlShader		untexturedMvpProgram;
    VGlShader		untexturedScreenSpaceProgram;
    VGlShader		overlayScreenFadeMaskProgram;
    VGlShader		overlayScreenDirectProgram;

    VGlGeometry		unitCubeLines;		// 12 lines that outline a 0 to 1 unit cube, intended to be scaled to cover bounds.
    VGlGeometry		panelGeometry;		// used for dialogs
    VGlGeometry		unitSquare;			// -1 to 1 in x and Y, 0 to 1 in texcoords
    VGlGeometry		fadedScreenMaskSquare;// faded screen mask for overlay rendering

    EyePostRender	eyeDecorations;

    ovrSensorState	sensorForNextWarp;

    VThread *renderThread;
    int				vrThreadTid;		// linux tid

    bool			showFPS;			// true to show FPS on screen
    bool			showVolumePopup;	// true to show volume popup when volume changes

    VrViewParms		viewParms;

    VString			infoText;			// informative text to show in front of the view
    V4Vectf		infoTextColor;		// color of info text
    V3Vectf		infoTextOffset;		// offset from center of screen in view space
    long long		infoTextEndFrame;	// time to stop showing text
    VPointTracker	infoTextPointTracker;	// smoothly tracks to text ideal location
    VPointTracker	fpsPointTracker;		// smoothly tracks to ideal FPS text location

    float 			touchpadTimer;
    V2Vectf		touchOrigin;
    float 			lastTouchpadTime;
    bool 			lastTouchDown;
    int 			touchState;

    bool			enableDebugOptions;	// enable debug key-commands for development testing

    long long 		recenterYawFrameStart;	// Enables reorient before sensor data is read.  Allows apps to reorient without having invalid orientation information for that frame.

    // Manages sound assets
     VSoundManager	soundManager;

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
    VMainActivity *appInterface;

    VMainActivity *activity;
    VKernel*        kernel;

    Private(App *self)
        : self(self)
        , vrThreadSynced(false)
        , createdSurface(false)
        , readyToExit(false)
        , eventLoop(100)
        , eyeTargets(nullptr)
        , loadingIconTexId(0)
        , javaVM(VrLibJavaVM)
        , uiJni(nullptr)
        , vrJni(nullptr)
        , vrActivityClass(nullptr)
        , createVrToastMethodId(nullptr)
        , clearVrToastsMethodId(nullptr)
        , playSoundPoolSoundMethodId(nullptr)
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
                vInfo("AppLocal::Init CallObjectMethod");
                vrJni->CallVoidMethod(javaObject, setDefaultLocaleId);
                if (vrJni->ExceptionOccurred())
                {
                    vrJni->ExceptionClear();
                    vWarn("Exception occurred in setDefaultLocale");
                }
                // re-get the font name for the new locale
                VrLocale::GetString(vrJni, javaObject, "@string/font_name", "efigs.fnt", fontName);
                fontName.prepend("res/raw/");
                // try to load the font
                if (!defaultFont->Load(packageCodePath, fontName))
                {
                    vFatal("Failed to load font for default locale!");
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

    void pause()
    {
        appInterface->onPause();

        kernel->exit();
    }

    void resume()
    {
        vInfo("OVRTimer AppLocal::Resume");

        // always reload the dev config on a resume
        JniUtils::LoadDevConfig(true);


        // Make sure the window surface is current, which it won't be
        // if we were previously in async mode
        // (Not needed now?)
        if (eglMakeCurrent(glOperation.display, windowSurface, windowSurface, glOperation.context) == EGL_FALSE)
        {
            vFatal("eglMakeCurrent failed:" << glOperation.getEglErrorString());
        }

        // Allow the app to override
        appInterface->configureVrMode(kernel);

        // Clear cursor trails
        gazeCursor->HideCursorForFrames(10);

        // Start up TimeWarp and the various performance options
        kernel->run();

        appInterface->onResume();
    }

    void initGlObjects()
    {
        vrParms = DefaultVrParmsForRenderer(glOperation);

        swapParms.WarpProgram = ChromaticAberrationCorrection(glOperation) ? WP_CHROMATIC : WP_SIMPLE;


        glOperation.logExtensions();

        externalTextureProgram2.initShader( VGlShader::getAdditionalVertexShaderSource(), VGlShader::getAdditionalFragmentShaderSource() );
        untexturedMvpProgram.initShader( VGlShader::getUntextureMvpVertexShaderSource(),VGlShader::getUntexturedFragmentShaderSource()  );
        untexturedScreenSpaceProgram.initShader( VGlShader::getUniformColorVertexShaderSource(), VGlShader::getUntexturedFragmentShaderSource() );
        overlayScreenFadeMaskProgram.initShader(VGlShader::getUntextureInverseColorVertexShaderSource(),VGlShader::getUntexturedFragmentShaderSource() );
        overlayScreenDirectProgram.initShader(VGlShader::getSingleTextureVertexShaderSource(),VGlShader::getSingleTextureFragmentShaderSource() );


        panelGeometry.createPlaneQuadGrid( 32, 16 );
        unitSquare.createPlaneQuadGrid(1, 1 );
        unitCubeLines.createUnitCubeGrid();


        eyeDecorations.Init();
    }

    void shutdownGlObjects()
    {
        externalTextureProgram2.destroy();
        untexturedMvpProgram.destroy();
        untexturedScreenSpaceProgram.destroy();
        overlayScreenFadeMaskProgram.destroy();
        overlayScreenDirectProgram.destroy();

        panelGeometry.destroy();
        unitSquare.destroy();
        unitCubeLines.destroy();
        fadedScreenMaskSquare.destroy();

        eyeDecorations.Shutdown();
    }

    void interpretTouchpad(VrInput &input)
    {
        // 1) Down -> Up w/ Motion = Slide
        // 2) Down -> Up w/out Motion -> Timeout = Single Tap
        // 3) Down -> Up w/out Motion -> Down -> Timeout = Nothing
        // 4) Down -> Up w/out Motion -> Down -> Up = Double Tap
        static const float timer_finger_down = 0.3f;
        static const float timer_finger_up = 0.3f;
        static const float min_swipe_distance = 100.0f;

        float currentTime = ovr_GetTimeInSeconds();
        float deltaTime = currentTime - lastTouchpadTime;
        lastTouchpadTime = currentTime;
        touchpadTimer = touchpadTimer + deltaTime;

        bool down = false, up = false;
        bool currentTouchDown = input.buttonState & BUTTON_TOUCH;

        if (currentTouchDown && !lastTouchDown)
        {
            //CreateToast("DOWN");
            down = true;
            touchOrigin = input.touch;
        }

        if (!currentTouchDown && lastTouchDown)
        {
            //CreateToast("UP");
            up = true;
        }

        lastTouchDown = currentTouchDown;

        input.touchRelative = input.touch - touchOrigin;
        float touchMagnitude = input.touchRelative.Length();
        input.swipeFraction = touchMagnitude / min_swipe_distance;

        switch (touchState)
        {
        case 0:
            //CreateToast("0 - %f", touchpadTimer);
            if (down)
            {
                touchState = 1;
                touchpadTimer = 0.0f;
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
                touchState = 0;
                touchpadTimer = 0.0f;
            }
            else if (up)
            {
                if (touchpadTimer < timer_finger_down)
                {
                    touchState = 2;
                    touchpadTimer = 0.0f;
                }
                else
                {
                    //CreateToast("SINGLE TOUCH");
                    input.buttonPressed |= BUTTON_TOUCH_SINGLE;
                    input.buttonReleased |= BUTTON_TOUCH_SINGLE;
                    input.buttonState |= BUTTON_TOUCH_SINGLE;
                    touchState = 0;
                    touchpadTimer = 0.0f;
                }
            }
            break;
        case 2:
            //CreateToast("2 - %f", touchpadTimer);
            if (touchpadTimer >= timer_finger_up)
            {
                //CreateToast("SINGLE TOUCH");
                input.buttonPressed |= BUTTON_TOUCH_SINGLE;
                input.buttonReleased |= BUTTON_TOUCH_SINGLE;
                input.buttonState |= BUTTON_TOUCH_SINGLE;
                touchState = 0;
                touchpadTimer = 0.0f;
            }
            else if (down)
            {
                touchState = 3;
                touchpadTimer = 0.0f;
            }
            break;
        case 3:
            //CreateToast("3 - %f", touchpadTimer);
            if (touchpadTimer >= timer_finger_down)
            {
                touchState = 0;
                touchpadTimer = 0.0f;
            }
            else if (up)
            {
                //CreateToast("DOUBLE TOUCH");
                input.buttonPressed |= BUTTON_TOUCH_DOUBLE;
                input.buttonReleased |= BUTTON_TOUCH_DOUBLE;
                input.buttonState |= BUTTON_TOUCH_DOUBLE;
                touchState = 0;
                touchpadTimer = 0.0f;
            }
            break;
        }
    }

    void frameworkButtonProcessing(const VrInput &input)
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
            vInfo("QAEvent " << timeStr << " (" << ovr_GetTimeInSeconds() << ") - QA event occurred");
        }

        // Display tweak testing, only when holding right trigger
        if (enableDebugOptions && rightTrigger)
        {
            if (input.buttonPressed & BUTTON_DPAD_RIGHT)
            {
                jclass vmDebugClass = vrJni->FindClass("dalvik/system/VMDebug");
                jmethodID dumpId = vrJni->GetStaticMethodID(vmDebugClass, "dumpReferenceTables", "()V");
                vrJni->CallStaticVoidMethod(vmDebugClass, dumpId);
                vrJni->DeleteLocalRef(vmDebugClass);
            }

            if (input.buttonPressed & BUTTON_Y)
            {	// display current scheduler state and clock rates
                //const char * str = ovr_CreateSchedulingReport(OvrMobile);
                //CreateToast("%s", str);
            }

            if (input.buttonPressed & BUTTON_B)
            {
                if (swapParms.WarpOptions & SWAP_OPTION_USE_SLICED_WARP)
                {
                    swapParms.WarpOptions &= ~SWAP_OPTION_USE_SLICED_WARP;
                    self->createToast("eye warp");
                }
                else
                {
                    swapParms.WarpOptions |= SWAP_OPTION_USE_SLICED_WARP;
                    self->createToast("slice warp");
                }
            }

            if (swapParms.WarpOptions & SWAP_OPTION_USE_SLICED_WARP)
            {
                extern float calibrateFovScale;

                if (input.buttonPressed & BUTTON_DPAD_LEFT)
                {
                    swapParms.PreScheduleSeconds -= 0.001f;
                    self->createToast("Schedule: %f", swapParms.PreScheduleSeconds);
                }
                if (input.buttonPressed & BUTTON_DPAD_RIGHT)
                {
                    swapParms.PreScheduleSeconds += 0.001f;
                    self->createToast("Schedule: %f", swapParms.PreScheduleSeconds);
                }
                if (input.buttonPressed & BUTTON_DPAD_UP)
                {
                    calibrateFovScale -= 0.01f;
                    self->createToast("calibrateFovScale: %f", calibrateFovScale);
                    pause();
                    resume();
                }
                if (input.buttonPressed & BUTTON_DPAD_DOWN)
                {
                    calibrateFovScale += 0.01f;
                    self->createToast("calibrateFovScale: %f", calibrateFovScale);
                    pause();
                    resume();
                }
            }
        }
    }

    void command(const VEvent &event)
    {
        // Always include the space in MatchesHead to prevent problems
        // with commands that have matching prefixes.

        if (event.name == "joy") {
            vAssert(event.data.isArray());
            joypad.sticks[0][0] = event.data.at(0).toFloat();
            joypad.sticks[0][1] = event.data.at(1).toFloat();
            joypad.sticks[1][0] = event.data.at(2).toFloat();
            joypad.sticks[1][1] = event.data.at(3).toFloat();
            return;
        }

        if (event.name == "touch") {
            vAssert(event.data.isArray());
            int	action = event.data.at(0).toInt();
            joypad.touch[0] = event.data.at(1).toFloat();
            joypad.touch[1] = event.data.at(2).toFloat();
            if (action == 0) {
                joypad.buttonState |= BUTTON_TOUCH;
            }
            if (action == 1) {
                joypad.buttonState &= ~BUTTON_TOUCH;
            }
            return;
        }

        if (event.name == "key") {
            vAssert(event.data.isArray());
            int	key = event.data.at(0).toInt();
            int down = event.data.at(1).toInt();
            int repeatCount = event.data.at(2).toInt();
            onKeyEvent(key, down, repeatCount);
            return;
        }

        if (event.name == "surfaceChanged") {
            vInfo(event.name);
            if (windowSurface != EGL_NO_SURFACE)
            {	// Samsung says this is an Android problem, where surfaces are reported as
                // created multiple times.
                vWarn("Skipping create work because window hasn't been destroyed.");
                return;
            }
            nativeWindow = static_cast<ANativeWindow *>(event.data.toPointer());

            EGLint attribs[100];
            int		numAttribs = 0;

            // Set the colorspace on the window
            windowSurface = EGL_NO_SURFACE;
            if (appInterface->wantSrgbFramebuffer())
            {
                attribs[numAttribs++] = VGlOperation::EGL_GL_COLORSPACE_KHR;
                attribs[numAttribs++] = VGlOperation::EGL_GL_COLORSPACE_SRGB_KHR;
            }
            // Ask for TrustZone rendering support
            if (appInterface->wantProtectedFramebuffer())
            {
                attribs[numAttribs++] = EGL_PROTECTED_CONTENT_EXT;
                attribs[numAttribs++] = EGL_TRUE;
            }
            attribs[numAttribs++] = EGL_NONE;

            // Android doesn't let the non-standard extensions show up in the
            // extension string, so we need to try it blind.
            windowSurface = eglCreateWindowSurface(glOperation.display, glOperation.config,
                    nativeWindow, attribs);


            if (windowSurface == EGL_NO_SURFACE)
            {
                const EGLint attribs2[] =
                {
                    EGL_NONE
                };
                windowSurface = eglCreateWindowSurface(glOperation.display, glOperation.config,
                        nativeWindow, attribs2);
                if (windowSurface == EGL_NO_SURFACE)
                {
                    FAIL("eglCreateWindowSurface failed: %s", glOperation.getEglErrorString());
                }
                framebufferIsSrgb = false;
                framebufferIsProtected = false;
            }
            else
            {
                framebufferIsSrgb = appInterface->wantSrgbFramebuffer();
                framebufferIsProtected = appInterface->wantProtectedFramebuffer();
            }

            if (eglMakeCurrent(glOperation.display, windowSurface, windowSurface, glOperation.context) == EGL_FALSE)
            {
                vFatal("eglMakeCurrent failed:" << glOperation.getEglErrorString());
            }

            createdSurface = true;

            // Let the client app setup now
            appInterface->onWindowCreated();

            // Resume
            if (!paused)
            {
                resume();
            }
            return;
        }

        if (event.name == "surfaceDestroyed") {
            vInfo("surfaceDestroyed");

            // Let the client app shutdown first.
            appInterface->onWindowDestroyed();

            // Handle it ourselves.
            if (eglMakeCurrent(glOperation.display, glOperation.pbufferSurface, glOperation.pbufferSurface,
                    glOperation.context) == EGL_FALSE)
            {
                vFatal("RC_SURFACE_DESTROYED: eglMakeCurrent pbuffer failed");
            }

            if (windowSurface != EGL_NO_SURFACE)
            {
                eglDestroySurface(glOperation.display, windowSurface);
                windowSurface = EGL_NO_SURFACE;
            }
            if (nativeWindow != nullptr)
            {
                ANativeWindow_release(nativeWindow);
                nativeWindow = nullptr;
            }
            return;
        }

        if (event.name == "pause") {
            vInfo("pause");
            if (!paused)
            {
                paused = true;
                pause();
            }
        }

        if (event.name == "resume") {
            LOG("resume");
            paused = false;
            // Don't actually do the resume operations if we don't have
            // a window yet.  They will be done when the window is created.
            if (windowSurface != EGL_NO_SURFACE)
            {
                resume();
            }
            else
            {
                vInfo("Skipping resume because windowSurface not set yet");
            }
        }

        if (event.name == "intent") {
            vAssert(event.data.isArray());
            VString fromPackageName = event.data.at(0).toString();
            VString uri = event.data.at(1).toString();
            VString json = event.data.at(2).toString();

            if (fromPackageName == EMPTY_INTENT_STR) {
                fromPackageName.clear();
            }
            if (uri == EMPTY_INTENT_STR) {
                uri.clear();
            }

            // assign launchIntent to the intent command
            launchIntentFromPackage = fromPackageName;
            launchIntentJSON = json;
            launchIntentURI = uri;

            // when the PlatformActivity is launched, this is how it gets its command to start
            // a particular UI.
            appInterface->onNewIntent(fromPackageName, json, uri);
            return;
        }

        if (event.name == "popup") {
            vAssert(event.data.isArray());
            int width = event.data.at(0).toInt();
            int height = event.data.at(1).toInt();
            float seconds = event.data.at(2).toFloat();

            dialogWidth = width;
            dialogHeight = height;
            dialogStopSeconds = ovr_GetTimeInSeconds() + seconds;

            dialogMatrix = PanelMatrix(lastViewMatrix, popupDistance, popupScale, width, height);

            glActiveTexture(GL_TEXTURE0);
            vInfo("RC_UPDATE_POPUP dialogTexture" << dialogTexture->textureId);
            dialogTexture->Update();
            glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

            return;
        }

        if (event.name == "sync") {
            return;
        }

        if (event.name == "quit") {
            kernel->exit();
            readyToExit = true;
            vInfo("VrThreadSynced=" << vrThreadSynced << " CreatedSurface=" << createdSurface << " ReadyToExit=" << readyToExit);
        }

        // Pass it on to the client app.
        appInterface->command(event);
    }

    void startRendering()
    {

        // Set the name that will show up in systrace
        pthread_setname_np(pthread_self(), "NervGear::VrThread");

        // Initialize the VR thread
        {
            vInfo("AppLocal::VrThreadFunction - init");

            // Get the tid for setting the scheduler
            vrThreadTid = gettid();

            // The Java VM needs to be attached on each thread that will use
            // it.  We need it to call UpdateTexture on surfaceTextures, which
            // must be done on the thread with the openGL context that created
            // the associated texture object current.
            vInfo("javaVM->AttachCurrentThread");
            const jint rtn = javaVM->AttachCurrentThread(&vrJni, 0);
            if (rtn != JNI_OK)
            {
                FAIL("javaVM->AttachCurrentThread returned %i", rtn);
            }

            // Set up another thread for making longer-running java calls
            // to avoid hitches.
            activity->Init(javaVM);

            // Create a new context and pbuffer surface
            const int windowDepth = 0;
            const int windowSamples = 0;
            const GLuint contextPriority = EGL_CONTEXT_PRIORITY_MEDIUM_IMG;
            glOperation.eglInit(EGL_NO_CONTEXT, GL_ES_VERSION,	// no share context,
                    8,8,8, windowDepth, windowSamples, // r g b
                    contextPriority);

            // Create our GL data objects
            initGlObjects();

            eyeTargets = new EyeBuffers;
            guiSys = new OvrGuiSysLocal;
            gazeCursor = new OvrGazeCursorLocal;
            vrMenuMgr = OvrVRMenuMgr::Create();
            debugLines = OvrDebugLines::Create();

            int w = 0;
            int h = 0;
            loadingIconTexId = LoadTextureFromApplicationPackage("res/raw/loading_indicator.png",
                                            TextureFlags_t(TEXTUREFLAG_NO_MIPMAPS), w, h);

            // Create the SurfaceTexture for dialog rendering.
            dialogTexture = new SurfaceTexture(vrJni);

            initFonts();

            soundManager.loadSoundAssets();

            debugLines->Init();

            gazeCursor->Init();

            vrMenuMgr->init();

            guiSys->init(self, *vrMenuMgr, *defaultFont);

            volumePopup = OvrVolumePopup::Create(self, *vrMenuMgr, *defaultFont);

            lastTouchpadTime = ovr_GetTimeInSeconds();
        }

        // FPS counter information
        int countApplicationFrames = 0;
        double lastReportTime = ceil(ovr_GetTimeInSeconds());

        while(!(vrThreadSynced && createdSurface && readyToExit))
        {
            //SPAM("FRAME START");

            gazeCursor->BeginFrame();

            // Process incoming messages until queue is empty
            forever {
                VEvent event = eventLoop.next();
                if (!event.isValid()) {
                    break;
                }
                command(event);
            }

            // handle any pending system activity events
            size_t const MAX_EVENT_SIZE = 4096;
            VString eventBuffer;

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
                VJson jsonObj;
                s >> jsonObj;
                if (jsonObj.type() == VJson::Object)
                {
                    VString command = jsonObj["Command"].toString();
                    if (command == SYSTEM_ACTIVITY_EVENT_REORIENT)
                    {
                        // for reorient, we recenter yaw natively, then pass the event along so that the client
                        // application can also handle the event (for instance, to reposition menus)
                        vInfo("VtThreadFunction: Acting on System Activity reorient event.");
                        self->recenterYaw(false);
                    }
                    else
                    {
                        // In the case of the returnToLauncher event, we always handler it internally and pass
                        // along an empty buffer so that any remaining events still get processed by the client.
                        vInfo("Unhandled System Activity event:" << command);
                    }
                }
                else
                {
                    // a malformed event string was pushed! This implies an error in the native code somewhere.
                    WARN("Error parsing System Activities Event: %s");
                }
            }

            // update volume popup
            if (showVolumePopup)
            {
                volumePopup->checkForVolumeChange(self);
            }

            // If we don't have a surface yet, or we are paused, sleep until
            // something shows up on the message queue.
            if (windowSurface == EGL_NO_SURFACE || paused)
            {
                if (!(vrThreadSynced && createdSurface && readyToExit))
                {
                    eventLoop.wait();
                }
                continue;
            }

            // if there is an error condition, warp swap and nothing else
            if (errorTexture != 0)
            {
                if (ovr_GetTimeInSeconds() >= errorMessageEndTime)
                {
                    kernel->destroy(EXIT_TYPE_FINISH_AFFINITY);
                }
                else
                {
                    ovrTimeWarpParms warpSwapMessageParms = kernel->InitTimeWarpParms(WARP_INIT_MESSAGE, errorTexture.texture);
                    warpSwapMessageParms.ProgramParms[0] = 0.0f;						// rotation in radians
                    warpSwapMessageParms.ProgramParms[1] = 1024.0f / errorTextureSize;	// message size factor
                    kernel->doSmooth(&warpSwapMessageParms);
                }
                continue;
            }

            // Let the client app initialize only once by calling OneTimeInit() when the windowSurface is valid.
            if (!self->oneTimeInitCalled)
            {
                if (appInterface->showLoadingIcon())
                {
                    const ovrTimeWarpParms warpSwapLoadingIconParms = kernel->InitTimeWarpParms(WARP_INIT_LOADING_ICON, loadingIconTexId);
                    kernel->doSmooth(&warpSwapLoadingIconParms);
                }
                vInfo("launchIntentJSON:" << launchIntentJSON);
                vInfo("launchIntentURI:" << launchIntentURI);

                appInterface->init(launchIntentFromPackage, launchIntentJSON, launchIntentURI);
                self->oneTimeInitCalled = true;
            }

            // latch the current joypad state and note transitions
            vrFrame.Input = joypad;
            vrFrame.Input.buttonPressed = joypad.buttonState & (~lastVrFrame.Input.buttonState);
            vrFrame.Input.buttonReleased = ~joypad.buttonState & (lastVrFrame.Input.buttonState & ~BUTTON_TOUCH_WAS_SWIPE);

            if (lastVrFrame.Input.buttonState & BUTTON_TOUCH_WAS_SWIPE)
            {
                if (lastVrFrame.Input.buttonReleased & BUTTON_TOUCH)
                {
                    vrFrame.Input.buttonReleased |= BUTTON_TOUCH_WAS_SWIPE;
                }
                else
                {
                    // keep it around this frame
                    vrFrame.Input.buttonState |= BUTTON_TOUCH_WAS_SWIPE;
                }
            }

            // Synthesize swipe gestures
            interpretTouchpad(vrFrame.Input);

            if (recenterYawFrameStart != 0)
            {
                // Perform a reorient before sensor data is read.  Allows apps to reorient without having invalid orientation information for that frame.
                // Do a warp swap black on the frame the recenter started.
                self->recenterYaw(recenterYawFrameStart == (vrFrame.FrameNumber + 1));  // vrFrame.FrameNumber hasn't been incremented yet, so add 1.
            }

            // Get the latest head tracking state, predicted ahead to the midpoint of the time
            // it will be displayed.  It will always be corrected to the real values by
            // time warp, but the closer we get, the less black will be pulled in at the edges.
            const double now = ovr_GetTimeInSeconds();
            static double prev;
            const double rawDelta = now - prev;
            prev = now;
            const double clampedPrediction = std::min(0.1, rawDelta * 2);
            sensorForNextWarp = kernel->ovr_GetPredictedSensorState(now + clampedPrediction);

            vrFrame.PoseState = sensorForNextWarp.Predicted;
            vrFrame.OvrStatus = sensorForNextWarp.Status;
            vrFrame.DeltaSeconds   = std::min(0.1, rawDelta);
            vrFrame.FrameNumber++;

            // Don't allow this to be excessively large, which can cause application problems.
            if (vrFrame.DeltaSeconds > 0.1f)
            {
                vrFrame.DeltaSeconds = 0.1f;
            }

            lastVrFrame = vrFrame;

            // resend any debug lines that have expired
            debugLines->BeginFrame(vrFrame.FrameNumber);

            // reset any VR menu submissions from previous frame
            vrMenuMgr->beginFrame();

            frameworkButtonProcessing(vrFrame.Input);

            KeyState::eKeyEventType event = backKeyState.Update(ovr_GetTimeInSeconds());
            if (event != KeyState::KEY_EVENT_NONE)
            {
                //LOG("BackKey: event %s", KeyState::EventNames[ event ]);
                // always allow the gaze cursor to peek at the event so it can start the gaze timer if necessary
                // update the gaze cursor timer
                if (event == KeyState::KEY_EVENT_DOWN)
                {
                    gazeCursor->StartTimer(backKeyState.GetLongPressTime(), backKeyState.GetDoubleTapTime());
                }
                else if (event == KeyState::KEY_EVENT_DOUBLE_TAP || event == KeyState::KEY_EVENT_SHORT_PRESS)
                {
                    gazeCursor->CancelTimer();
                }
                else if (event == KeyState::KEY_EVENT_LONG_PRESS)
                {
                    //StartSystemActivity(PUI_GLOBAL_MENU);
                    kernel->destroy(EXIT_TYPE_FINISH);
                }

                // let the menu handle it if it's open
                bool consumedKey = guiSys->onKeyEvent(self, AKEYCODE_BACK, event);

                // pass to the app if nothing handled it before this
                if (!consumedKey)
                {
                    consumedKey = appInterface->onKeyEvent(AKEYCODE_BACK, event);
                }
                // if nothing consumed the key and it's a short-press, exit the application to OculusHome
                if (!consumedKey)
                {
                    if (event == KeyState::KEY_EVENT_SHORT_PRESS)
                    {
                        consumedKey = true;
                        LOG("BUTTON_BACK: confirming quit in platformUI");
                        kernel->destroy(EXIT_TYPE_FINISH);
                    }
                }
            }

            if (showFPS)
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

                V3Vectf viewPos = GetViewMatrixPosition(lastViewMatrix);
                V3Vectf viewFwd = GetViewMatrixForward(lastViewMatrix);
                V3Vectf newPos = viewPos + viewFwd * 1.5f;
                fpsPointTracker.Update(ovr_GetTimeInSeconds(), newPos);

                fontParms_t fp;
                fp.AlignHoriz = HORIZONTAL_CENTER;
                fp.Billboard = true;
                fp.TrackRoll = false;
                worldFontSurface->DrawTextBillboarded3Df(*defaultFont, fp, fpsPointTracker.GetCurPosition(),
                        0.8f, V4Vectf(1.0f, 0.0f, 0.0f, 1.0f), "%.1f fps", LastFrameRate);
                LastFrameTime = currentFrameTime;
            }


            // draw info text
            if (infoTextEndFrame >= vrFrame.FrameNumber)
            {
                V3Vectf viewPos = GetViewMatrixPosition(lastViewMatrix);
                V3Vectf viewFwd = GetViewMatrixForward(lastViewMatrix);
                V3Vectf viewUp(0.0f, 1.0f, 0.0f);
                V3Vectf viewLeft = viewUp.Cross(viewFwd);
                V3Vectf newPos = viewPos + viewFwd * infoTextOffset.z + viewUp * infoTextOffset.y + viewLeft * infoTextOffset.x;
                infoTextPointTracker.Update(ovr_GetTimeInSeconds(), newPos);

                fontParms_t fp;
                fp.AlignHoriz = HORIZONTAL_CENTER;
                fp.AlignVert = VERTICAL_CENTER;
                fp.Billboard = true;
                fp.TrackRoll = false;
                worldFontSurface->DrawTextBillboarded3Df(*defaultFont, fp, infoTextPointTracker.GetCurPosition(),
                        1.0f, infoTextColor, infoText.toCString());
            }

            // Main loop logic / draw code
            if (!readyToExit)
            {
                lastViewMatrix = appInterface->onNewFrame(vrFrame);
            }

            kernel->ovr_HandleDeviceStateChanges();

            // MWC demo hack to allow keyboard swipes
            joypad.buttonState &= ~(BUTTON_SWIPE_FORWARD|BUTTON_SWIPE_BACK);

            // Report frame counts once a second
            countApplicationFrames++;
            const double timeNow = floor(ovr_GetTimeInSeconds());
            if (timeNow > lastReportTime)
            {
                vInfo("FPS:" << countApplicationFrames << "GPU time:" << eyeTargets->LogEyeSceneGpuTime.GetTotalTime() << "ms");
                countApplicationFrames = 0;
                lastReportTime = timeNow;
            }

            //SPAM("FRAME END");
        }

        // Shutdown the VR thread
        {
            vInfo("AppLocal::VrThreadFunction - shutdown");

            // Shut down the message queue so it cannot overflow.
            eventLoop.quit();

            if (errorTexture != 0)
            {
                FreeTexture(errorTexture);
            }

            appInterface->shutdown();

            guiSys->shutdown(*vrMenuMgr);

            vrMenuMgr->shutdown();

            gazeCursor->Shutdown();

            debugLines->Shutdown();

            shutdownFonts();

            delete dialogTexture;
            dialogTexture = nullptr;

            delete eyeTargets;
            eyeTargets = nullptr;

            delete guiSys;
            guiSys = nullptr;

            delete gazeCursor;
            gazeCursor = nullptr;

            OvrVRMenuMgr::Free(vrMenuMgr);
            OvrDebugLines::Free(debugLines);

            shutdownGlObjects();

            glOperation.eglExit();

            // Detach from the Java VM before exiting.
            vInfo("javaVM->DetachCurrentThread");
            const jint rtn = javaVM->DetachCurrentThread();
            if (rtn != JNI_OK)
            {
                vInfo("javaVM->DetachCurrentThread returned" << rtn);
            }
        }
    }

    jclass getGlobalClassReference(const char * className) const
    {
        jclass lc = uiJni->FindClass(className);
        if (lc == 0) {
            vFatal("Failed to find class" << className);
        }
        // Turn it into a global ref, so we can safely use it in the VR thread
        jclass gc = (jclass) uiJni->NewGlobalRef(lc);
        uiJni->DeleteLocalRef(lc);
        return gc;
    }


    void onKeyEvent(const int keyCode, const bool down, const int repeatCount)
    {
        // the back key is special because it has to handle long-press and double-tap
        if (keyCode == AKEYCODE_BACK)
        {
            //DROIDLOG("BackKey", "BACK KEY PRESSED");
            // back key events, because of special handling for double-tap, short-press and long-press,
            // are handled in AppLocal::VrThreadFunction.
            backKeyState.HandleEvent(ovr_GetTimeInSeconds(), down, repeatCount);
            return;
        }

        // the app menu is always the first consumer so it cannot be usurped
        bool consumedKey = false;
        if (repeatCount == 0)
        {
            consumedKey = guiSys->onKeyEvent(self, keyCode, down ? KeyState::KEY_EVENT_DOWN : KeyState::KEY_EVENT_UP);
        }

        // for all other keys, allow VrAppInterface the chance to handle and consume the key first
        if (!consumedKey)
        {
            consumedKey = appInterface->onKeyEvent(keyCode, down ? KeyState::KEY_EVENT_DOWN : KeyState::KEY_EVENT_UP);
        }

        // ALL VRLIB KEY HANDLING OTHER THAN APP MENU SHOULD GO HERE
        if (!consumedKey && enableDebugOptions)
        {
            float const IPD_STEP = 0.001f;

            // FIXME: this should set consumedKey = true now that we pass key events via appInterface first,
            // but this would likely break some apps right now that rely on the old behavior
            // consumedKey = true;

            if (down && keyCode == AKEYCODE_RIGHT_BRACKET)
            {
                vInfo("BUTTON_SWIPE_FORWARD");
                joypad.buttonState |= BUTTON_SWIPE_FORWARD;
                return;
            }
            else if (down && keyCode == AKEYCODE_LEFT_BRACKET)
            {
                vInfo("BUTTON_SWIPE_BACK");
                joypad.buttonState |= BUTTON_SWIPE_BACK;
                return;
            }
            else if (keyCode == AKEYCODE_S)
            {
                if (repeatCount == 0 && down) // first down only
                {
                    eyeTargets->ScreenShot();
                    self->createToast("screenshot");
                    return;
                }
            }
            else if (keyCode == AKEYCODE_F && down && repeatCount == 0)
            {
                self->setShowFPS(showFPS);
                return;
            }
            else if (keyCode == AKEYCODE_COMMA && down && repeatCount == 0)
            {
                float const IPD_MIN_CM = 0.0f;
                viewParms.InterpupillaryDistance = std::max(IPD_MIN_CM * 0.01f, viewParms.InterpupillaryDistance - IPD_STEP);
                self->showInfoText(1.0f, "%.3f", viewParms.InterpupillaryDistance);
                return;
            }
            else if (keyCode == AKEYCODE_PERIOD && down && repeatCount == 0)
            {
                float const IPD_MAX_CM = 8.0f;
                viewParms.InterpupillaryDistance = std::min(IPD_MAX_CM * 0.01f, viewParms.InterpupillaryDistance + IPD_STEP);
                self->showInfoText(1.0f, "%.3f", viewParms.InterpupillaryDistance);
                return;
            }
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
                    joypad.buttonState |= 1<<i;
                }
                else
                {
                    joypad.buttonState &= ~(1<<i);
                }
                return;
            }
        }
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

App *NervGearAppInstance = nullptr;

App::App(JNIEnv *jni, jobject activityObject, VMainActivity *activity)
    : oneTimeInitCalled(false)
    , d(new Private(this))
{
    d->activity = activity;

    d->uiJni = jni;
    vInfo("----------------- AppLocal::AppLocal() -----------------");
    vAssert(NervGearAppInstance == nullptr);
    NervGearAppInstance = this;

    d->kernel = VKernel::GetInstance();
    d->storagePaths = new VStandardPath(jni, activityObject);

	//WaitForDebuggerToAttach();

    memset(& d->sensorForNextWarp, 0, sizeof(d->sensorForNextWarp));

    d->sensorForNextWarp.Predicted.Pose.Orientation = VQuatf();

    JniUtils::LoadDevConfig(false);

	// Default time warp parms
    d->swapParms = d->kernel->InitTimeWarpParms();

	// Default EyeParms
    d->vrParms.resolution = 1024;
    d->vrParms.multisamples = 4;
    d->vrParms.colorFormat = COLOR_8888;
    d->vrParms.depthFormat = DEPTH_24;

    d->javaObject = d->uiJni->NewGlobalRef(activityObject);

	// A difficulty with JNI is that we can't resolve our (non-Android) package
	// classes on other threads, so lookup everything we need right now.
    d->vrActivityClass = d->getGlobalClassReference(activityClassName);
    VrLocale::VrActivityClass = d->vrActivityClass;

    d->createVrToastMethodId = d->GetMethodID("createVrToastOnUiThread", "(Ljava/lang/String;)V");
    d->clearVrToastsMethodId = d->GetMethodID("clearVrToasts", "()V");
    d->playSoundPoolSoundMethodId = d->GetMethodID("playSoundPoolSound", "(Ljava/lang/String;)V");

	// Get the path to the .apk and package name
    d->packageCodePath = d->activity->getPackageCodePath();

	// Hook the App and AppInterface together
    d->appInterface = activity;

	// Load user profile data relevant to rendering
    VUserSettings config;
    config.load();
    d->viewParms.InterpupillaryDistance = config.ipd;
    d->viewParms.EyeHeight = config.eyeHeight;
    d->viewParms.HeadModelDepth = config.headModelDepth;
    d->viewParms.HeadModelHeight = config.headModelHeight;

    d->renderThread = new VThread([](void *data)->int{
        App::Private *d = static_cast<App::Private *>(data);
        d->startRendering();
        return 0;
    }, d);
}

App::~App()
{
    vInfo("---------- ~AppLocal() ----------");

    if (d->javaObject != 0)
	{
        //d->uiJni->DeleteGlobalRef(d->javaObject);
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
    d->renderThread->start();
}

void App::stopVrThread()
{
    d->eventLoop.post("quit");
    bool finished = d->renderThread->wait();
    if (!finished) {
        vWarn("failed to wait for VrThread");
	}
}

void App::syncVrThread()
{
    d->eventLoop.send("sync");
    d->vrThreadSynced = true;
}

VEventLoop &App::eventLoop()
{
    return d->eventLoop;
}

void App::createToast(const char * fmt, ...)
{
	char bigBuffer[4096];
	va_list	args;
    va_start(args, fmt);
    vsnprintf(bigBuffer, sizeof(bigBuffer), fmt, args);
    va_end(args);

    JNIEnv *jni = nullptr;
    jstring cmdString = nullptr;
    if (d->javaVM->AttachCurrentThread(&jni, nullptr) == JNI_OK) {
        cmdString = JniUtils::Convert(jni, bigBuffer);

        d->activity->eventLoop().post([=]{
            JNIEnv *jni = nullptr;
            if (d->javaVM->AttachCurrentThread(&jni, nullptr) == JNI_OK) {
                jni->CallVoidMethod(d->javaObject, d->createVrToastMethodId, cmdString);
            }
            jni->DeleteLocalRef(cmdString);
        });
    }
}

void App::playSound(const char *name)
{
	// Get sound from SoundManager
	VString soundFile;
    if (!d->soundManager.getSound(name, soundFile)) {
        soundFile = VString::fromUtf8(name);
    }

    d->activity->eventLoop().post([=]{
        JNIEnv *jni = nullptr;
        if (d->javaVM->AttachCurrentThread(&jni, 0) == JNI_OK) {
            jstring cmdString = JniUtils::Convert(jni, soundFile);
            jni->CallVoidMethod(d->javaObject, d->playSoundPoolSoundMethodId, cmdString);
            jni->DeleteLocalRef(cmdString);
        }
    });
}

//void ToggleScreenColor()
//{
//    VGlOperation glOperation;
//	static int	color;

//	color ^= 1;

//    glEnable(GL_WRITEONLY_RENDERING_QCOM);
//    glClearColor(color, 1-color, 0, 1);
//    glClear(GL_COLOR_BUFFER_BIT);

//	// The Adreno driver has an unfortunate optimization so it doesn't
//	// actually flush if all that was done was a clear.
//    glOperation.GL_Finish();
//    glDisable(GL_WRITEONLY_RENDERING_QCOM);
//}

/*
 * eyeParms()
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

VR4Matrixf App::matrixInterpolation(const VR4Matrixf & startMatrix, const VR4Matrixf & endMatrix, double t)
{
    VR4Matrixf result;
    VQuat<float> startQuat = (VQuat<float>) startMatrix ;
    VQuat<float> endQuat = (VQuat<float>) endMatrix ;
    VQuat<float> quatResult;

	double cosHalfTheta = startQuat.w * endQuat.w +
						  startQuat.x * endQuat.x +
						  startQuat.y * endQuat.y +
						  startQuat.z * endQuat.z;

    if(fabs(cosHalfTheta) >= 1.0)
	{
		result = startMatrix;

        V3Vect<float> startTrans(startMatrix.M[0][3], startMatrix.M[1][3], startMatrix.M[2][3]);
        V3Vect<float> endTrans(endMatrix.M[0][3], endMatrix.M[1][3], endMatrix.M[2][3]);

        V3Vect<float> interpolationVector = startTrans.Lerp(endTrans, t);

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
        result = (VR4Matrixf) quatResult;
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

        result = (VR4Matrixf) quatResult;
	}

    V3Vect<float> startTrans(startMatrix.M[0][3], startMatrix.M[1][3], startMatrix.M[2][3]);
    V3Vect<float> endTrans(endMatrix.M[0][3], endMatrix.M[1][3], endMatrix.M[2][3]);

    V3Vect<float> interpolationVector = startTrans.Lerp(endTrans, t);

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
 VSoundManager & App::soundMgr()
{
    return d->soundManager;
}

bool App::isGuiOpen() const
{
    return d->guiSys->isAnyMenuOpen();
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

VR4Matrixf const & App::lastViewMatrix() const
{
    return d->lastViewMatrix;
}

void App::setLastViewMatrix(VR4Matrixf const & m)
{
    d->lastViewMatrix = m;
}

EyeParms & App::vrParms()
{
    return d->vrParms;
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

VKernel* App::kernel()
{
    return  d->kernel;
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

VMainActivity *App::appInterface()
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
    d->infoTextColor = V4Vectf(1.0f);
    d->infoTextOffset = V3Vectf(0.0f, 0.0f, 1.5f);
    d->infoTextPointTracker.Reset();
    d->infoTextEndFrame = d->vrFrame.FrameNumber + (long long)(duration * 60.0f) + 1;
}

void App::showInfoText(float const duration, V3Vectf const & offset, V4Vectf const & color, const char * fmt, ...)
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

void App::setShowVolumePopup(bool const show)
{
    d->showVolumePopup = show;
}

bool App::showVolumePopup() const
{
    return d->showVolumePopup;
}

void App::recenterYaw(const bool showBlack)
{
    vInfo("AppLocal::RecenterYaw");
    if (showBlack)
	{
        const ovrTimeWarpParms warpSwapBlackParms = d->kernel->InitTimeWarpParms(WARP_INIT_BLACK);
        d->kernel->doSmooth(&warpSwapBlackParms);

	}
    d->kernel->ovr_RecenterYaw();

	// Change lastViewMatrix to mirror what is done to the sensor orientation by ovr_RecenterYaw.
	// Get the current yaw rotation and cancel it out. This is necessary so that subsystems that
	// rely on lastViewMatrix do not end up using the orientation from before the recenter if they
	// are called before the beginning of the next frame.
	float yaw;
	float pitch;
	float roll;
    d->lastViewMatrix.ToEulerAngles< VAxis_Y, VAxis_X, VAxis_Z, VRotate_CCW, VHanded_R >(&yaw, &pitch, &roll);

	// undo the yaw
    VR4Matrixf unrotYawMatrix(VQuatf(VAxis_Y, -yaw));
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
//void App::drawBounds( const V3Vectf &mins, const V3Vectf &maxs, const VR4Matrixf &mvp, const V3Vectf &color )
//{

//    VGlOperation glOperation;
//    VR4Matrixf	scaled = mvp * VR4Matrixf::Translation( mins ) * VR4Matrixf::Scaling( maxs - mins );
//    const VGlShader & prog = d->untexturedMvpProgram;
//    glUseProgram(prog.program);
//    glLineWidth( 1.0f );
//    glUniform4f(prog.uniformColor, color.x, color.y, color.z, 1);
//    glUniformMatrix4fv(prog.uniformModelViewProMatrix, 1, GL_FALSE /* not transposed */,
//            scaled.Transposed().M[0] );
//    glOperation.glBindVertexArrayOES_( d->unitCubeLines.vertexArrayObject );
//    glDrawElements(GL_LINES, d->unitCubeLines.indexCount, GL_UNSIGNED_SHORT, NULL);
//    glOperation.glBindVertexArrayOES_( 0 );
//}

void App::drawDialog( const VR4Matrixf & mvp )
{
    // draw the pop-up dialog
    const float now = ovr_GetTimeInSeconds();
    if ( now >= d->dialogStopSeconds )
    {
        return;
    }
    const VR4Matrixf dialogMvp = mvp * d->dialogMatrix;

    const float fadeSeconds = 0.5f;
    const float f = now - ( d->dialogStopSeconds - fadeSeconds );
    const float clampF = f < 0.0f ? 0.0f : f;
    const float alpha = 1.0f - clampF;

    drawPanel( d->dialogTexture->textureId, dialogMvp, alpha );
}

void App::drawPanel( const GLuint externalTextureId, const VR4Matrixf & dialogMvp, const float alpha )
{
    const VGlShader & prog = d->externalTextureProgram2;
    glUseProgram( prog.program );
    glUniform4f(prog.uniformColor, 1, 1, 1, alpha );

    glUniformMatrix4fv(prog.uniformTexMatrix, 1, GL_FALSE, VR4Matrixf::Identity().Transposed().M[0]);
    glUniformMatrix4fv(prog.uniformModelViewProMatrix, 1, GL_FALSE, dialogMvp.Transposed().M[0] );

    // It is important that panels write to destination alpha, or they
    // might get covered by an overlay plane/cube in TimeWarp.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, externalTextureId);
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    d->panelGeometry.drawElements();
    glDisable( GL_BLEND );
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0 );	// don't leave it bound
}

void App::drawEyeViewsPostDistorted( VR4Matrixf const & centerViewMatrix, const int numPresents )
{
    VGlOperation glOperation;
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
    const float fovDegrees = d->kernel->device->eyeDisplayFov[0] +
            ( ( d->swapParms.MinimumVsyncs > 1 ) ? 10.0f : 0.0f ) +
            ( ( !d->showVignette ) ? 5.0f : 0.0f );

    // DisplayMonoMode uses a single eye rendering for speed improvement
    // and / or high refresh rate double-scan hardware modes.
    const int numEyes = d->renderMonoMode ? 1 : 2;


    // Flush out and report any errors
    glOperation.logErrorsEnum("FrameStart");

    if ( d->drawCalibrationLines && d->calibrationLinesDrawn )
    {
        // doing a time warp test, don't generate new images
        vInfo( "drawCalibrationLines && calibrationLinesDrawn" );
    }
    else
    {
        // possibly change the buffer parameters
        d->eyeTargets->BeginFrame( d->vrParms );

        for ( int eye = 0; eye < numEyes; eye++ )
        {
            d->eyeTargets->BeginRenderingEye( eye );

            // Call back to the app for drawing.
            const VR4Matrixf mvp = d->appInterface->drawEyeView( eye, fovDegrees );

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

        d->kernel->doSmooth(&d->swapParms);

    }
}

// Draw a screen to an eye buffer the same way it would be drawn as a
// time warp overlay.
//void App::drawScreenDirect( const GLuint texid, const ovrMatrix4f & mvp )
//{
//    VGlOperation glOperation;
//    const VR4Matrixf mvpMatrix( mvp );
//    glActiveTexture( GL_TEXTURE0 );
//    glBindTexture( GL_TEXTURE_2D, texid );

//    glUseProgram( d->overlayScreenDirectProgram.program );

//    glUniformMatrix4fv( d->overlayScreenDirectProgram.uniformModelViewProMatrix, 1, GL_FALSE, mvpMatrix.Transposed().M[0] );

//    glOperation.glBindVertexArrayOES_( d->unitSquare.vertexArrayObject );
//    glDrawElements( GL_TRIANGLES, d->unitSquare.indexCount, GL_UNSIGNED_SHORT, NULL );

//    glBindTexture( GL_TEXTURE_2D, 0 );	// don't leave it bound
//}

// draw a zero to destination alpha
void App::drawScreenMask( const ovrMatrix4f & mvp, const float fadeFracX, const float fadeFracY )
{
    VR4Matrixf mvpMatrix( mvp );

    glUseProgram( d->overlayScreenFadeMaskProgram.program );

    glUniformMatrix4fv( d->overlayScreenFadeMaskProgram.uniformModelViewProMatrix, 1, GL_FALSE, mvpMatrix.Transposed().M[0] );

    if ( d->fadedScreenMaskSquare.vertexArrayObject == 0 )
    {
        d->fadedScreenMaskSquare.createScreenMaskSquare( fadeFracX, fadeFracY );
    }

    glColorMask( 0.0f, 0.0f, 0.0f, 1.0f );
    d->fadedScreenMaskSquare.drawElements();
    glColorMask( 1.0f, 1.0f, 1.0f, 1.0f );
}

NV_NAMESPACE_END
