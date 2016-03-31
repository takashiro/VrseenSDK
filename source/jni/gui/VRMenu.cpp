/************************************************************************************

Filename    :   VRMenu.cpp
Content     :   Class that implements the basic framework for a VR menu, holds all the
				components for a single menu, and updates the VR menu event handler to
				process menu events for a single menu.
Created     :   June 30, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

// includes required for VRMenu.h

#include "VRMenu.h"

#include <android/keycodes.h>
#include "VRMenuMgr.h"
#include "VRMenuEventHandler.h"
#include "App.h"
#include "GuiSys.h"
#include "VArray.h"

namespace NervGear {

char const * VRMenu::MenuStateNames[MENUSTATE_MAX] =
{
    "MENUSTATE_OPENING",
	"MENUSTATE_OPEN",
    "MENUSTATE_CLOSING",
	"MENUSTATE_CLOSED"
};

// singleton so that other ids initialized during static initialization can be based off tis
VRMenuId_t VRMenu::GetRootId()
{
	VRMenuId_t ID_ROOT( -1 );
	return ID_ROOT;
}

//==============================
// VRMenu::VRMenu
VRMenu::VRMenu( char const * name ) :
	m_curMenuState( MENUSTATE_CLOSED ),
	m_nextMenuState( MENUSTATE_CLOSED ),
	m_eventHandler( NULL ),
	m_name( name ),
	m_menuDistance( 1.45f ),
	m_isInitialized( false ),
	m_componentsInitialized( false )
{
	m_eventHandler = new VRMenuEventHandler;
}

//==============================
// VRMenu::~VRMenu
VRMenu::~VRMenu()
{
	delete m_eventHandler;
	m_eventHandler = NULL;
}

//==============================
// VRMenu::Create
VRMenu * VRMenu::Create( char const * menuName )
{
	return new VRMenu( menuName );
}

//==============================
// VRMenu::Init
void VRMenu::init( OvrVRMenuMgr & menuMgr, BitmapFont const & font, float const menuDistance, VRMenuFlags_t const & flags, VArray< VRMenuComponent* > comps /*= Array< VRMenuComponent* >( )*/ )
{
	vAssert( !m_rootHandle.IsValid() );
    vAssert( !m_name.isEmpty() );

	m_flags = flags;
	m_menuDistance = menuDistance;

	// create an empty root item
	VRMenuObjectParms rootParms( VRMENU_CONTAINER, comps,
            VRMenuSurfaceParms( "root" ), "Root",
            VPosf(), V3Vectf( 1.0f, 1.0f, 1.0f ), VRMenuFontParms(),
			GetRootId(), VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t() );
	m_rootHandle = menuMgr.createObject( rootParms );
	VRMenuObject * root = menuMgr.toObject( m_rootHandle );
	if ( root == NULL )
	{
		vWarn("RootHandle (" << m_rootHandle.Get() << ") is invalid!");
		return;
	}

	m_isInitialized = true;
	m_componentsInitialized = false;
}

void VRMenu::initWithItems( OvrVRMenuMgr & menuMgr, BitmapFont const & font, float const menuDistance, VRMenuFlags_t const & flags, VArray< VRMenuObjectParms const * > & itemParms )
{
	init( menuMgr, font, menuDistance, flags );
	addItems( menuMgr, font, itemParms, rootHandle(), true );
}

//==============================================================
// ChildParmsPair
class ChildParmsPair
{
public:
	ChildParmsPair( menuHandle_t const handle, VRMenuObjectParms const * parms ) :
		Handle( handle ),
		Parms( parms )
	{
	}
	ChildParmsPair() :
		Parms( NULL )
	{
	}

	menuHandle_t				Handle;
	VRMenuObjectParms const *	Parms;
};

