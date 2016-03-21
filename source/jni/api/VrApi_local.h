#pragma once

#include "DirectRender.h"
#include "HmdInfo.h"
#include "VFrameSmooth.h"

NV_USING_NAMESPACE

struct ovrMobile
{
	// To avoid problems with game loops that may inadvertantly
	// call WarpSwap() after doing LeaveVrMode(), the structure
	// is never actually freed, but just flagged as already destroyed.
	bool					Destroyed;

	// Valid for the thread that called ovr_EnterVrMode
	JNIEnv	*				Jni;

	// Thread from which VR mode was entered.
	pid_t					EnterTid;

    VFrameSmooth *			Warp;
    hmdInfoInternal_t	HmdInfo;
	ovrModeParms			Parms;
    TimeWarpInitParms	Twp;
};
