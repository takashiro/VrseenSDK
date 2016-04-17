#include "VRMenuComponent.h"

#include "VKernel.h"		// ovrPoseStatef
#include "VFrame.h"
#include "App.h"
#include "VRMenuMgr.h"

namespace NervGear {

	const char * VRMenuComponent::TYPE_NAME = "";

//==============================
// VRMenuComponent::OnEvent
eMsgStatus VRMenuComponent::onEvent( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
    vAssert( self != NULL );

    //-------------------
    // do any pre work that every event handler must do
    //-------------------

    //vInfo("VrMenu: OnEvent '" << VRMenuEventTypeNames[event.EventType] << "' for '" << self->GetText() << "'");

    // call the overloaded implementation
    eMsgStatus status = onEventImpl( app, vrFrame, menuMgr, self, event );

    //-------------------
    // do any post work that every event handle must do
    //-------------------

    return status;
}


} // namespace NervGear