//==============================
// VRMenu::AddItems
void VRMenu::addItems( OvrVRMenuMgr & menuMgr, BitmapFont const & font,
        NervGear::VArray< VRMenuObjectParms const * > & itemParms,
        menuHandle_t parentHandle, bool const recenter )
{
    const V3Vectf fwd( 0.0f, 0.0f, 1.0f );
    const V3Vectf up( 0.0f, 1.0f, 0.0f );
    const V3Vectf left( 1.0f, 0.0f, 0.0f );

	// create all items in the itemParms array, add each one to the parent, and position all items
	// without the INIT_FORCE_POSITION flag vertically, one on top of the other
    VRMenuObject * root = menuMgr.toObject( parentHandle );
    vAssert( root != NULL );

	VArray< ChildParmsPair > pairs;

    V3Vectf nextItemPos( 0.0f );
	int childIndex = 0;
	for ( int i = 0; i < itemParms.length(); ++i )
	{
        VRMenuObjectParms const * parms = itemParms[i];
		// assure all ids are different
		for ( int j = 0; j < itemParms.length(); ++j )
		{
			if ( j != i && parms->Id != VRMenuId_t() && parms->Id == itemParms[j]->Id )
			{
                vInfo("Duplicate menu object ids for" << parms->Text << "and" << itemParms[j]->Text << "!");
			}
		}
		menuHandle_t handle = menuMgr.createObject( *parms );
		if ( handle.IsValid() && root != NULL )
		{
			pairs.append( ChildParmsPair( handle, parms ) );
			root->addChild( menuMgr, handle );
			VRMenuObject * obj = menuMgr.toObject( root->getChildHandleForIndex( childIndex++ ) );
			if ( obj != NULL && ( parms->InitFlags & VRMENUOBJECT_INIT_FORCE_POSITION ) == 0 )
			{
                VBoxf const & lb = obj->getTextLocalBounds( font );
                V3Vectf size = lb.GetSize() * obj->localScale();
                V3Vectf centerOfs( left * ( size.x * -0.5f ) );
				if ( !parms->ParentId.IsValid() )	// only contribute to height if not being reparented
				{
					// stack the items
                    obj->setLocalPose( VPosf( VQuatf(), nextItemPos + centerOfs ) );
					// calculate the total height
					nextItemPos += up * size.y;
				}
				else // otherwise center local to parent
				{
                    obj->setLocalPose( VPosf( VQuatf(), centerOfs ) );
				}
			}
		}
	}

	// reparent
    VArray< menuHandle_t > reparented;
	for ( int i = 0; i < pairs.length(); ++i )
	{
		ChildParmsPair const & pair = pairs[i];
		if ( pair.Parms->ParentId.IsValid() )
		{
			menuHandle_t parentHandle = handleForId( menuMgr, pair.Parms->ParentId );
			VRMenuObject * parent = menuMgr.toObject( parentHandle );
			if ( parent != NULL )
			{
				root->removeChild( menuMgr, pair.Handle );
				parent->addChild( menuMgr, pair.Handle );
			}
		}
	}

    if ( recenter )
    {
	    // center the menu based on the height of the auto-placed children
	    float offset = nextItemPos.y * 0.5f;
        V3Vectf rootPos = root->localPosition();
	    rootPos -= offset * up;
	    root->setLocalPosition( rootPos );
    }
}

//==============================
// VRMenu::Shutdown
void VRMenu::shutdown( OvrVRMenuMgr & menuMgr )
{
    vAssert(m_isInitialized);

	// this will free the root and all children
	if ( m_rootHandle.IsValid() )
	{
		menuMgr.freeObject( m_rootHandle );
		m_rootHandle.Release();
	}
}

//==============================
// VRMenu::Frame
void VRMenu::repositionMenu( App * app, VR4Matrixf const & viewMatrix )
{
    const VR4Matrixf invViewMatrix = viewMatrix.Inverted();
    const V3Vectf viewPos( GetViewMatrixPosition( viewMatrix ) );
    const V3Vectf viewFwd( GetViewMatrixForward( viewMatrix ) );

	if ( m_flags & VRMENU_FLAG_TRACK_GAZE )
	{
		m_menuPose = CalcMenuPosition( viewMatrix, invViewMatrix, viewPos, viewFwd, m_menuDistance );
	}
	else if ( m_flags & VRMENU_FLAG_PLACE_ON_HORIZON )
	{
		m_menuPose = CalcMenuPositionOnHorizon( viewMatrix, invViewMatrix, viewPos, viewFwd, m_menuDistance );
	}
}

//==============================
// VRMenu::Frame
void VRMenu::repositionMenu( App * app )
{
	repositionMenu( app, app->lastViewMatrix() );
}

