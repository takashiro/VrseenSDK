/************************************************************************************

Filename    :   VRMenuEventHandler.cpp
Content     :   Menu component for handling hit tests and dispatching events.
Created     :   June 23, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "VRMenuEventHandler.h"

#include "api/VKernel.h"		// ovrPoseStatef

#include "VFrame.h"
#include "App.h"
#include "GazeCursor.h"
#include "VRMenuMgr.h"
#include "VRMenuComponent.h"

namespace NervGear {

//==============================
// VRMenuEventHandler::VRMenuEventHandler
VRMenuEventHandler::VRMenuEventHandler()
{
}

//==============================
// VRMenuEventHandler::~VRMenuEventHandler
VRMenuEventHandler::~VRMenuEventHandler()
{
}

//==============================
// VRMenuEventHandler::Frame
void VRMenuEventHandler::frame( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, BitmapFont const & font,
        menuHandle_t const & rootHandle,VPosf const & menuPose, gazeCursorUserId_t const & gazeUserId,
		VArray< VRMenuEvent > & events )
{
	VRMenuObject * root = menuMgr.toObject( rootHandle );
	if ( root == NULL )
	{
		return;
	}

	// find the object the gaze is touching and update gaze focus
    const VR4Matrixf viewMatrix = app->lastViewMatrix();
    const V3Vectf viewPos( GetViewMatrixPosition( viewMatrix ) );
    const V3Vectf viewFwd( GetViewMatrixForward( viewMatrix ) );

	HitTestResult result;
	menuHandle_t hitHandle = root->hitTest( app, menuMgr, font, menuPose, viewPos, viewFwd, ContentFlags_t( CONTENT_SOLID ), result );
	result.RayStart = viewPos;
	result.RayDir = viewFwd;

	VRMenuObject * hit = hitHandle.isValid() ? menuMgr.toObject( hitHandle ) : NULL;
	app->gazeCursor().UpdateForUser( gazeUserId, result.t, ( hit != NULL && !( hit->flags() & VRMenuObjectFlags_t( VRMENUOBJECT_NO_GAZE_HILIGHT ) ) ) ? CURSOR_STATE_HILIGHT : CURSOR_STATE_NORMAL );
/*
    if ( hit != NULL )
    {
        app->ShowInfoText( 0.0f, "%s", hit->GetText().toCString() );
    }
*/
	bool focusChanged = ( hitHandle != m_focusedHandle );
	if ( focusChanged )
	{
		// focus changed
		VRMenuObject * oldFocus = menuMgr.toObject( m_focusedHandle );
		if ( oldFocus != NULL )
		{
			// setup event for item losing the focus
            VRMenuEvent event( VRMENU_EVENT_FOCUS_LOST, EVENT_DISPATCH_TARGET, m_focusedHandle, V3Vectf( 0.0f ), result );
			events.append( event );
		}
		if ( hit != NULL )
		{
			if ( ( hit->flags() & VRMENUOBJECT_FLAG_NO_FOCUS_GAINED ) == 0 )
			{
				// set up event for item gaining the focus
                VRMenuEvent event( VRMENU_EVENT_FOCUS_GAINED, EVENT_DISPATCH_FOCUS, hitHandle, V3Vectf( 0.0f ), result );
				events.append( event );
			}
		}
		m_focusedHandle = hitHandle;
	}

	bool touchPressed = ( vrFrame.input.buttonPressed & ( BUTTON_TOUCH | BUTTON_A ) ) != 0;
	bool touchReleased = !touchPressed && ( vrFrame.input.buttonReleased & ( BUTTON_TOUCH | BUTTON_A ) ) != 0;
    bool touchDown = ( vrFrame.input.buttonState & BUTTON_TOUCH ) != 0;

    if ( ( vrFrame.input.buttonPressed & BUTTON_SWIPE_UP ) != 0 )
    {
        VRMenuEvent event( VRMENU_EVENT_SWIPE_UP, EVENT_DISPATCH_FOCUS, m_focusedHandle, V3Vectf( 0.0f ), result );
		events.append( event );
    }
    if ( ( vrFrame.input.buttonPressed & BUTTON_SWIPE_DOWN ) != 0 )
    {
        VRMenuEvent event( VRMENU_EVENT_SWIPE_DOWN, EVENT_DISPATCH_FOCUS, m_focusedHandle, V3Vectf( 0.0f ), result );
		events.append( event );

    }
    if ( ( vrFrame.input.buttonPressed & BUTTON_SWIPE_FORWARD ) != 0 )
    {
        VRMenuEvent event( VRMENU_EVENT_SWIPE_FORWARD, EVENT_DISPATCH_FOCUS, m_focusedHandle, V3Vectf( 0.0f ), result );
		events.append( event );

    }
    if ( ( vrFrame.input.buttonPressed & BUTTON_SWIPE_BACK ) != 0 )
    {
        VRMenuEvent event( VRMENU_EVENT_SWIPE_BACK, EVENT_DISPATCH_FOCUS, m_focusedHandle, V3Vectf( 0.0f ), result );
		events.append( event );
    }

 /*
    // report swipe data
    char const * swipeNames[5] = { "none", "down", "up", "back", "forward" };
    int swipeUpDown = ( vrFrame.Input.buttonPressed & BUTTON_SWIPE_UP ) ? 2 : 0;
    swipeUpDown = ( vrFrame.Input.buttonPressed & BUTTON_SWIPE_DOWN ) ? 1 : swipeUpDown;
    int swipeForwardBack = ( vrFrame.Input.buttonPressed & BUTTON_SWIPE_FORWARD ) ? 4 : 0;
    swipeForwardBack = ( vrFrame.Input.buttonPressed & BUTTON_SWIPE_BACK ) ? 3 : swipeForwardBack;

    app->ShowInfoText( 1.0f, "touch %s\n( %s, %s )\n( %.2f, %.2f )\n( %.2f, %.2f )",
            touchDown ? swipeNames[1] : swipeNames[2],
            swipeNames[swipeUpDown], swipeNames[swipeForwardBack],
            vrFrame.Input.touch[0], vrFrame.Input.touch[1],
            vrFrame.Input.touchRelative[0], vrFrame.Input.touchRelative[1] );
*/
    // if nothing is focused, send events to the root
	if ( touchPressed )
	{
        VRMenuEvent event( VRMENU_EVENT_TOUCH_DOWN, EVENT_DISPATCH_FOCUS, m_focusedHandle, V3Vectf( 0.0f ), result );
		events.append( event );
	}
	if ( touchReleased )
	{
        VRMenuEvent event( VRMENU_EVENT_TOUCH_UP, EVENT_DISPATCH_FOCUS, m_focusedHandle, V3Vectf( vrFrame.input.touchRelative, 0.0f ), result );
		events.append( event );
	}
    if ( touchDown )
    {
        if ( vrFrame.input.touchRelative.LengthSq() > VConstantsf::SmallestNonDenormal )
        {
            VRMenuEvent event( VRMENU_EVENT_TOUCH_RELATIVE, EVENT_DISPATCH_FOCUS, m_focusedHandle, V3Vectf( vrFrame.input.touchRelative, 0.0f ), result );
            events.append( event );
        }
        VRMenuEvent event( VRMENU_EVENT_TOUCH_ABSOLUTE, EVENT_DISPATCH_FOCUS, m_focusedHandle, V3Vectf( vrFrame.input.touch, 0.0f ), result );
        events.append( event );
    }

    {
        // always post the frame event to the root
        VRMenuEvent event( VRMENU_EVENT_FRAME_UPDATE, EVENT_DISPATCH_BROADCAST, menuHandle_t(), V3Vectf( 0.0f ), result );
        events.append( event );
    }
}

