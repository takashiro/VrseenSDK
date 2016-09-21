#include "App.h"

#include <android/keycodes.h>
#include <android/native_window.h>
#include <3rdparty/nanovg/nanovg.h>

#include "VModule.h"
#include "VEglDriver.h"
#include "android/JniUtils.h"
#include "android/VOsBuild.h"
#include "VTimer.h"
#include "VAlgorithm.h"
#include "BitmapFont.h"

#include "VDeviceManager.h"
#include "EyePostRender.h"
#include "VTexture.h"
#include "ModelView.h"
#include "SurfaceTexture.h"

#include "VDevice.h"
#include "VFrameSmooth.h"
#include "VKernel.h"
#include "VUserSettings.h"

#include "VZipFile.h"
#include "VJson.h"
#include "VLog.h"
#include "VMainActivity.h"
#include "VThread.h"
#include "VStandardPath.h"
#include "VColor.h"
#include "VScene.h"
#include "VRotationSensor.h"
#include "VResource.h"
#include "VTexture.h"
#include "VGui.h"
#include "VModel.h"

//#define TEST_TIMEWARP_WATCHDOG
#define EGL_PROTECTED_CONTENT_EXT 0x32c0

NV_NAMESPACE_BEGIN

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

static VVect3f ViewOrigin(const VMatrix4f & view)
{
    return VVect3f(view.cell[0][3], view.cell[1][3], view.cell[2][3]);
}

static VVect3f ViewForward(const VMatrix4f & view)
{
    return VVect3f(-view.cell[0][2], -view.cell[1][2], -view.cell[2][2]);
}

// Always make the panel upright, even if the head was tilted when created
static VMatrix4f PanelMatrix(const VMatrix4f & lastViewMatrix, const float popupDistance,
        const float popupScale, const int width, const int height)
{
    // TODO: this won't be valid until a frame has been rendered
    const VMatrix4f invView = lastViewMatrix.inverted();
    const VVect3f forward = ViewForward(invView);
    const VVect3f levelforward = VVect3f(forward.x, 0.0f, forward.z).normalized();
    // TODO: check degenerate case
    const VVect3f up(0.0f, 1.0f, 0.0f);
    const VVect3f right = levelforward.crossProduct(up);

    const VVect3f center = ViewOrigin(invView) + levelforward * popupDistance;
    const float xScale = (float)width / 768.0f * popupScale;
    const float yScale = (float)height / 768.0f * popupScale;
    const VMatrix4f panelMatrix = VMatrix4f(
            xScale * right.x, yScale * up.x, forward.x, center.x,
            xScale * right.y, yScale * up.y, forward.y, center.y,
            xScale * right.z, yScale * up.z, forward.z, center.z,
            0, 0, 0, 1);

//	vInfo("PanelMatrix center:" << center.xcenter.ycenter.z);
//	LogMatrix("PanelMatrix", panelMatrix);

    return panelMatrix;
}

extern void DebugMenuBounds(void * appPtr, const char * cmd);
extern void DebugMenuHierarchy(void * appPtr, const char * cmd);
extern void DebugMenuPoses(void * appPtr, const char * cmd);

static VEyeItem::Settings DefaultVrParmsForRenderer(const VEglDriver & glOperation)
{
    VEyeItem::settings.resolution = 1024;
    VEyeItem::settings.multisamples = (glOperation.m_gpuType == VEglDriver::GPU_TYPE_ADRENO_330) ? 2 : 2;//zx_note :4
    VEyeItem::settings.colorFormat = VColor::COLOR_8888;
    VEyeItem::settings.commonParameterDepth = VEyeItem::CommonParameter::DepthFormat_24;
    VEyeItem::settings.wantSingleBuffer = VDeviceManager::getSupportedVRDeviceType() != VDeviceManager::VDevice_Type_Unknown;

    VEyeItem::settings.wantSingleBuffer = VOsBuild::getString(VOsBuild::Model).contains("ZTE A2017") ||
                                          VOsBuild::getString(VOsBuild::Model).contains("SM-N910") ||
                                          VOsBuild::getString(VOsBuild::Model).contains("SM-N916") ||
                                          VOsBuild::getString(VOsBuild::Model).contains("SM-N920") ||
                                          VOsBuild::getString(VOsBuild::Model).contains("SM-G920") ||
                                          VOsBuild::getString(VOsBuild::Model).contains("SM-G925") ||
                                          VOsBuild::getString(VOsBuild::Model).contains("SM-G928") ||
                                          VOsBuild::getString(VOsBuild::Model).contains("vivo Xplay5S");

    return VEyeItem::settings;
}

static bool ChromaticAberrationCorrection(const VEglDriver & glOperation)
{
    return (glOperation.m_gpuType & VEglDriver::GPU_TYPE_ADRENO) != 0 && (glOperation.m_gpuType >= VEglDriver::GPU_TYPE_ADRENO_420);
}

struct App::Private
{
    App *self;
    // Primary apps will exit(0) when they get an onDestroy() so we
    // never leave any cpu-sucking process running, but the platformUI
    // needs to just return to the primary activity.
    volatile bool vrThreadSynced;
    volatile bool createdSurface;
    volatile bool surfaceChanged;
    volatile bool readyToExit;		// start exit procedure
    volatile bool running;

    // Most calls in from java should communicate through this.
    VEventLoop eventLoop;

    // Egl context and surface for rendering
    VEglDriver m_glStatus;


    JavaVM *javaVM;

    JNIEnv *vrJni;			// for use by the VR thread

    VString launchIntentURI;			// URI app was launched with
    VString launchIntentJSON;			// extra JSON data app was launched with
    VString launchIntentFromPackage;	// package that sent us the launch intent

    VString packageCodePath;	// path to apk to open as zip and load resources

    bool paused;				// set/cleared by onPause / onResume

    float popupDistance;
    float popupScale;


    VPointTracker fpsPointTracker;
    bool showFPS;

    // Every application gets a basic dialog surface.
    SurfaceTexture *dialogTexture;

