
#pragma once

#include "vglobal.h"

#include <jni.h>

void ovr_InitBuildStrings( JNIEnv * env );

enum eBuildString
{
	BUILDSTR_BRAND,
	BUILDSTR_DEVICE,
	BUILDSTR_DISPLAY,
	BUILDSTR_FINGERPRINT,
	BUILDSTR_HARDWARE,
	BUILDSTR_HOST,
	BUILDSTR_ID,
	BUILDSTR_MODEL,
	BUILDSTR_PRODUCT,
	BUILDSTR_SERIAL,
	BUILDSTR_TAGS,
	BUILDSTR_TYPE,
	BUILDSTR_MAX
};

// Returns the value of a specific build string.
char const * ovr_GetBuildString( eBuildString const id );