//==============================
// VRMenuEventHandler::InitComponents
void VRMenuEventHandler::initComponents( VArray< VRMenuEvent > & events )
{
    VRMenuEvent event( VRMENU_EVENT_INIT, EVENT_DISPATCH_BROADCAST, menuHandle_t(), V3Vectf( 0.0f ), HitTestResult() );
	events.append( event );
}

//==============================
// VRMenuEventHandler::Opening
void VRMenuEventHandler::opening( VArray< VRMenuEvent > & events )
{
	vInfo("Opening");
	// broadcast the opening event
    VRMenuEvent event( VRMENU_EVENT_OPENING, EVENT_DISPATCH_BROADCAST, menuHandle_t(), V3Vectf( 0.0f ), HitTestResult() );
	events.append( event );
}

//==============================
// VRMenuEventHandler::Opened
void VRMenuEventHandler::opened( VArray< VRMenuEvent > & events )
{
	vInfo("Opened");
	// broadcast the opened event
    VRMenuEvent event( VRMENU_EVENT_OPENED, EVENT_DISPATCH_BROADCAST, menuHandle_t(), V3Vectf( 0.0f ), HitTestResult() );
	events.append( event );
}

//==============================
// VRMenuEventHandler::Closing
void VRMenuEventHandler::closing( VArray< VRMenuEvent > & events )
{
	vInfo("Closing");
	// broadcast the closing event
    VRMenuEvent event( VRMENU_EVENT_CLOSING, EVENT_DISPATCH_BROADCAST, menuHandle_t(), V3Vectf( 0.0f ), HitTestResult() );
	events.append( event );
}