//==============================
// VRMenu::Frame
void VRMenu::frame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        BitmapFont const & font, BitmapFontSurface & fontSurface, VR4Matrixf const & viewMatrix,
        gazeCursorUserId_t const gazeUserId )
{
    const VR4Matrixf invViewMatrix = viewMatrix.Inverted();
    const V3Vectf viewPos( GetViewMatrixPosition( viewMatrix ) );
    const V3Vectf viewFwd( GetViewMatrixForward( viewMatrix ) );

	VArray< VRMenuEvent > events;

	if ( !m_componentsInitialized )
	{
		m_eventHandler->initComponents( events );
		m_componentsInitialized = true;
	}

	if ( m_nextMenuState != m_curMenuState )
	{
		vInfo("NextMenuState for '" << name() << "':" << MenuStateNames[m_nextMenuState]);
		switch( m_nextMenuState )
		{
            case MENUSTATE_OPENING:
				repositionMenu( app, viewMatrix );
				m_eventHandler->opening( events );
                break;
			case MENUSTATE_OPEN:
				{
                    m_openSoundLimiter.playMenuSound( app, m_name, "sv_release_active", 0.1 );
					m_eventHandler->opened( events );
				}
				break;
            case MENUSTATE_CLOSING:
				m_eventHandler->closing( events );
                break;
			case MENUSTATE_CLOSED:
				{
                    m_closeSoundLimiter.playMenuSound( app, m_name, "sv_deselect", 0.1 );
					m_eventHandler->closed( events );
				}
				break;
            default:
                vAssert( !"Unhandled menu state!" );
                break;
		}
		m_curMenuState = m_nextMenuState;
	}

    switch( m_curMenuState )
    {
        case MENUSTATE_OPENING:
			if ( isFinishedOpening() )
			{
				m_nextMenuState = MENUSTATE_OPEN;
			}
            break;
        case MENUSTATE_OPEN:
            break;
        case MENUSTATE_CLOSING:
			if ( isFinishedClosing() )
			{
				m_nextMenuState = MENUSTATE_CLOSED;
			}
            break;
        case MENUSTATE_CLOSED:
	        // handle remaining events -- not focus path is empty right now, but this may still broadcast messages to controls
		    m_eventHandler->handleEvents( app, vrFrame, menuMgr, m_rootHandle, events );
		    return;
        default:
            vAssert( !"Unhandled menu state!" );
            break;
	}

	if ( m_flags & VRMENU_FLAG_TRACK_GAZE )
	{
		m_menuPose = CalcMenuPosition( viewMatrix, invViewMatrix, viewPos, viewFwd, m_menuDistance );
	}
	else if ( m_flags & VRMENU_FLAG_TRACK_GAZE_HORIZONTAL )
	{
		m_menuPose = CalcMenuPositionOnHorizon( viewMatrix, invViewMatrix, viewPos, viewFwd, m_menuDistance );
	}

    frameImpl( app, vrFrame, menuMgr, font, fontSurface, gazeUserId );

	m_eventHandler->frame( app, vrFrame, menuMgr, font, m_rootHandle, m_menuPose, gazeUserId, events );

	m_eventHandler->handleEvents( app, vrFrame, menuMgr, m_rootHandle, events );

	VRMenuObject * root = menuMgr.toObject( m_rootHandle );
	if ( root != NULL )
	{
		VRMenuRenderFlags_t renderFlags;
		menuMgr.submitForRendering( app->debugLines(), font, fontSurface, m_rootHandle, m_menuPose, renderFlags );
	}

}

//==============================
// VRMenu::OnKeyEvent
bool VRMenu::onKeyEvent( App * app, int const keyCode, KeyState::eKeyEventType const eventType )
{
    if ( onKeyEventImpl( app, keyCode, eventType ) )
    {
        // consumed by sub class
        return true;
    }

	if ( keyCode == AKEYCODE_BACK )
	{
		vInfo("VRMenu '" << name() << "' Back key event:" << KeyState::EventNames[eventType]);

		switch( eventType )
		{
			case KeyState::KEY_EVENT_LONG_PRESS:
				return false;
			case KeyState::KEY_EVENT_DOWN:
				return false;
			case KeyState::KEY_EVENT_SHORT_PRESS:
				if ( isOpenOrOpening() )
				{
					if ( m_flags & VRMENU_FLAG_BACK_KEY_EXITS_APP )
					{
                        //app->StartSystemActivity( PUI_CONFIRM_QUIT );
                        return false;
					}
					else if ( m_flags & VRMENU_FLAG_SHORT_PRESS_HANDLED_BY_APP )
					{
						return false;
					}
					else if ( !( m_flags & VRMENU_FLAG_BACK_KEY_DOESNT_EXIT ) )
					{
						close( app, app->gazeCursor() );
						return true;
					}
				}
				return false;
			case KeyState::KEY_EVENT_DOUBLE_TAP:
			default:
				return false;
		}
	}
	return false;
}

//==============================
// VRMenu::Open
void VRMenu::open( App * app, OvrGazeCursor & gazeCursor, bool const instant )
{
	if ( m_curMenuState == MENUSTATE_CLOSED || m_curMenuState == MENUSTATE_CLOSING )
	{
        openImpl( app, gazeCursor );
		m_nextMenuState = instant ? MENUSTATE_OPEN : MENUSTATE_OPENING;
	}
}

//==============================
// VRMenu::Close
void VRMenu::close( App * app, OvrGazeCursor & gazeCursor, bool const instant )
{
	if ( m_curMenuState == MENUSTATE_OPEN || m_curMenuState == MENUSTATE_OPENING )
	{
        close_Impl( app, gazeCursor );
		m_nextMenuState = instant ? MENUSTATE_CLOSED : MENUSTATE_CLOSING;
	}
}