    // Current joypad state, without pressed / released calculation
    VInput joypad;

    // drawing parameters
    int dialogWidth;
    int dialogHeight;

    // Dialogs will be oriented base down in the view when they
    // were generated.

    VMatrix4f lastViewMatrix;

    ANativeWindow *nativeWindow;
    EGLSurface windowSurface;

    bool drawCalibrationLines;	// currently toggled by right trigger
    bool calibrationLinesDrawn;	// after draw, go to static time warp test
    bool showVignette;			// render the vignette

    int  m_width;
    int  m_height;
    EGLint m_attribs[100];
    bool framebufferIsSrgb;			// requires KHR_gl_colorspace
    bool framebufferIsProtected;		// requires GPU trust zone extension

    // Only render a single eye view, which will get warped for both
    // screen eyes.
    bool renderMonoMode;

    VFrame lastVrFrame;

    VGlShader untexturedMvpProgram;
    VGlShader untexturedScreenSpaceProgram;
    VGlShader overlayScreenFadeMaskProgram;
    VGlShader overlayScreenDirectProgram;

    VGlGeometry unitCubeLines;		// 12 lines that outline a 0 to 1 unit cube, intended to be scaled to cover bounds.
    VGlGeometry unitSquare;			// -1 to 1 in x and Y, 0 to 1 in texcoords
    VGlGeometry fadedScreenMaskSquare;// faded screen mask for overlay rendering

    EyePostRender eyeDecorations;

    VRotationState sensorForNextWarp;

    VThread *renderThread;
    int vrThreadTid;		// linux tid

    VViewSettings viewSettings;

    float touchpadTimer;
    VVect2f touchOrigin;
    float lastTouchpadTime;
    bool lastTouchDown;
    int touchState;

    bool enableDebugOptions;	// enable debug key-commands for development testing

    long long recenterYawFrameStart;	// Enables reorient before sensor data is read.  Allows apps to reorient without having invalid orientation information for that frame.

    BitmapFont *defaultFont;
    BitmapFontSurface *worldFontSurface;
    KeyState backKeyState;
    VStandardPath *storagePaths;

    VTexture errorTexture;
    int errorTextureSize;
    double errorMessageEndTime;

    jobject javaObject;
    VMainActivity *activity;
    VKernel *kernel;
//    VScene *scene;
    VGui *gui;
    const std::list<VModule *> &modules;

    VTimeWarpParms	swapParms;
    VArray<VModel*> models;
    VEyeItem* eyeItem;

    Private(App *self)
        : self(self)
        , vrThreadSynced(false)
        , createdSurface(false)
        , surfaceChanged(false)
        , readyToExit(false)
        , running(false)
        , eventLoop(100)
        , javaVM(JniUtils::GetJavaVM())
        , vrJni(nullptr)
        , paused(true)
        , popupDistance(2.0f)
        , popupScale(1.0f)
        , showFPS(false)//zx_note  close fps show
        , dialogWidth(0)
        , dialogHeight(0)
        , nativeWindow(nullptr)
        , windowSurface(EGL_NO_SURFACE)
        , drawCalibrationLines(false)
        , calibrationLinesDrawn(false)
        , showVignette(true)
        , framebufferIsSrgb(false)
        , framebufferIsProtected(false)
        , renderMonoMode(false)
        , vrThreadTid(0)
        , touchpadTimer(0.0f)
        , lastTouchpadTime(0.0f)
        , lastTouchDown(false)
        , touchState(0)
        , enableDebugOptions(false)
        , recenterYawFrameStart(0)
        , defaultFont(nullptr)
        , worldFontSurface(nullptr)
        , backKeyState(0.25f, 0.75f)
        , storagePaths(nullptr)
        , errorTextureSize(0)
        , errorMessageEndTime(-1.0)
        , javaObject(nullptr)
        , activity(nullptr)
//        , scene(new VScene)
        , gui(new VGui)
        , modules(VModule::List())
        , eyeItem(nullptr)
    {
    }

    ~Private()
    {
//        delete scene;
        delete gui;
    }

    void initFonts()
    {
        defaultFont = BitmapFont::Create();
        if (!defaultFont->Load(packageCodePath, "res/raw/efigs.fnt")) {
            vWarn("default font not found");
        }

        worldFontSurface = BitmapFontSurface::Create();

        worldFontSurface->Init(8192);
    }

    void shutdownFonts()
    {
        BitmapFont::Free(defaultFont);
        BitmapFontSurface::Free(worldFontSurface);
    }

    void pause()
    {
        activity->onPause();

        VKernel::instance()->pause();
//        for(VModule *module : modules) {
//            module->onPause();
//        }
    }

    void resume()
    {
        vInfo("OVRTimer AppLocal::Resume");

        // Make sure the window surface is current, which it won't be
        // if we were previously in async mode
        // (Not needed now?)
//        if (eglMakeCurrent(m_glStatus.m_display, windowSurface, windowSurface, m_glStatus.m_context) == EGL_FALSE)
//        {
//            vFatal("eglMakeCurrent failed:" << m_glStatus.getEglErrorString());
//        }

        // Allow the app to override
        activity->configureVrMode(kernel);
        VEyeItem::settings.useMultiview &= VEglDriver::glIsExtensionString("GL_OVR_multiview2");
        vInfo("useMultiview : "<<VEyeItem::settings.useMultiview);
        activity->onResume();

        VKernel::instance()->resume();
//        for (VModule *module : modules) {
//            module->onResume();
//        }
    }

