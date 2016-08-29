#pragma once

#include "VRotationState.h"
#include "VMatrix.h"
#include <jni.h>
#include <android/native_window_jni.h>

NV_NAMESPACE_BEGIN

enum {
//    VK_INHIBIT_SRGB_FB = 1,
//    VK_USE_S = 2,
//    VK_FLUSH = 4,
//    VK_FIXED_LAYER = 8,
//    VK_DISPLAY_CURSOR = 16,
//    VK_IMAGE = 32,
//    VK_DRAW_LINES = 64

    // To get gamma correct sRGB filtering of the eye textures, the textures must be
    // allocated with GL_SRGB8_ALPHA8 format and the window surface must be allocated
    // with these attributes:
    // EGL_GL_COLORSPACE_KHR,  EGL_GL_COLORSPACE_SRGB_KHR
    //
    // While we can reallocate textures easily enough, we can't change the window
    // colorspace without relaunching the entire application, so if you want to
    // be able to toggle between gamma correct and incorrect, you must allocate
    // the framebuffer as sRGB, then inhibit that processing when using normal
    // textures.
            SWAP_OPTION_INHIBIT_SRGB_FRAMEBUFFER	= 1,
    // Enable / disable the sliced warp
            SWAP_OPTION_USE_SLICED_WARP				= 2,
    // Flush the warp swap pipeline so the images show up immediately.
    // This is expensive and should only be used when an immediate transition
    // is needed like displaying black when resetting the HMD orientation.
            SWAP_OPTION_FLUSH						= 4,
    // The overlay plane is a HUD, and should ignore head tracking.
    // This is generally poor practice for VR.
            SWAP_OPTION_FIXED_OVERLAY				= 8,
    // The third image plane is blended separately over only a small, central
    // section of each eye for performance reasons, so it is enabled with
    // a flag instead of a shared ovrTimeWarpProgram.
            SWAP_OPTION_SHOW_CURSOR					= 16,
    // Use default images. This is used for showing black and the loading icon.
            SWAP_OPTION_DEFAULT_IMAGES				= 32,
    // Draw the axis lines after warp to show the skew with the pre-warp lines.
            SWAP_OPTION_DRAW_CALIBRATION_LINES		= 64
};

enum VrKernelProgram {
//    VK_DEFAULT,
//    VK_PLANE,
//    VK_PLANE_SPECIAL,
//    VK_CUBE,
//    VK_CUBE_SPECIAL,
//    VK_LOGO,
//    VK_HALF,
//    VK_PLANE_LAYER,
//    VK_PLANE_LOD,
//    VK_RESERVED,
//
//    VK_DEFAULT_CB,
//    VK_PLANE_CB,
//    VK_PLANE_SPECIAL_CB,
//    VK_CUBE_CB,
//    VK_CUBE_SPECIAL_CB,
//    VK_LOGO_CB,
//    VK_HALF_CB,
//    VK_PLANE_LAYER_CB,
//    VK_PLANE_LOD_CB,
//    VK_RESERVED_CB,
//
//    VK_MAX

    WP_SIMPLE,
    WP_MASKED_PLANE,						// overlay plane shows through masked areas in eyes
    WP_MASKED_PLANE_EXTERNAL,				// overlay plane shows through masked areas in eyes, using external texture as source
    WP_MASKED_CUBE,							// overlay cube shows through masked areas in eyes
    WP_CUBE,								// overlay cube only, no main scene (for power savings)
    WP_LOADING_ICON,						// overlay loading icon
    WP_MIDDLE_CLAMP,						// UE4 stereo in a single texture
    WP_OVERLAY_PLANE,						// world shows through transparent parts of overlay plane
    WP_OVERLAY_PLANE_SHOW_LOD,				// debug tool to color tint based on mip levels
    WP_CAMERA,

    // with correction for chromatic aberration
    WP_CHROMATIC,
    WP_CHROMATIC_MASKED_PLANE,				// overlay plane shows through masked areas in eyes
    WP_CHROMATIC_MASKED_PLANE_EXTERNAL,		// overlay plane shows through masked areas in eyes, using external texture as source
    WP_CHROMATIC_MASKED_CUBE,				// overlay cube shows through masked areas in eyes
    WP_CHROMATIC_CUBE,						// overlay cube only, no main scene (for power savings)
    WP_CHROMATIC_LOADING_ICON,				// overlay loading icon
    WP_CHROMATIC_MIDDLE_CLAMP,				// UE4 stereo in a single texture
    WP_CHROMATIC_OVERLAY_PLANE,				// world shows through transparent parts of overlay plane
    WP_CHROMATIC_OVERLAY_PLANE_SHOW_LOD,	// debug tool to color tint based on mip levels
    WP_CHROMATIC_CAMERA,

