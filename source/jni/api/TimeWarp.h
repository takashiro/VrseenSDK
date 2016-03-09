#pragma once

/*
 * Overview:
 *
 * The TimeWarp system is designed to provide a very consistent
 * and low latency rendering loop for a head mounted display, while allowing
 * an application to render at lower and more irregular frame rates.
 *
 */

#include <pthread.h>
#include "Android/GlUtils.h"		// GLuint, etc
#include "Android/LogUtils.h"
#include "DirectRender.h"
#include "VrApi.h"
#include "HmdInfo.h"

const int SCHED_FIFO_PRIORITY_NONE			= 0;
const int SCHED_FIFO_PRIORITY_VRTHREAD		= 1;
const int SCHED_FIFO_PRIORITY_DEVICEMNGR	= 2;
const int SCHED_FIFO_PRIORITY_TIMEWARP		= 3;

NV_NAMESPACE_BEGIN

enum TimeWarpMode_t
{
	TWM_SWAPBUFFERS,	// No front buffer rendering, so tools and capture programs work.
	TWM_SYNCHRONOUS,	// Front buffer from render thread for debugging.
	TWM_ASYNCHRONOUS	// Default behavior -- front buffer from background thread.
};

class TimeWarpInitParms
{
public:
	TimeWarpInitParms() :
        frontBuffer( true ),
        asynchronousTimeWarp( true ),
        enableImageServer( false ),
        distortionFileName( NULL ),
        javaVm( NULL ),
        vrLibClass( NULL ),
        activityObject( NULL ),
        gameThreadTid( 0 ),
        buildVersionSDK( 0 ) {}

	// Graphics debug and video capture tools may not like front buffer rendering.
    bool				frontBuffer;

	// Shipping applications will almost always want this on,
	// but if you want to draw directly to the screen for
	// debug tasks, you can run synchronously so the init
	// thread is still current on the window.
    bool				asynchronousTimeWarp;

	// Set true to enable the image server, which allows a
	// remote device to view frames from the current VR session.
    bool				enableImageServer;

	// If not NULL, the distortion mesh will be loaded from this file
    const char *		distortionFileName;

	// directory to load external data from
    VString				externalStorageDirectory;

    hmdInfoInternal_t	hmdInfo;

	// For changing SCHED_FIFO on the calling thread.
    JavaVM *			javaVm;
    jclass				vrLibClass;
    jobject				activityObject;
    pid_t				gameThreadTid;

    int					buildVersionSDK;
};

// Abstract interface
class TimeWarp
{
public:
	// Optionally spawns a separate thread that will warp
	// eye images directly to the screen, regardless of the
	// frame rate of the main thread.
	//
	// Call with the main OpenGL context already current, so
	// the new graphics context can know the correct display and config,
	// and can share texture objects with it.
	//
	// On return with useBackgroundThread_, the calling context will be current on
	// a tiny pbuffer, since the background context has taken over
	// the real window surface. EGL_KHR_surfaceless_context is unfortunately
	// not supported on the platforms we care about.
	//
	// Calling Startup() if TimeWarp is already active will be a fatal error.
	//
	// Any problems during startup will be a fatal error.
	//
	// Vsync must be initialized before calling.
	static TimeWarp * Factory( TimeWarpInitParms initParms );

	// The system should be able to shutdown and reinitialize multiple times
	// by an application.  Each pause of the application will require a shutdown,
	// and resume should restart.
	virtual ~TimeWarp() = 0;

	// Accepts a new pos + texture set that will be used for future warps.
	// The parms are copied, and are not referenced after the function returns.
	//
	// The application GL context that rendered the eye images must be current,
	// but drawing does not need to be completed.  In the asynchronous case, a
	// sync object will be added to the current context so the background thread
	// can know when it is ready to use.
	//
	// In the async case, this will block until the textures from the previous
	// WarpSwap have completed rendering, to allow one frame of overlap for maximum
	// GPU utilization, but prevent multiple frames from piling up variable latency.
	//
	// This will block until at least one vsync has passed since the last
	// call to WarpSwap to prevent applications with simple scenes from
	// generating completely wasted frames.
	//
	// Calling with TimeWarp uninitialized will log a warning and return.
	//
	// Calling from a thread other than the one that called startup will be
	// a fatal error.
    virtual void		warpSwap( const ovrTimeWarpParms & parms ) = 0;

	// Get the thread ID so we can set SCHED_FIFO
    virtual int			warpThreadTid() const = 0;
    virtual pthread_t	warpThread() const = 0;
};

NV_NAMESPACE_END