//==============================
// VRMenuEventHandler::Closed
void VRMenuEventHandler::closed( VArray< VRMenuEvent > & events )
{
	vInfo("Closed");
	// broadcast the closed event
    VRMenuEvent event( VRMENU_EVENT_CLOSED, EVENT_DISPATCH_BROADCAST, menuHandle_t(), V3Vectf( 0.0f ), HitTestResult() );
	events.append( event );

	if ( m_focusedHandle.isValid() )
	{
        VRMenuEvent event( VRMENU_EVENT_FOCUS_LOST, EVENT_DISPATCH_TARGET, m_focusedHandle, V3Vectf( 0.0f ), HitTestResult() );
		events.append( event );
		m_focusedHandle.reset();
		vInfo("Released FocusHandle");
	}
}

//==============================
// LogEventType
static inline void LogEventType(const VRMenuEvent &event, const VString &message)
{
    if (event.eventType != VRMENU_EVENT_TOUCH_RELATIVE) {
        return;
    }

    vWarn("VrMenu:" << VRMenuEvent::EventTypeNames[event.eventType] << message);
}

//==============================
// FindTargetPath
static void FindTargetPath( OvrVRMenuMgr const & menuMgr,
        menuHandle_t const curHandle, VArray< menuHandle_t > & targetPath )
{
    VRMenuObject * obj = menuMgr.toObject( curHandle );
    if ( obj != NULL )
    {
        FindTargetPath( menuMgr, obj->parentHandle(), targetPath );
        targetPath.append( curHandle );
    }
}

//==============================
// FindTargetPath
static void FindTargetPath( OvrVRMenuMgr const & menuMgr, menuHandle_t const rootHandle,
        menuHandle_t const curHandle, VArray< menuHandle_t > & targetPath )
{
    FindTargetPath( menuMgr, curHandle, targetPath );
    if ( targetPath.length() == 0 )
    {
        targetPath.append( rootHandle );   // ensure at least root is in the path
    }
}

