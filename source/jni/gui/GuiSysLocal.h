/************************************************************************************

Filename    :   GuiSysLocal.h
Content     :   The main menu that appears in native apps when pressing the HMT button.
Created     :   July 22, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#if !defined( OVR_OvrGuiSysLocal_h )
#define OVR_OvrGuiSysLocal_h

#include "VRMenu.h"
#include "GuiSys.h"

NV_NAMESPACE_BEGIN

//==============================================================
// OvrGuiSysLocal
class OvrGuiSysLocal : public OvrGuiSys
{
public:
							OvrGuiSysLocal();
	virtual					~OvrGuiSysLocal();

	virtual void			init( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font );
	virtual void			shutdown( OvrVRMenuMgr & menuMgr );
	virtual void			frame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    BitmapFont const & font, BitmapFontSurface & fontSurface,
                                    VR4Matrixf const & viewMatrix );
	virtual bool			onKeyEvent( App * app, int const keyCode, KeyState::eKeyEventType const eventType );
    virtual void			resetMenuOrientations( App * app, VR4Matrixf const & viewMatrix );

    virtual void            addMenu( VRMenu * menu );
	virtual VRMenu *		getMenu( char const * menuName ) const;
	virtual void			destroyMenu( OvrVRMenuMgr & menuMgr, VRMenu * menu );
    virtual void			openMenu( App * app, OvrGazeCursor & gazeCursor, char const * menuName );
	virtual void			closeMenu( App * app, char const * menuName, bool const closeInstantly );
	virtual void			closeMenu( App * app, VRMenu * menu, bool const closeInstantly );
	virtual bool			isMenuActive( char const * menuName ) const;
	virtual bool			isAnyMenuActive() const;
	virtual bool			isAnyMenuOpen() const;

	gazeCursorUserId_t		GetGazeUserId() const { return GazeUserId; }

private:
	VArray< VRMenu* >	    Menus;
	VArray< VRMenu* >		ActiveMenus;

    bool					IsInitialized;

    gazeCursorUserId_t		GazeUserId;			// user id for the gaze cursor

private:
    int                     FindMenuIndex( char const * menuName ) const;
	int						FindMenuIndex( VRMenu const * menu ) const;
	int						FindActiveMenuIndex( VRMenu const * menu ) const;
	int						FindActiveMenuIndex( char const * menuName ) const;
	void					MakeActive( VRMenu * menu );
	void					MakeInactive( VRMenu * menu );

    VArray< VRMenuComponent* > GetDefaultComponents();
};

NV_NAMESPACE_END

#endif // OVR_OvrGuiSysLocal_h