    void initGlObjects()
    {
        DefaultVrParmsForRenderer(m_glStatus);

        //kernel->setSmoothProgram(ChromaticAberrationCorrection(m_glStatus) ? VK_DEFAULT_CB : VK_DEFAULT);
        swapParms.WarpProgram = ChromaticAberrationCorrection(m_glStatus) ? WP_CHROMATIC : WP_SIMPLE;
        m_glStatus.logExtensions();

        untexturedMvpProgram.initShader( VGlShader::getUntextureMvpVertexShaderSource(),VGlShader::getUntexturedFragmentShaderSource()  );
        untexturedScreenSpaceProgram.initShader( VGlShader::getUniformColorVertexShaderSource(), VGlShader::getUntexturedFragmentShaderSource() );
        overlayScreenFadeMaskProgram.initShader(VGlShader::getUntextureInverseColorVertexShaderSource(),VGlShader::getUntexturedFragmentShaderSource() );
        overlayScreenDirectProgram.initShader(VGlShader::getSingleTextureVertexShaderSource(),VGlShader::getSingleTextureFragmentShaderSource() );


        unitSquare.createPlaneQuadGrid(1, 1 );
        unitCubeLines.createUnitCubeGrid();


        eyeDecorations.Init();
    }

    void shutdownGlObjects()
    {
        untexturedMvpProgram.destroy();
        untexturedScreenSpaceProgram.destroy();
        overlayScreenFadeMaskProgram.destroy();
        overlayScreenDirectProgram.destroy();

        unitSquare.destroy();
        unitCubeLines.destroy();
        fadedScreenMaskSquare.destroy();

        eyeDecorations.Shutdown();
    }

    void interpretTouchpad(VInput &input)
    {
        // 1) Down -> Up w/ Motion = Slide
        // 2) Down -> Up w/out Motion -> Timeout = Single Tap
        // 3) Down -> Up w/out Motion -> Down -> Timeout = Nothing
        // 4) Down -> Up w/out Motion -> Down -> Up = Double Tap
        static const float timer_finger_down = 0.3f;
        static const float timer_finger_up = 0.3f;
        static const float min_swipe_distance = 100.0f;

        float currentTime = VTimer::Seconds();
        float deltaTime = currentTime - lastTouchpadTime;
        lastTouchpadTime = currentTime;
        touchpadTimer = touchpadTimer + deltaTime;

        bool down = false, up = false;
        bool currentTouchDown = input.buttonState & BUTTON_TOUCH;

        if (currentTouchDown && !lastTouchDown) {
            //CreateToast("DOWN");
            down = true;
            touchOrigin = input.touch;
        }

        if (!currentTouchDown && lastTouchDown) {
            //CreateToast("UP");
            up = true;
        }

        lastTouchDown = currentTouchDown;

        input.touchRelative = input.touch - touchOrigin;
        float touchMagnitude = input.touchRelative.length();
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
                if (fabs(input.touchRelative.x) > fabs(input.touchRelative.y))
                {
                    if (input.touchRelative.x < 0) {
                        //CreateToast("SWIPE FORWARD");
                        dir = BUTTON_SWIPE_FORWARD | BUTTON_TOUCH_WAS_SWIPE;
                    } else {
                        //CreateToast("SWIPE BACK");
                        dir = BUTTON_SWIPE_BACK | BUTTON_TOUCH_WAS_SWIPE;
                    }
                }
                else
                {
                    if (input.touchRelative.y > 0)
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

    void frameworkButtonProcessing(const VInput &input)
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
            vInfo("QAEvent " << timeStr << " (" << VTimer::Seconds() << ") - QA event occurred");
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
                    //self->createToast("eye warp");
                }
                else
                {
                    swapParms.WarpOptions |= SWAP_OPTION_USE_SLICED_WARP;
                    //self->createToast("slice warp");
                }
            }