//==============================
// VRMenuEventHandler::HandleEvents
void VRMenuEventHandler::handleEvents( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
		menuHandle_t const rootHandle, VArray< VRMenuEvent > const & events ) const
{
    VRMenuObject * root = menuMgr.toObject( rootHandle );
    if ( root == NULL )
    {
        return;
    }

    // find the list of all objects that are in the focused path
    VArray< menuHandle_t > focusPath;
    FindTargetPath( menuMgr, rootHandle, m_focusedHandle, focusPath );

    VArray< menuHandle_t > targetPath;

	for ( int i = 0; i < events.length(); ++i )
	{
		VRMenuEvent const & event = events[i];
        switch ( event.dispatchType )
        {
            case EVENT_DISPATCH_BROADCAST:
                {
                    // broadcast to everything
		            broadcastEvent( app, vrFrame, menuMgr, event, root );
                }
                break;
            case EVENT_DISPATCH_FOCUS:
                // send to the focus path only -- this list should be parent -> child order
                dispatchToPath( app, vrFrame, menuMgr, event, focusPath, false );
                break;
            case EVENT_DISPATCH_TARGET:
                if ( targetPath.length() == 0 || event.targetHandle != targetPath.back() )
                {
                    targetPath.clear();
                    FindTargetPath( menuMgr, rootHandle, event.targetHandle, targetPath );
                }
                dispatchToPath( app, vrFrame, menuMgr, event, targetPath, false );
                break;
            default:
                vAssert( !"unknown dispatch type" );
                break;
        }
	}
}

//==============================
// VRMenuEventHandler::DispatchToComponents
bool VRMenuEventHandler::dispatchToComponents( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuEvent const & event, VRMenuObject * receiver ) const
{
    vAssert(receiver != NULL);

    VArray< VRMenuComponent* > const & list = receiver->componentList();
	int numComps = list.length();
	for ( int i = 0; i < numComps; ++i )
	{
		VRMenuComponent * item = list[i];
		if ( item->handlesEvent( VRMenuEventFlags_t( event.eventType ) ) )
		{
            LogEventType(event, "DispatchEvent: to '" + receiver->text() + "'");

            if (item->onEvent(app, vrFrame, menuMgr, receiver, event ) == MSG_STATUS_CONSUMED) {
                LogEventType(event, "DispatchEvent: receiver '" + receiver->text() + "', component " + VString::number(i) + " consumed event.");
                return true;    // consumed by component
            }
		}
	}
    return false;
}

//==============================
// VRMenuEventHandler::DispatchToPath
bool VRMenuEventHandler::dispatchToPath( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuEvent const & event, VArray< menuHandle_t > const & path, bool const log ) const
{
    // send to the focus path only -- this list should be parent -> child order
    for ( int i = 0; i < path.length(); ++i )
    {
        VRMenuObject * obj = menuMgr.toObject( path[i] );
		char const * const indent = "                                                                ";
        // set to
        if ( obj != NULL && dispatchToComponents( app, vrFrame, menuMgr, event, obj ) )
        {
			if ( log )
			{
                vInfo(&indent[64 - i * 2] << "DispatchToPath:" << VRMenuEvent::EventTypeNames[event.eventType] << ", object '" << VString( obj != NULL ? obj->text() : "<null>" ) << "' consumed event.");
			}
            return true;    // consumed by a component
        }
		if ( log )
		{
            vInfo(&indent[64 - i * 2] << "DispatchToPath:" << VRMenuEvent::EventTypeNames[event.eventType] << ", object '" << VString(obj != NULL ? obj->text() : "<null>") << "' passed event.");
		}
    }
    return false;
}

//==============================
// VRMenuEventHandler::BroadcastEvent
bool VRMenuEventHandler::broadcastEvent( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuEvent const & event, VRMenuObject * receiver ) const
{
    vAssert(receiver != NULL);

    // allow parent components to handle first
    if ( dispatchToComponents( app, vrFrame, menuMgr, event, receiver ) )
    {
        return true;
    }

    // if the parent did not consume, dispatch to children
    int numChildren = receiver->numChildren();
    for ( int i = 0; i < numChildren; ++i )
    {
        menuHandle_t childHandle = receiver->getChildHandleForIndex( i );
        VRMenuObject * child = menuMgr.toObject( childHandle );
        if ( child != NULL && broadcastEvent( app, vrFrame, menuMgr, event, child ) )
        {
            return true;    // consumed by child
        }
    }
    return false;
}

} // namespace NervGear