//==============================
// VRMenu::HandleForId
menuHandle_t VRMenu::handleForId( OvrVRMenuMgr & menuMgr, VRMenuId_t const id ) const
{
	VRMenuObject * root = menuMgr.toObject( m_rootHandle );
	return root->childHandleForId( menuMgr, id );
}

//==============================
// VRMenu::CalcMenuPosition
VPosf VRMenu::CalcMenuPosition( VR4Matrixf const & viewMatrix, VR4Matrixf const & invViewMatrix,
        V3Vectf const & viewPos, V3Vectf const & viewFwd, float const menuDistance )
{
	// spawn directly in front
    VQuatf rotation( -viewFwd, 0.0f );
    VQuatf viewRot( invViewMatrix );
    VQuatf fullRotation = rotation * viewRot;

    V3Vectf position( viewPos + viewFwd * menuDistance );

    return VPosf( fullRotation, position );
}

//==============================
// VRMenu::CalcMenuPositionOnHorizon
VPosf VRMenu::CalcMenuPositionOnHorizon( VR4Matrixf const & viewMatrix, VR4Matrixf const & invViewMatrix,
        V3Vectf const & viewPos, V3Vectf const & viewFwd, float const menuDistance )
{
	// project the forward view onto the horizontal plane
    V3Vectf const up( 0.0f, 1.0f, 0.0f );
	float dot = viewFwd.Dot( up );
    V3Vectf horizontalFwd = ( dot < -0.99999f || dot > 0.99999f ) ? V3Vectf( 1.0f, 0.0f, 0.0f ) : viewFwd - ( up * dot );
	horizontalFwd.Normalize();

    VR4Matrixf horizontalViewMatrix = VR4Matrixf::LookAtRH( V3Vectf( 0 ), horizontalFwd, up );
	horizontalViewMatrix.Transpose();	// transpose because we want the rotation opposite of where we're looking

	// this was only here to test rotation about the local axis
    //VQuatf rotation( -horizontalFwd, 0.0f );

    VQuatf viewRot( horizontalViewMatrix );
    VQuatf fullRotation = /*rotation * */viewRot;

    V3Vectf position( viewPos + horizontalFwd * menuDistance );

    return VPosf( fullRotation, position );
}

//==============================
// VRMenu::OnItemEvent
void VRMenu::onItemEvent( App * app, VRMenuId_t const itemId, VRMenuEvent const & event )
{
	onItemEvent_Impl( app, itemId, event );
}

//==============================
// VRMenu::Init_Impl
bool VRMenu::init_Impl( OvrVRMenuMgr & menuMgr, BitmapFont const & font, float const menuDistance,
                VRMenuFlags_t const & flags, VArray< VRMenuObjectParms const * > & itemParms )
{
    return true;
}

//==============================
// VRMenu::Shutdown_Impl
void VRMenu::shutdown_Impl( OvrVRMenuMgr & menuMgr )
{
}

//==============================
// VRMenu::Frame_Impl
void VRMenu::frameImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, BitmapFont const & font,
        BitmapFontSurface & fontSurface, gazeCursorUserId_t const gazeUserId )
{
}

//==============================
// VRMenu::OnKeyEvent_Impl
bool VRMenu::onKeyEventImpl( App * app, int const keyCode, KeyState::eKeyEventType const eventType )
{
    return false;
}

//==============================
// VRMenu::Open_Impl
void VRMenu::openImpl( App * app, OvrGazeCursor & gazeCursor )
{
}

//==============================
// VRMenu::Close_Impl
void VRMenu::close_Impl( App * app, OvrGazeCursor & gazeCursor )
{
}

//==============================
// VRMenu::OnItemEvent_Impl
void VRMenu::onItemEvent_Impl( App * app, VRMenuId_t const itemId, VRMenuEvent const & event )
{
}

//==============================
// VRMenu::GetFocusedHandle()
menuHandle_t VRMenu::focusedHandle() const
{
	if ( m_eventHandler != NULL )
	{
		return m_eventHandler->focusedHandle();
	}
	return menuHandle_t();
}

//==============================
// VRMenu::ResetMenuOrientation
void VRMenu::resetMenuOrientation( App * app, VR4Matrixf const & viewMatrix )
{
	vInfo("ResetMenuOrientation for '" << name() << "'");
	resetMenuOrientation_Impl( app, viewMatrix );
}

//==============================
// VRMenuResetMenuOrientation_Impl
void VRMenu::resetMenuOrientation_Impl( App * app, VR4Matrixf const & viewMatrix )
{
	repositionMenu( app, viewMatrix );
}

} // namespace NervGear