            if (swapParms.WarpOptions & SWAP_OPTION_USE_SLICED_WARP)
            {
                extern float calibrateFovScale;

                if (input.buttonPressed & BUTTON_DPAD_LEFT)
                {
                    swapParms.PreScheduleSeconds -= 0.001f;
                    //self->createToast("Schedule: %f",  swapParms.PreScheduleSeconds);
                }
                if (input.buttonPressed & BUTTON_DPAD_RIGHT)
                {
                    swapParms.PreScheduleSeconds += 0.001f;
                    //self->createToast("Schedule: %f",  swapParms.PreScheduleSeconds);
                }
                if (input.buttonPressed & BUTTON_DPAD_UP)
                {
                    calibrateFovScale -= 0.01f;
                    //self->createToast("calibrateFovScale: %f", calibrateFovScale);
                    pause();
                    resume();
                }
                if (input.buttonPressed & BUTTON_DPAD_DOWN)
                {
                    calibrateFovScale += 0.01f;
                    //self->createToast("calibrateFovScale: %f", calibrateFovScale);
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
            float x = event.data.at(1).toFloat();
            float y = event.data.at(2).toFloat();
            joypad.touch.x = event.data.at(1).toFloat();
            joypad.touch.y = event.data.at(2).toFloat();
            if (action == 0) {
                joypad.buttonState |= BUTTON_TOUCH;
            }
            if (action == 1) {
                joypad.buttonState &= ~BUTTON_TOUCH;
            }
            gui->onTouchEvent(x, y);
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

        if(event.name == "surfaceCreated") {
            vInfo(event.name);
            if (windowSurface != EGL_NO_SURFACE)
            {	// Samsung says this is an Android problem, where surfaces are reported as
                // created multiple times.
                vWarn("Skipping create work because window hasn't been destroyed.");
                return;
            }

            nativeWindow = static_cast<ANativeWindow *>(event.data.toPointer());
            int		numAttribs = 0;

            // Set the colorspace on the window
            windowSurface = EGL_NO_SURFACE;
            if (activity->wantSrgbFramebuffer())
            {
                m_attribs[numAttribs++] = VEglDriver::EGL_GL_COLORSPACE_KHR;
                m_attribs[numAttribs++] = VEglDriver::EGL_GL_COLORSPACE_SRGB_KHR;
            }
            // Ask for TrustZone rendering support
            if (activity->wantProtectedFramebuffer())
            {
                m_attribs[numAttribs++] = EGL_PROTECTED_CONTENT_EXT;
                m_attribs[numAttribs++] = EGL_TRUE;
            }

            //use single buffer
            if(self->vrParms().wantSingleBuffer)
            {
                m_attribs[numAttribs++] = EGL_RENDER_BUFFER;
                m_attribs[numAttribs++] = EGL_SINGLE_BUFFER;
            }

            m_attribs[numAttribs++] = EGL_NONE;

            // Android doesn't let the non-standard extensions show up in the
            // extension string, so we need to try it blind.
            windowSurface = eglCreateWindowSurface(m_glStatus.m_display, m_glStatus.m_config,
                    nativeWindow, m_attribs);

            if (windowSurface == EGL_NO_SURFACE )
            {
                numAttribs = 0;
                //use single buffer
                if(self->vrParms().wantSingleBuffer)
                {
                    m_attribs[numAttribs++] = EGL_RENDER_BUFFER;
                    m_attribs[numAttribs++] = EGL_SINGLE_BUFFER;
                }
                m_attribs[numAttribs++] = EGL_NONE;


                windowSurface = eglCreateWindowSurface(m_glStatus.m_display, m_glStatus.m_config,
                        nativeWindow, m_attribs);
                if (windowSurface == EGL_NO_SURFACE)
                {
                    vFatal("eglCreateWindowSurface failed:" << m_glStatus.getEglErrorString());
                }
                framebufferIsSrgb = false;
                framebufferIsProtected = false;
            }
            else
            {
                framebufferIsSrgb = activity->wantSrgbFramebuffer();
                framebufferIsProtected = activity->wantProtectedFramebuffer();
            }

//            if (eglMakeCurrent(m_glStatus.m_display, windowSurface, windowSurface, m_glStatus.m_context) == EGL_FALSE)
//            {
//                vFatal("eglMakeCurrent failed:" << m_glStatus.getEglErrorString());
//            }

            eglQuerySurface( m_glStatus.m_display, windowSurface, EGL_WIDTH,  &m_width );
            eglQuerySurface( m_glStatus.m_display, windowSurface, EGL_HEIGHT, &m_height );

            createdSurface = true;

            // Let the client app setup now
            activity->onWindowCreated();

            // start framesmooth thread
            VKernel::instance()->enterVrMode();

            // Resume
            if (!paused)
            {
                resume();
            }
            return;
        }

        if (event.name == "surfaceChanged") {
            vInfo(event.name);
            if (windowSurface == EGL_NO_SURFACE)
            {	// Samsung says this is an Android problem, where surfaces are reported as
                // created multiple times.
                vWarn("Skipping create work because window hasn't been destroyed.");
                return;
            }

            if( m_width != event.data.at(0).toInt()
                || m_height != event.data.at(1).toInt() ) {
                eglDestroySurface(m_glStatus.m_display, windowSurface);
                windowSurface = eglCreateWindowSurface(m_glStatus.m_display, m_glStatus.m_config, nativeWindow, m_attribs);

                eglQuerySurface( m_glStatus.m_display, windowSurface, EGL_WIDTH,  &m_width );
                eglQuerySurface( m_glStatus.m_display, windowSurface, EGL_HEIGHT, &m_height );
            }

            VKernel::instance()->setSurfaceRevolution(windowSurface);

            surfaceChanged = true;
//            VKernel::instance()->setSurfaceRevolution(windowSurface);
            // Let the client app setup now
//            activity->onWindowCreated();
            return;
        }

        if (event.name == "surfaceDestroyed") {
            vInfo("surfaceDestroyed");

            // Let the client app shutdown first.
            activity->onWindowDestroyed();

            // stop framesmooth thread
            VKernel::instance()->destroy(EXIT_TYPE_EXIT);

            // Handle it ourselves.
            if (eglMakeCurrent(m_glStatus.m_display, m_glStatus.m_pbufferSurface, m_glStatus.m_pbufferSurface,
                    m_glStatus.m_context) == EGL_FALSE)
            {
                vFatal("RC_SURFACE_DESTROYED: eglMakeCurrent pbuffer failed");
            }

            if (windowSurface != EGL_NO_SURFACE)
            {
                eglDestroySurface(m_glStatus.m_display, windowSurface);
                windowSurface = EGL_NO_SURFACE;
            }
            if (nativeWindow != nullptr)
            {
                ANativeWindow_release(nativeWindow);
                nativeWindow = nullptr;
            }

            createdSurface = false;
            surfaceChanged = false;
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
            vInfo("resume");
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
            activity->onNewIntent(fromPackageName, json, uri);
            return;
        }

        if (event.name == "popup") {
            vAssert(event.data.isArray());
            int width = event.data.at(0).toInt();
            int height = event.data.at(1).toInt();
            float seconds = event.data.at(2).toFloat();

            dialogWidth = width;
            dialogHeight = height;
            self->dialog.dialogStopSeconds = VTimer::Seconds() + seconds;

            self->dialog.dialogMatrix = PanelMatrix(lastViewMatrix, popupDistance, popupScale, width, height);

            glActiveTexture(GL_TEXTURE0);
            vInfo("RC_UPDATE_POPUP dialogTexture" << self->dialog.dialogTexture->textureId);
            self->dialog.dialogTexture->Update();
            glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

            return;
        }

        if (event.name == "sync") {
            return;
        }

        if (event.name == "quit") {
            kernel->destroy(EXIT_TYPE_EXIT);
            readyToExit = true;
            vInfo("VrThreadSynced=" << vrThreadSynced << " CreatedSurface=" << createdSurface << " ReadyToExit=" << readyToExit);
        }

        if(event.name == "loadModel"){
            VString path = event.data.at(0).toString();
            std::function<void()> completeListener = event.data.at(1).toFunction();

            VModel* model = new VModel;
            model->loadAsync(path,completeListener);

            models.push_back(model);
            return;
        }

        if(event.name == "loadModelCompleted"){
            VModel::command(event);
            return;
        }

        if(event.name == "activityInitCompleted"){
            if(activity->showLoadingIcon()) gui->removeLoading();
            return;
        }

        // Pass it on to the client app.
        activity->command(event);
    }

    void run()
    {
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
                vFatal("javaVM->AttachCurrentThread returned" << rtn);
            }

            // Set up another thread for making longer-running java calls
            // to avoid hitches.
            activity->Init(javaVM);

            // Create a new context and pbuffer surface
            const int windowDepth = 0;
            const int windowSamples = 0;
            const GLuint contextPriority = EGL_CONTEXT_PRIORITY_MEDIUM_IMG;
            if( !m_glStatus.eglInit(EGL_NO_CONTEXT, GL_ES_VERSION,8,8,8, windowDepth, windowSamples, contextPriority)
                || m_glStatus.m_glEsVersion <= 2 )
            {
                vFatal("Failed to init egl or egl version is less than 3!");
            }

            // Create our GL data objects
            initGlObjects();

            for (VModule *module : modules) {
                module->onStart();
            }

//            scene->addEyeItem();
//            scene->addEyeItem();
            eyeItem = new VEyeItem;

            // Create the SurfaceTexture for dialog rendering.
            self->dialog.dialogTexture = new SurfaceTexture(vrJni);

            lastTouchpadTime = VTimer::Seconds();
        }

