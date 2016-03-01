#pragma once

#include <jni.h>
#include "sensor/Stereo.h"		// for LensConfig

NV_NAMESPACE_BEGIN

struct hmdInfoInternal_t
{
	LensConfig lens;

	float	lensSeparation;		// in meters

	// These values are always as if the display is in landscape
	// mode, being swapped from the system values if the manifest
	// is configured for portrait.
	float	widthMeters;		// in meters
	float	heightMeters;		// in meters
	int		widthPixels;		// in pixels
	int		heightPixels;		// in pixels
	float	horizontalOffsetMeters; // the horizontal offset between the screen center and midpoint between lenses

	// Refresh rate of the display.
	float	displayRefreshRate;

	// Currently returns a conservative 1024x1024
	int		eyeTextureResolution[2];

	// This is a product of the lens distortion and the screen size,
	// but there is no truly correct answer.
	//
	// There is a tradeoff in resolution and coverage.
	// Too small of an fov will leave unrendered pixels visible, but too
	// large wastes resolution or fill rate.  It is unreasonable to
	// increase it until the corners are completely covered, but we do
	// want most of the outside edges completely covered.
	//
	// Applications might choose to render a larger fov when angular
	// acceleration is high to reduce black pull in at the edges by
	// TimeWarp.
	float	eyeTextureFov[2];
};

// The activity may not be a vrActivity if we are in Unity.
// We can't look up vrActivityClass here, because we may be on a non-startup thread
// in native apps.
hmdInfoInternal_t GetDeviceHmdInfo( const char * buildModel, JNIEnv * env, jobject activity, jclass vrActivityClass );
}


