/************************************************************************************

Filename    :   ActionComponents.h
Content     :   Misc. VRMenu Components to handle actions
Created     :   September 12, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#include "ActionComponents.h"

namespace NervGear {

//==============================
// OvrButton_OnUp::OnEvent_Impl
eMsgStatus OvrButton_OnUp::onEventImpl( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
	VRMenuObject * self, VRMenuEvent const & event )
{
    vAssert(event.eventType == VRMENU_EVENT_TOUCH_UP);
    vInfo("Button id" << m_buttonId.Get() << "clicked");
	m_menu->onItemEvent( app, m_buttonId, event );
	return MSG_STATUS_CONSUMED;
}

}