        // FPS counter information
        int countApplicationFrames = 0;
        double lastReportTime = ceil(VTimer::Seconds());

        while(!(vrThreadSynced && createdSurface && readyToExit))
        {
            //SPAM("FRAME START");
            // Process incoming messages until queue is empty
            forever {
                VEvent event = eventLoop.next();
                if (!event.isValid()) {
                    break;
                }
                command(event);

            }

            // If we don't have a surface yet, or we are paused, sleep until
            // something shows up on the message queue.
            if ((windowSurface == EGL_NO_SURFACE) || (surfaceChanged == false) || paused)
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
                if (VTimer::Seconds() >= errorMessageEndTime)
                {
                    kernel->destroy(EXIT_TYPE_FINISH_AFFINITY);
                }
                else
                {
                    VTimeWarpParms warpSwapMessageParms = kernel->InitTimeWarpParms(WARP_INIT_MESSAGE, errorTexture.id());
                    warpSwapMessageParms.ProgramParms[0] = 0.0f;						// rotation in radians
                    warpSwapMessageParms.ProgramParms[1] = 1024.0f / errorTextureSize;	// message size factor
                    kernel->doSmooth(&warpSwapMessageParms);

//                    kernel->InitTimeWarpParms();
//                    kernel->setSmoothOption( VK_INHIBIT_SRGB_FB | VK_FLUSH | VK_IMAGE);
//
//                    kernel->setSmoothProgram(VK_LOGO);
//                    float mprogramParms[4];
//                    mprogramParms[0] = 0.0f;		// rotation in radians per second
//                    mprogramParms[1] = 2.0f;
//                    kernel->setProgramParms(mprogramParms);
//                            // icon size factor smaller than fullscreen
//                    for ( int eye = 0; eye < 2; eye++ )
//                    {
//                       kernel->setSmoothEyeTexture(0, eye, 0);
//                       kernel->setSmoothEyeTexture(errorTexture.id(), eye, 1);
//
//                    }
//
//                    kernel->doSmooth();
//                    kernel->setSmoothProgram(VK_DEFAULT);

                }
                continue;
            }

            // Let the client app initialize only once by calling OneTimeInit() when the windowSurface is valid.
            if (!running)
            {
                vInfo("launchIntentJSON:" << launchIntentJSON);
                vInfo("launchIntentURI:" << launchIntentURI);

                gui->init();
                if (activity->showLoadingIcon())
                {
                    gui->showLoading(0);
                }

                initFonts();
                self->dialog.init();
                activity->init(launchIntentFromPackage, launchIntentJSON, launchIntentURI);

                running = true;
            }

            // latch the current joypad state and note transitions
            self->text.vrFrame.input = joypad;
            self->text.vrFrame.input.buttonPressed = joypad.buttonState & (~lastVrFrame.input.buttonState);
            self->text.vrFrame.input.buttonReleased = ~joypad.buttonState & (lastVrFrame.input.buttonState & ~BUTTON_TOUCH_WAS_SWIPE);

            if (lastVrFrame.input.buttonState & BUTTON_TOUCH_WAS_SWIPE)
            {
                if (lastVrFrame.input.buttonReleased & BUTTON_TOUCH)
                {
                    self->text.vrFrame.input.buttonReleased |= BUTTON_TOUCH_WAS_SWIPE;
                }
                else
                {
                    // keep it around this frame
                    self->text.vrFrame.input.buttonState |= BUTTON_TOUCH_WAS_SWIPE;
                }
            }

            // Synthesize swipe gestures
            interpretTouchpad(self->text.vrFrame.input);

            if (recenterYawFrameStart != 0)
            {
                // Perform a reorient before sensor data is read.  Allows apps to reorient without having invalid orientation information for that frame.
                // Do a warp swap black on the frame the recenter started.
                self->recenterYaw(recenterYawFrameStart == (self->text.vrFrame.id + 1));  // vrFrame.FrameNumber hasn't been incremented yet, so add 1.
            }

            // Get the latest head tracking state, predicted ahead to the midpoint of the time
            // it will be displayed.  It will always be corrected to the real values by
            // time warp, but the closer we get, the less black will be pulled in at the edges.
            const double now = VTimer::Seconds();
            static double prev = 0.0;
            const double rawDelta = now - prev;
            prev = now;
            const double clampedPrediction = std::min(0.1, rawDelta * 2);
            sensorForNextWarp = VRotationSensor::instance()->predictState(now + clampedPrediction);

            self->text.vrFrame.pose = sensorForNextWarp;
            self->text.vrFrame.deltaSeconds   = std::min(0.1, rawDelta);
            self->text.vrFrame.id++;

            // Don't allow this to be excessively large, which can cause application problems.
            if (self->text.vrFrame.deltaSeconds > 0.1f)
            {
                self->text.vrFrame.deltaSeconds = 0.1f;
            }

            lastVrFrame = self->text.vrFrame;

            frameworkButtonProcessing(self->text.vrFrame.input);