    WP_PROGRAM_MAX
};

enum eExitType {
    EXIT_TYPE_NONE,
    EXIT_TYPE_FINISH,
    EXIT_TYPE_FINISH_AFFINITY,
    EXIT_TYPE_EXIT
};

// Note that if overlays are dynamic, they must be triple buffered just
// like the eye images.
typedef struct
{
    // If TexId == 0, this image is disabled.
    // Most applications will have the overlay image
    // disabled.
    //
    // Because OpenGL ES doesn't support clampToBorder,
    // it is the application's responsibility to make sure
    // that all mip levels of the texture have a black border
    // that will show up when time warp pushes the texture partially
    // off screen.
    //
    // Overlap textures will only show through where alpha on the
    // primary texture is not 1.0, so they do not require a border.
    unsigned int	TexId;

    // Experimental separate R/G/B cube maps
    unsigned int	PlanarTexId[3];

    // Points on the screen are mapped by a distortion correction
    // function into ( TanX, TanY, 1, 1 ) vectors that are transformed
    // by this matrix to get ( S, T, Q, _ ) vectors that are looked
    // up with texture2dproj() to get texels.
    VMatrix4f TexCoordsFromTanAngles;

    // The sensor state for which ModelViewMatrix is correct.
    // It is ok to update the orientation for each eye, which
    // can help minimize black edge pull-in, but the position
    // must remain the same for both eyes, or the position would
    // seem to judder "backwards in time" if a frame is dropped.
    VRotationState Pose;
} ovrTimeWarpImage;

static const int	MAX_WARP_EYES = 2;
static const int	MAX_WARP_IMAGES = 3;

typedef struct
{
    // Images used for each eye.
    // Per eye: 0 = world, 1 = overlay screen, 2 = gaze cursor
    ovrTimeWarpImage 			Images[MAX_WARP_EYES][MAX_WARP_IMAGES];

    // Combination of ovrTimeWarpOption flags.
    int 						WarpOptions;

    // Rotation from a joypad can be added on generated frames to reduce
    // judder in FPS style experiences when the application framerate is
    // lower than the vsync rate.
    // This will be applied to the view space distorted
    // eye vectors before applying the rest of the time warp.
    // This will only be added when the same ovrTimeWarpParms is used for
    // more than one vsync.
    VMatrix4f ExternalVelocity;

    // WarpSwap will not return until at least this many vsyncs have
    // passed since the previous WarpSwap returned.
    // Setting to 2 will reduce power consumption and may make animation
    // more regular for applications that can't hold full frame rate.
    int							MinimumVsyncs;

    // Time in seconds to start drawing before each slice.
    // Clamped at 0.014 high and 0.002 low, but the very low
    // values will usually result in screen tearing.
    float						PreScheduleSeconds;

    // Which program to run with these images.
    VrKernelProgram			WarpProgram;

    // Program-specific tuning values.
    float						ProgramParms[4];
} ovrTimeWarpParms;

typedef enum
{
    WARP_INIT_DEFAULT,
    WARP_INIT_BLACK,
    WARP_INIT_LOADING_ICON,
    WARP_INIT_MESSAGE
} ovrWarpInit;

class VKernel
{
public:
    static VKernel *instance();
    void run();
    void exit();
    void destroy(eExitType type);

    int msaa;
    bool isRunning;
    bool asyncSmooth;
    jobject m_ActivityObject;
    ANativeWindow *m_NativeWindow;
    jmethodID	VrEnableVRModeStaticId;

    void doSmooth(const ovrTimeWarpParms * parms );

    ovrTimeWarpParms  InitTimeWarpParms( const ovrWarpInit init = WARP_INIT_DEFAULT, const unsigned int texId = 0 );
    int getBuildVersion();
private:
    VKernel();
};

NV_NAMESPACE_END
