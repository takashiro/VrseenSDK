/************************************************************************************

Filename    :   VRMenuEvent.cpp
Content     :   Event class for menu events.
Created     :   June 23, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#include "VRMenuEvent.h"

namespace NervGear {

char const * VRMenuEvent::EventTypeNames[] =
{
	"VRMENU_EVENT_FOCUS_GAINED",
	"VRMENU_EVENT_FOCUS_LOST",
	"VRMENU_EVENT_TOUCH_DOWN",
	"VRMENU_EVENT_TOUCH_UP",
    "VRMENU_EVENT_TOUCH_RELATIVE",
    "VRMENU_EVENT_TOUCH_ABSOLUTE",
	"VRMENU_EVENT_SWIPE_FORWARD",
	"VRMENU_EVENT_SWIPE_BACK",
	"VRMENU_EVENT_SWIPE_UP",
	"VRMENU_EVENT_SWIPE_DOWN",
    "VRMENU_EVENT_FRAME_UPDATE",
    "VRMENU_EVENT_SUBMIT_FOR_RENDERING",
    "VRMENU_EVENT_RENDER",
	"VRMENU_EVENT_OPENING",
	"VRMENU_EVENT_OPENED",
	"VRMENU_EVENT_CLOSING",
	"VRMENU_EVENT_CLOSED",
	"VRMENU_EVENT_INIT"
};

} // namespace NervGear