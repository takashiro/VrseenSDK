/************************************************************************************

Filename    :   GuiSys.h
Content     :   Manager for native GUIs.
Created     :   June 6, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#if !defined( OVR_OvrGuiSys_h )
#define OVR_OvrGuiSys_h

#include "VRMenuObject.h"
#include "../KeyState.h"

NV_NAMESPACE_BEGIN

class VRMenuEvent;
struct VrFrame;
class App;
class VRMenu;

//==============================================================
// OvrGuiSys
class OvrGuiSys
{
public:
    static char const *		APP_MENU_NAME;
	static Vector4f const	BUTTON_DEFAULT_TEXT_COLOR;
	static Vector4f const	BUTTON_HILIGHT_TEXT_COLOR;

	virtual				~OvrGuiSys() { }

    virtual void		init( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font ) = 0;
    virtual void		shutdown( OvrVRMenuMgr & menuMgr ) = 0;
    virtual void		frame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                BitmapFont const & font , BitmapFontSurface & fontSurface,
								Matrix4f const & viewMatrix ) = 0;

	// called when the app menu is up and a key event is received. Return true if the menu consumed
	// the event.
    virtual bool		onKeyEvent( App * app, int const keyCode, KeyState::eKeyEventType const eventType ) = 0;
	// called when re-orient is done from Universal Menu or mount on 
    virtual void		resetMenuOrientations( App * app, Matrix4f const & viewMatrix ) = 0;

    // Add a new menu that can be opened to receive events
    virtual void        addMenu( VRMenu * menu ) = 0;
	// Removes and frees a menu that was previously added
    virtual void		destroyMenu( OvrVRMenuMgr & menuMgr, VRMenu * menu ) = 0;

	// Return the menu with the matching name
    virtual VRMenu *	getMenu( char const * menuName ) const = 0;

	// Opens a menu and places it in the active list
    virtual void		openMenu( App * app, OvrGazeCursor & gazeCursor, char const * name ) = 0;
	// Closes a menu. It will be removed from the active list once it has finished closing.
    virtual void		closeMenu( App * app, const char * name, bool const closeInstantly ) = 0;
	// Closes a menu. It will be removed from the active list once it has finished closing.
    virtual void		closeMenu( App * app, VRMenu * menu, bool const closeInstantly ) = 0;

    virtual bool		isMenuActive( char const * menuName ) const = 0;
    virtual bool		isAnyMenuActive() const = 0;
    virtual bool		isAnyMenuOpen() const = 0;
};

NV_NAMESPACE_END

#endif // OVR_OvrGuiSys_h