            KeyState::eKeyEventType event = backKeyState.Update(VTimer::Seconds());
            if (event != KeyState::KEY_EVENT_NONE)
            {
                //vInfo("BackKey: event" << KeyState::EventNames[ event ]);
                // always allow the gaze cursor to peek at the event so it can start the gaze timer if necessary
                // update the gaze cursor timer
                if (event == KeyState::KEY_EVENT_DOWN)
                {
                }
                else if (event == KeyState::KEY_EVENT_DOUBLE_TAP || event == KeyState::KEY_EVENT_SHORT_PRESS)
                {
                }
                else if (event == KeyState::KEY_EVENT_LONG_PRESS)
                {
                    //StartSystemActivity(PUI_GLOBAL_MENU);
                    kernel->destroy(EXIT_TYPE_FINISH);
                }

                // let the menu handle it if it's open
                bool consumedKey = activity->onKeyEvent(AKEYCODE_BACK, event);

                // if nothing consumed the key and it's a short-press, exit the application to OculusHome
                if (!consumedKey)
                {
                    if (event == KeyState::KEY_EVENT_SHORT_PRESS)
                    {
                        consumedKey = true;
                        vInfo("BUTTON_BACK: confirming quit in platformUI");
                        kernel->destroy(EXIT_TYPE_FINISH);
                    }
                }
            }

            if (showFPS) {
                const int FPS_NUM_FRAMES_TO_AVERAGE = 30;
                static double  LastFrameTime = VTimer::Seconds();
                static double  AccumulatedFrameInterval = 0.0;
                static int   NumAccumulatedFrames = 0;
                static float LastFrameRate = 60.0f;

                double currentFrameTime = VTimer::Seconds();
                double frameInterval = currentFrameTime - LastFrameTime;
                AccumulatedFrameInterval += frameInterval;
                NumAccumulatedFrames++;
                if (NumAccumulatedFrames > FPS_NUM_FRAMES_TO_AVERAGE) {
                    double interval = (AccumulatedFrameInterval / NumAccumulatedFrames);  // averaged
                    AccumulatedFrameInterval = 0.0;
                    NumAccumulatedFrames = 0;
                    LastFrameRate = 1.0f / float(interval > 0.000001 ? interval : 0.00001);
                }

                VVect3f viewPos(0.0f,-2.0f,0.0f); //= lastViewMatrix.viewPosition();    //viewForward();
                VVect3f viewFwd = lastViewMatrix.viewForward();
                VVect3f newPos = viewPos + viewFwd * 1.5f;
                fpsPointTracker.Update(VTimer::Seconds(), newPos);

                fontParms_t fp;
                fp.AlignHoriz = HORIZONTAL_CENTER;
                fp.Billboard = true;
                fp.TrackRoll = false;
                VString temp;
                temp.sprintf("%.1f fps", LastFrameRate);
                worldFontSurface->DrawTextBillboarded3D(*defaultFont, fp, fpsPointTracker.GetCurPosition(),
                        0.8f, VVect4f(1.0f, 0.0f, 0.0f, 1.0f), temp);

                LastFrameTime = currentFrameTime;
            }

            // draw info text
            if (self->text.infoTextEndFrame >= self->text.vrFrame.id)
            {
                VVect3f viewPos = lastViewMatrix.viewPosition();
                VVect3f viewFwd = lastViewMatrix.viewForward();
                VVect3f viewUp(0.0f, 1.0f, 0.0f);
                VVect3f viewLeft = viewUp.crossProduct(viewFwd);
                VVect3f newPos = viewPos + viewFwd * self->text.infoTextOffset.z + viewUp * self->text.infoTextOffset.y + viewLeft * self->text.infoTextOffset.x;
                self->text.infoTextPointTracker.Update(VTimer::Seconds(), newPos);

                fontParms_t fp;
                fp.AlignHoriz = HORIZONTAL_CENTER;
                fp.AlignVert = VERTICAL_CENTER;
                fp.Billboard = true;
                fp.TrackRoll = false;
                worldFontSurface->DrawTextBillboarded3D(*defaultFont, fp, self->text.infoTextPointTracker.GetCurPosition(),
                        1.0f, self->text.infoTextColor, self->text.infoText);
            }

            // Main loop logic / draw code
            if (!readyToExit) {
                lastViewMatrix = activity->onNewFrame(self->text.vrFrame);

                self->drawEyeViewsPostDistorted(lastViewMatrix);
            }

            // MWC demo hack to allow keyboard swipes
            joypad.buttonState &= ~(BUTTON_SWIPE_FORWARD|BUTTON_SWIPE_BACK);

