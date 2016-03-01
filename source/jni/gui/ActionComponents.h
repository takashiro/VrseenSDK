/************************************************************************************

Filename    :   ActionComponents.h
Content     :   Misc. VRMenu Components to handle actions
Created     :   September 12, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#if !defined( OVR_ActionComponents_h )
#define OVR_ActionComponents_h

#include "VRMenuComponent.h"
#include "VRMenu.h"

NV_NAMESPACE_BEGIN

class VRMenu;

//==============================================================
// OvrButton_OnUp
// This is a generic component that forwards a touch up to a menu (normally its owner)
class OvrButton_OnUp : public VRMenuComponent_OnTouchUp
{
public:
	static const int TYPE_ID = 1010;

	OvrButton_OnUp( VRMenu * menu, VRMenuId_t const buttonId ) :
		VRMenuComponent_OnTouchUp(),
        m_menu( menu ),
        m_buttonId( buttonId )
	{
	}

    void setId( VRMenuId_t	newButtonId ) { m_buttonId = newButtonId; }

    virtual int		typeId( ) const { return TYPE_ID; }
	
private:
	virtual eMsgStatus  onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
		VRMenuObject * self, VRMenuEvent const & event );

private:
    VRMenu *	m_menu;		// menu that holds the button
    VRMenuId_t	m_buttonId;	// id of the button this control handles
};

}

#endif //OVR_ActionComponents_h
