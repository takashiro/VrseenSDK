/************************************************************************************

Filename    :   VRMenuComponent.h
Content     :   Menuing system for VR apps.
Created     :   June 23, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "VRMenuComponent.h"

#include "Android/Log.h"
#include "api/VKernel.h"		// ovrPoseStatef

#include "../Input.h"
#include "../App.h"
#include "../Input.h"
#include "VRMenuMgr.h"

namespace NervGear {

	const char * VRMenuComponent::TYPE_NAME = "";

//==============================
// VRMenuComponent::OnEvent
eMsgStatus VRMenuComponent::onEvent( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
    vAssert( self != NULL );

    //-------------------
    // do any pre work that every event handler must do
    //-------------------

    //DROIDLOG( "VrMenu", "OnEvent '%s' for '%s'", VRMenuEventTypeNames[event.EventType], self->GetText().toCString() );

    // call the overloaded implementation
    eMsgStatus status = onEventImpl( app, vrFrame, menuMgr, self, event );

    //-------------------
    // do any post work that every event handle must do
    //-------------------

    return status;
}


} // namespace NervGear
