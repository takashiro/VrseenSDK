/************************************************************************************

Filename    :   VRMenu.h
Content     :   Class that implements the basic framework for a VR menu, holds all the
				components for a single menu, and updates the VR menu event handler to
				process menu events for a single menu.
Created     :   June 30, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#if !defined( OVR_VRMenu_h )
#define OVR_VRMenu_h

#include "Android/LogUtils.h"

#include "Std.h"
#include "VRMenuObject.h"
#include "SoundLimiter.h"
#include "GazeCursor.h"
#include "KeyState.h"

NV_NAMESPACE_BEGIN

class App;
struct VrFrame;
class BitmapFont;
class BitmapFontSurface;
class VRMenuEventHandler;

//==============================
// DeletePointerArray
// helper function for cleaning up an array of pointers
template< typename T > 
void DeletePointerArray( Array< T* > & a )
{
    for ( int i = 0; i < a.length(); ++i )
    {
        delete a[i];
        a[i] = NULL;
    }
    a.clear();
}

enum eVRMenuFlags
{
    // initially place the menu in front of the user's view on the horizon plane but do not move to follow
    // the user's gaze.
    VRMENU_FLAG_PLACE_ON_HORIZON,
	// place the menu directly in front of the user's view on the horizon plane each frame. The user will
	// sill be able to gaze track vertically.
	VRMENU_FLAG_TRACK_GAZE_HORIZONTAL,
	// place the menu directly in front of the user's view each frame -- this means gaze tracking won't
	// be available since the user can't look at different parts of the menu.
	VRMENU_FLAG_TRACK_GAZE,
	// If set, just consume the back key but do nothing with it (for warning menu that must be accepted)
	VRMENU_FLAG_BACK_KEY_DOESNT_EXIT,
	// If set, a short-press of the back key will exit the app when in this menu
	VRMENU_FLAG_BACK_KEY_EXITS_APP,
	// If set, return false so short press is passed to app
	VRMENU_FLAG_SHORT_PRESS_HANDLED_BY_APP
};

typedef VFlags<eVRMenuFlags> VRMenuFlags_t;

//==============================================================
// VRMenu
class VRMenu
{
public:
	friend class OvrGuiSysLocal;

	enum eMenuState {
        MENUSTATE_OPENING,
		MENUSTATE_OPEN,
        MENUSTATE_CLOSING,
		MENUSTATE_CLOSED,
        MENUSTATE_MAX
	};

    static char const * MenuStateNames[MENUSTATE_MAX];

	static VRMenuId_t GetRootId();

	static VRMenu *			Create( char const * menuName );

    void					init( OvrVRMenuMgr & menuMgr, BitmapFont const & font, float const menuDistance,
									VRMenuFlags_t const & flags, Array< VRMenuComponent* > comps = Array< VRMenuComponent* >( ) );
    void					initWithItems( OvrVRMenuMgr & menuMgr, BitmapFont const & font, float const menuDistance,
									VRMenuFlags_t const & flags, Array< VRMenuObjectParms const * > & itemParms );
    void                    addItems( OvrVRMenuMgr & menuMgr, BitmapFont const & font,
                                    Array< VRMenuObjectParms const * > & itemParms,
                                    menuHandle_t parentHandle, bool const recenter );
    void					shutdown( OvrVRMenuMgr & menuMgr );
    void					frame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, BitmapFont const & font,
                                    BitmapFontSurface & fontSurface, Matrix4f const & viewMatrix, gazeCursorUserId_t const gazeUserId );
    bool					onKeyEvent( App * app, int const keyCode, KeyState::eKeyEventType const eventType );
    void					open( App * app, OvrGazeCursor & gazeCursor, bool const instant = false );
    void					close( App * app, OvrGazeCursor & gazeCursor, bool const instant = false );

    bool                    isOpen() const { return m_curMenuState == MENUSTATE_OPEN; }
    bool                    isOpenOrOpening() const { return m_curMenuState == MENUSTATE_OPEN || m_curMenuState == MENUSTATE_OPENING || m_nextMenuState == MENUSTATE_OPEN || m_nextMenuState == MENUSTATE_OPENING; }
    
    bool                    isClosed() const { return m_curMenuState == MENUSTATE_CLOSED; }
    bool                    isClosedOrClosing() const { return m_curMenuState == MENUSTATE_CLOSED || m_curMenuState == MENUSTATE_CLOSING || m_nextMenuState == MENUSTATE_CLOSED || m_nextMenuState == MENUSTATE_CLOSING; }

    void					onItemEvent( App * app, VRMenuId_t const itemId, class VRMenuEvent const & event );

	// Clients can query the current menu state but to change it they must use
	// SetNextMenuState() and allow the menu to switch states when it can.
    eMenuState				curMenuState() const { return m_curMenuState; }

	// Returns the next menu state.
    eMenuState				nextMenuState() const { return m_nextMenuState; }
	// Sets the next menu state.  The menu will switch to that state at the next
	// opportunity.
    void					setNextMenuState( eMenuState const s ) { m_nextMenuState = s; }

    menuHandle_t			rootHandle() const { return m_rootHandle; }
    menuHandle_t			focusedHandle() const;
    Posef const &			menuPose() const { return m_menuPose; }
    void					setMenuPose( Posef const & pose ) { m_menuPose = pose; }

    menuHandle_t			handleForId( OvrVRMenuMgr & menuMgr, VRMenuId_t const id ) const;

    char const *			name() const { return m_name.toCString(); }
    bool                    isMenu( char const * menuName ) const { return OVR_stricmp( m_name.toCString(), menuName ) == 0; }

	// Use an arbitrary view matrix. This is used when updating the menus and passing the current matrix
    void					repositionMenu( App * app, Matrix4f const & viewMatrix );
	// Use app's lastViewMatrix. This is used when positioning a menu more or less in front of the user
	// when we do not have the most current view matrix available.
    void					repositionMenu( App * app );

	//	Reset the MenuPose orientation - for now we assume identity orientation as the basis for all menus
    void					resetMenuOrientation( App * app, Matrix4f const & viewMatrix );
	
    VRMenuFlags_t const &	flags() const { return m_flags; }
    void					setFlags( VRMenuFlags_t	const & flags ) { m_flags = flags; }

protected:
	// only derived classes can instance a VRMenu
	VRMenu( char const * name );
	// only GuiSysLocal can free a VRMenu
	virtual ~VRMenu();

private:
    menuHandle_t			m_rootHandle;			// handle to the menu's root item (to which all other items must be attached)

    eMenuState				m_curMenuState;		// the current menu state
    eMenuState				m_nextMenuState;		// the state the menu should move to next

    Posef					m_menuPose;			// world-space position and orientation of this menu's root item

    SoundLimiter			m_openSoundLimiter;	// prevents the menu open sound from playing too often
    SoundLimiter			m_closeSoundLimiter;	// prevents the menu close sound from playing too often

    VRMenuEventHandler *	m_eventHandler;

    VString                  m_name;				// name of the menu

    VRMenuFlags_t			m_flags;				// various flags that dictate menu behavior
    float					m_menuDistance;		// distance from eyes
    bool					m_isInitialized;		// true if Init was called
    bool					m_componentsInitialized;	// true if init message has been sent to components

private:
	static Posef			CalcMenuPosition( Matrix4f const & viewMatrix, Matrix4f const & invViewMatrix,
									Vector3f const & viewPos, Vector3f const & viewFwd, float const menuDistance );
	static Posef			CalcMenuPositionOnHorizon( Matrix4f const & viewMatrix, Matrix4f const & invViewMatrix,
									Vector3f const & viewPos, Vector3f const & viewFwd, float const menuDistance );

    // return true to continue with normal initialization (adding items) or false to skip.
    virtual bool    init_Impl( OvrVRMenuMgr & menuMgr, BitmapFont const & font, float const menuDistance,
                            VRMenuFlags_t const & flags, Array< VRMenuObjectParms const * > & itemParms );
    virtual void    shutdown_Impl( OvrVRMenuMgr & menuMgr );
    virtual void    frameImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, BitmapFont const & font,
                                    BitmapFontSurface & fontSurface, gazeCursorUserId_t const gazeUserId );
    // return true if the key was consumed.
    virtual bool    onKeyEventImpl( App * app, int const keyCode, KeyState::eKeyEventType const eventType );
    virtual void    openImpl( App * app, OvrGazeCursor & gazeCursor );
    virtual void    close_Impl( App * app, OvrGazeCursor & gazeCursor );
    virtual void	onItemEvent_Impl( App * app, VRMenuId_t const itemId, class VRMenuEvent const & event );
    virtual void	resetMenuOrientation_Impl( App * app, Matrix4f const & viewMatrix );

	// return true when finished opening/closing - allowing derived menus to animate etc. during open/close
    virtual bool	isFinishedOpening() const { return true;  }
    virtual bool	isFinishedClosing() const { return true; }
};

NV_NAMESPACE_END

#endif // OVR_VRMenu_h