            // Report frame counts once a second
            countApplicationFrames++;
            const double timeNow = floor(VTimer::Seconds());
            if (timeNow > lastReportTime)
            {

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

            for (VModule *module : modules) {
                module->onDestroy();
            }

            if (errorTexture != 0)
            {
                glDeleteTextures(1, &errorTexture.id());
            }

            activity->shutdown();

            shutdownFonts();

            delete self->dialog.dialogTexture;
            self->dialog.dialogTexture = nullptr;

            shutdownGlObjects();

            vInfo("javaVM->DetachCurrentThread");
            const jint rtn = javaVM->DetachCurrentThread();
            if (rtn != JNI_OK)
            {
                vInfo("javaVM->DetachCurrentThread returned" << rtn);
            }

//            delete scene;
//            scene = nullptr;

            delete eyeItem;
            eyeItem = nullptr;

            for (VModel *model : models) {
                delete model;
            }

            m_glStatus.eglExit();
            renderThread->exit();
        }
    }

    void onKeyEvent(const int keyCode, const bool down, const int repeatCount)
    {
        if (down) {
            gui->onKeyEvent(keyCode, repeatCount);
        }

        // the back key is special because it has to handle long-press and double-tap
        if (keyCode == AKEYCODE_BACK)
        {
            // back key events, because of special handling for double-tap, short-press and long-press,
            // are handled in AppLocal::VrThreadFunction.
            backKeyState.HandleEvent(VTimer::Seconds(), down, repeatCount);
            return;
        }

        // the app menu is always the first consumer so it cannot be usurped
        bool consumedKey = false;
        // for all other keys, allow VrAppInterface the chance to handle and consume the key first
        if (!consumedKey)
        {
            consumedKey = activity->onKeyEvent(keyCode, down ? KeyState::KEY_EVENT_DOWN : KeyState::KEY_EVENT_UP);
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
                    //TODO snapshot功能需要修改
                    //eyeTargets->snapshot();
                    //self->createToast("screenshot");
                    return;
                }
            }
            else if (keyCode == AKEYCODE_COMMA && down && repeatCount == 0)
            {
                float const IPD_MIN_CM = 0.0f;
                viewSettings.interpupillaryDistance = std::max(IPD_MIN_CM * 0.01f, viewSettings.interpupillaryDistance - IPD_STEP);
                VString text;
                text.sprintf("%.3f", viewSettings.interpupillaryDistance);
                self->text.show(text, 1.0f);
                return;
            }
            else if (keyCode == AKEYCODE_PERIOD && down && repeatCount == 0)
            {
                float const IPD_MAX_CM = 8.0f;
                viewSettings.interpupillaryDistance = std::min(IPD_MAX_CM * 0.01f, viewSettings.interpupillaryDistance + IPD_STEP);
                VString text;
                text.sprintf("%.3f", viewSettings.interpupillaryDistance);
                self->text.show(text, 1.0f);
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

App *vAppInstance = nullptr;

App::App(JNIEnv *jni, jobject activityObject, VMainActivity *activity)
    : d(new Private(this))
{
    d->activity = activity;
    vInfo("----------------- AppLocal::AppLocal() -----------------");
    vAssert(vAppInstance == nullptr);
    vAppInstance = this;

    d->kernel = VKernel::instance();
    d->storagePaths = new VStandardPath(jni, activityObject);

	//WaitForDebuggerToAttach();

    memset(& d->sensorForNextWarp, 0, sizeof(d->sensorForNextWarp));

    d->sensorForNextWarp.w = 1;
    d->sensorForNextWarp.x = 0;
    d->sensorForNextWarp.y = 0;
    d->sensorForNextWarp.z = 0;

	// Default time warp parms
//    d->kernel->InitTimeWarpParms();
    d->swapParms = d->kernel->InitTimeWarpParms();
    d->javaObject = jni->NewGlobalRef(activityObject);

	// Get the path to the .apk and package name
    d->packageCodePath = d->activity->getPackageCodePath();

	// Load user profile data relevant to rendering
    VUserSettings config;
    config.load();
    d->viewSettings.interpupillaryDistance = config.ipd;
    d->viewSettings.eyeHeight = config.eyeHeight;
    d->viewSettings.headModelDepth = config.headModelDepth;
    d->viewSettings.headModelHeight = config.headModelHeight;

    d->renderThread = new VThread([](void *data)->int{
        App::Private *d = static_cast<App::Private *>(data);
        d->run();
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

void App::execute()
{
    d->renderThread->start();
    d->eventLoop.send("sync");
    d->vrThreadSynced = true;
}

void App::quit()
{
    d->eventLoop.post("quit");
    bool finished = d->renderThread->wait();
    if (!finished) {
        vWarn("failed to wait for VrThread");
    }
}

bool App::isRunning() const
{
    return d->running;
}

VEventLoop &App::eventLoop()
{
    return d->eventLoop;
}

VEyeItem::Settings &App::eyeSettings()
{
    return VEyeItem::settings;
}

BitmapFont & App::defaultFont()
{
    return *d->defaultFont;
}
BitmapFontSurface & App::worldFontSurface()
{
    return *d->worldFontSurface;
}

const VStandardPath & App::storagePaths()
{
    return *d->storagePaths;
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

VMatrix4f const & App::lastViewMatrix() const
{
    return d->lastViewMatrix;
}

void App::setLastViewMatrix(VMatrix4f const & m)
{
    d->lastViewMatrix = m;
}

VEyeItem::Settings & App::vrParms()
{
    return VEyeItem::settings;
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

JNIEnv * App::vrJni()
{
    return d->vrJni;
}

jobject	& App::javaObject()
{
    return d->javaObject;
}

VGui *App::gui() const
{
    return d->gui;
}

VKernel* App::kernel()
{
    return  d->kernel;
}

SurfaceTexture * App::dialogTexture()
{
    return dialog.dialogTexture;
}

VTimeWarpParms const & App::swapParms() const
{
    return d->swapParms;
}

VTimeWarpParms & App::swapParms()
{
    return d->swapParms;
}

const VRotationState &App::sensorForNextWarp() const
{
    return d->sensorForNextWarp;
}

VMainActivity *App::appInterface()
{
    return d->activity;
}

const VViewSettings &App::viewSettings() const
{
    return d->viewSettings;
}

void App::setViewSettings(const VViewSettings &settings)
{
    d->viewSettings = settings;
}

KeyState & App::backKeyState()
{
    return d->backKeyState;
}

void App::recenterYaw(const bool showBlack)
{
    vInfo("AppLocal::RecenterYaw");
    if (showBlack)
	{
        const VTimeWarpParms warpSwapBlackParms = d->kernel->InitTimeWarpParms(WARP_INIT_BLACK);
        d->kernel->doSmooth(&warpSwapBlackParms);

//        d->kernel->InitTimeWarpParms();
//        d->kernel->setSmoothOption( VK_INHIBIT_SRGB_FB | VK_FLUSH | VK_IMAGE);
//        d->kernel->setSmoothProgram( VK_DEFAULT);
//        for ( int eye = 0; eye < 2; eye++ )
//        {
//           d->kernel->setSmoothEyeTexture(eye,0,0);		// default replaced with a black texture
//        }


//        d->kernel->doSmooth();


	}
    //d->kernel->ovr_RecenterYaw();
    VRotationSensor *sensor = VRotationSensor::instance();
    sensor->recenterYaw();

	// Change lastViewMatrix to mirror what is done to the sensor orientation by ovr_RecenterYaw.
	// Get the current yaw rotation and cancel it out. This is necessary so that subsystems that
	// rely on lastViewMatrix do not end up using the orientation from before the recenter if they
	// are called before the beginning of the next frame.
	float yaw;
	float pitch;
	float roll;
    d->lastViewMatrix.toEulerAngles< VAxis_Y, VAxis_X, VAxis_Z, VRotate_CCW, VHanded_R >(&yaw, &pitch, &roll);

	// undo the yaw
    VMatrix4f unrotYawMatrix(VQuatf(VAxis_Y, -yaw));
    d->lastViewMatrix = d->lastViewMatrix * unrotYawMatrix;
}

void App::setRecenterYawFrameStart(const long long frameNumber)
{
    d->recenterYawFrameStart = frameNumber;
}

long long App::recenterYawFrameStart() const
{
    return d->recenterYawFrameStart;
}

// Debug tool to draw outlines of a 3D bounds
//void App::drawBounds( const V3Vectf &mins, const V3Vectf &maxs, const VR4Matrixf &mvp, const V3Vectf &color )
//{

//
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

void App::drawEyeViewsPostDistorted( VMatrix4f const & centerViewMatrix, const int numPresents )
{
    // update vr lib systems after the app frame, but before rendering anything

    worldFontSurface().Finish( centerViewMatrix );

    // Increase the fov by about 10 degrees if we are not holding 60 fps so
    // there is less black pull-in at the edges.
    //
    // Doing this dynamically based just on time causes visible flickering at the
    // periphery when the fov is increased, so only do it if minimumVsyncs is set.
    const float fovDegrees = VDevice::instance()->eyeDisplayFov[0] +
            ( (d->swapParms.MinimumVsyncs > 1 ) ? 10.0f : 0.0f ) +
            ( ( !d->showVignette ) ? 5.0f : 0.0f );

    // DisplayMonoMode uses a single eye rendering for speed improvement
    // and / or high refresh rate double-scan hardware modes.
    const int numEyes = d->renderMonoMode || VEyeItem::settings.useMultiview? 1 : 2;

    // Flush out and report any errors
    d->m_glStatus.logErrorsEnum("FrameStart");

    if ( d->drawCalibrationLines && d->calibrationLinesDrawn )
    {
        // doing a time warp test, don't generate new images
        vInfo( "drawCalibrationLines && calibrationLinesDrawn" );
    }
    else
    {

        for(int eye = 0;eye<numEyes;++eye)
        {
            d->gui->prepare();
            d->eyeItem->paint(eye);

            // Call back to the app for drawing.
            d->activity->drawEyeView(eye, fovDegrees);

            for (VModel *model : d->models) {
                model->draw(eye);
            }

            worldFontSurface().Render3D(defaultFont(),eye);

            d->gui->update(eye);
            d->gui->commit();

            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);

            // Optionally draw thick calibration lines into the texture,
            // which will be overlayed by the thinner origin cross when
            // distorted to the window.
            if (d->drawCalibrationLines) {
                //TODO need update when use multiview
                d->eyeDecorations.DrawEyeCalibrationLines(fovDegrees, eye);
                d->calibrationLinesDrawn = true;
            }
            else {
                d->calibrationLinesDrawn = false;
            }

            dialog.draw(eye);

            if (d->showVignette) {
                // Draw a thin vignette at the edges of the view so clamping will give black
                // This will not be reflected correctly in overlay planes.
                // EyeDecorations.DrawEyeVignette();

                d->eyeDecorations.FillEdge(VEyeItem::settings.resolution, VEyeItem::settings.resolution);
            }

            d->eyeItem->afterPaint(eye);
        }
    }


    // This eye set is complete, use it now.
    if ( numPresents > 0 )
    {
        for(int eye = 0;eye<=1;++eye)
        {
            d->swapParms.Images[eye][0].TexCoordsFromTanAngles = VMatrix4f::TanAngleMatrixFromFov( fovDegrees );
            d->swapParms.Images[eye][0].TexId = d->eyeItem->completedEyes(d->renderMonoMode ? 0 : eye ).textures;
            d->swapParms.Images[eye][0].Pose  = d->sensorForNextWarp;
            // d->kernel->m_smoothProgram = ChromaticAberrationCorrection(glOperation) ? WP_CHROMATIC : WP_SIMPLE;
        }

        //d->kernel->doSmooth();
        d->kernel->doSmooth(&d->swapParms );
    }
}

// Draw a screen to an eye buffer the same way it would be drawn as a
// time warp overlay.
//void App::drawScreenDirect( const GLuint texid, const ovrMatrix4f & mvp )
//{
//
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
void App::drawScreenMask(const VMatrix4f &mvp, const float fadeFracX, const float fadeFracY)
{
    glUseProgram(d->overlayScreenFadeMaskProgram.program);

    glUniformMatrix4fv(d->overlayScreenFadeMaskProgram.uniformModelViewProMatrix, 1, GL_FALSE, mvp.transposed().cell[0]);

    if (d->fadedScreenMaskSquare.vertexArrayObject == 0) {
        d->fadedScreenMaskSquare.createScreenQuad( fadeFracX, fadeFracY );
    }

    glColorMask(0.0f, 0.0f, 0.0f, 1.0f);
    d->fadedScreenMaskSquare.drawElements();
    glColorMask(1.0f, 1.0f, 1.0f, 1.0f);
}

bool App::isShowFPS() const
{
    return d->showFPS;
}

void App::showFPS(bool const show)
{
    bool temp = d->showFPS;
    d->showFPS = show;
    if (d->showFPS && !temp) {
        d->fpsPointTracker.Reset();
    }
}

const VZipFile &App::apkFile() const
{
    static VZipFile current(packageCodePath());
    return current;
}

VMatrix4f App::getModelViewProMatrix(int eye) const
{
    return d->activity->getModelViewProMatrix(eye);
}


NV_NAMESPACE_END
