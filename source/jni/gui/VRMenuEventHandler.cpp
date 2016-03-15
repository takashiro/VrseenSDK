/************************************************************************************

Filename    :   VRMenuEventHandler.cpp
Content     :   Menu component for handling hit tests and dispatching events.
Created     :   June 23, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "VRMenuEventHandler.h"

#include "Android/LogUtils.h"

#include "api/VrApi.h"		// ovrPoseStatef

#include "Input.h"
#include "VrCommon.h"
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
void VRMenuEventHandler::frame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, BitmapFont const & font, 
        menuHandle_t const & rootHandle,Posef const & menuPose, gazeCursorUserId_t const & gazeUserId, 
		Array< VRMenuEvent > & events )
{
	VRMenuObject * root = menuMgr.toObject( rootHandle );
	if ( root == NULL )
	{
		return;
	}

	// find the object the gaze is touching and update gaze focus
	const Matrix4f viewMatrix = app->lastViewMatrix();
	const Vector3f viewPos( GetViewMatrixPosition( viewMatrix ) );
	const Vector3f viewFwd( GetViewMatrixForward( viewMatrix ) );

	HitTestResult result;
	menuHandle_t hitHandle = root->hitTest( app, menuMgr, font, menuPose, viewPos, viewFwd, ContentFlags_t( CONTENT_SOLID ), result );
	result.RayStart = viewPos;
	result.RayDir = viewFwd;

	VRMenuObject * hit = hitHandle.IsValid() ? menuMgr.toObject( hitHandle ) : NULL;
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
			VRMenuEvent event( VRMENU_EVENT_FOCUS_LOST, EVENT_DISPATCH_TARGET, m_focusedHandle, Vector3f( 0.0f ), result );
			events.append( event );
		}
		if ( hit != NULL )
		{
			if ( ( hit->flags() & VRMENUOBJECT_FLAG_NO_FOCUS_GAINED ) == 0 )
			{
				// set up event for item gaining the focus
				VRMenuEvent event( VRMENU_EVENT_FOCUS_GAINED, EVENT_DISPATCH_FOCUS, hitHandle, Vector3f( 0.0f ), result );
				events.append( event );
			}
		}
		m_focusedHandle = hitHandle;
	}

	bool touchPressed = ( vrFrame.Input.buttonPressed & ( BUTTON_TOUCH | BUTTON_A ) ) != 0;
	bool touchReleased = !touchPressed && ( vrFrame.Input.buttonReleased & ( BUTTON_TOUCH | BUTTON_A ) ) != 0;
    bool touchDown = ( vrFrame.Input.buttonState & BUTTON_TOUCH ) != 0;

    if ( ( vrFrame.Input.buttonPressed & BUTTON_SWIPE_UP ) != 0 )
    {
		VRMenuEvent event( VRMENU_EVENT_SWIPE_UP, EVENT_DISPATCH_FOCUS, m_focusedHandle, Vector3f( 0.0f ), result );
		events.append( event );
    }
    if ( ( vrFrame.Input.buttonPressed & BUTTON_SWIPE_DOWN ) != 0 )
    {
		VRMenuEvent event( VRMENU_EVENT_SWIPE_DOWN, EVENT_DISPATCH_FOCUS, m_focusedHandle, Vector3f( 0.0f ), result );
		events.append( event );

    }
    if ( ( vrFrame.Input.buttonPressed & BUTTON_SWIPE_FORWARD ) != 0 )
    {
		VRMenuEvent event( VRMENU_EVENT_SWIPE_FORWARD, EVENT_DISPATCH_FOCUS, m_focusedHandle, Vector3f( 0.0f ), result );
		events.append( event );

    }
    if ( ( vrFrame.Input.buttonPressed & BUTTON_SWIPE_BACK ) != 0 )
    {
		VRMenuEvent event( VRMENU_EVENT_SWIPE_BACK, EVENT_DISPATCH_FOCUS, m_focusedHandle, Vector3f( 0.0f ), result );
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
		VRMenuEvent event( VRMENU_EVENT_TOUCH_DOWN, EVENT_DISPATCH_FOCUS, m_focusedHandle, Vector3f( 0.0f ), result );
		events.append( event );
	}
	if ( touchReleased )
	{
		VRMenuEvent event( VRMENU_EVENT_TOUCH_UP, EVENT_DISPATCH_FOCUS, m_focusedHandle, Vector3f( vrFrame.Input.touchRelative, 0.0f ), result );
		events.append( event );
	}
    if ( touchDown )
    {
        if ( vrFrame.Input.touchRelative.LengthSq() > Mathf::SmallestNonDenormal )
        {
            VRMenuEvent event( VRMENU_EVENT_TOUCH_RELATIVE, EVENT_DISPATCH_FOCUS, m_focusedHandle, Vector3f( vrFrame.Input.touchRelative, 0.0f ), result );
            events.append( event );
        }
        VRMenuEvent event( VRMENU_EVENT_TOUCH_ABSOLUTE, EVENT_DISPATCH_FOCUS, m_focusedHandle, Vector3f( vrFrame.Input.touch, 0.0f ), result );
        events.append( event );
    }

    {
        // always post the frame event to the root
        VRMenuEvent event( VRMENU_EVENT_FRAME_UPDATE, EVENT_DISPATCH_BROADCAST, menuHandle_t(), Vector3f( 0.0f ), result );
        events.append( event );
    }
}

//==============================
// VRMenuEventHandler::InitComponents
void VRMenuEventHandler::initComponents( Array< VRMenuEvent > & events )
{
	VRMenuEvent event( VRMENU_EVENT_INIT, EVENT_DISPATCH_BROADCAST, menuHandle_t(), Vector3f( 0.0f ), HitTestResult() );
	events.append( event );
}

//==============================
// VRMenuEventHandler::Opening
void VRMenuEventHandler::opening( Array< VRMenuEvent > & events )
{
	LOG( "Opening" );
	// broadcast the opening event
	VRMenuEvent event( VRMENU_EVENT_OPENING, EVENT_DISPATCH_BROADCAST, menuHandle_t(), Vector3f( 0.0f ), HitTestResult() );
	events.append( event );
}

//==============================
// VRMenuEventHandler::Opened
void VRMenuEventHandler::opened( Array< VRMenuEvent > & events )
{
	LOG( "Opened" );
	// broadcast the opened event
	VRMenuEvent event( VRMENU_EVENT_OPENED, EVENT_DISPATCH_BROADCAST, menuHandle_t(), Vector3f( 0.0f ), HitTestResult() );
	events.append( event );
}

//==============================
// VRMenuEventHandler::Closing
void VRMenuEventHandler::closing( Array< VRMenuEvent > & events )
{
	LOG( "Closing" );
	// broadcast the closing event
	VRMenuEvent event( VRMENU_EVENT_CLOSING, EVENT_DISPATCH_BROADCAST, menuHandle_t(), Vector3f( 0.0f ), HitTestResult() );
	events.append( event );
}

//==============================
// VRMenuEventHandler::Closed
void VRMenuEventHandler::closed( Array< VRMenuEvent > & events )
{
	LOG( "Closed" );
	// broadcast the closed event
	VRMenuEvent event( VRMENU_EVENT_CLOSED, EVENT_DISPATCH_BROADCAST, menuHandle_t(), Vector3f( 0.0f ), HitTestResult() );
	events.append( event );

	if ( m_focusedHandle.IsValid() )
	{
		VRMenuEvent event( VRMENU_EVENT_FOCUS_LOST, EVENT_DISPATCH_TARGET, m_focusedHandle, Vector3f( 0.0f ), HitTestResult() );
		events.append( event );
		m_focusedHandle.Release();
		LOG( "Released FocusHandle" );
	}
}

//==============================
// LogEventType
static inline void LogEventType( VRMenuEvent const & event, char const * fmt, ... )
{
#if 1
    if ( event.eventType != VRMENU_EVENT_TOUCH_RELATIVE )
    {
        return;
    }

    char fmtBuff[256];
    va_list args;
    va_start( args, fmt );
    vsnprintf( fmtBuff, sizeof( fmtBuff ), fmt, args );
    va_end( args );

    char buffer[512];
    OVR_sprintf( buffer, sizeof( buffer ), "%s: %s", VRMenuEvent::EventTypeNames[event.eventType], fmtBuff );

    __android_log_write( ANDROID_LOG_WARN, "VrMenu", buffer );
#endif
}

//==============================
// FindTargetPath
static void FindTargetPath( OvrVRMenuMgr const & menuMgr, 
        menuHandle_t const curHandle, Array< menuHandle_t > & targetPath ) 
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
        menuHandle_t const curHandle, Array< menuHandle_t > & targetPath ) 
{
    FindTargetPath( menuMgr, curHandle, targetPath );
    if ( targetPath.length() == 0 )
    {
        targetPath.append( rootHandle );   // ensure at least root is in the path
    }
}

//==============================
// VRMenuEventHandler::HandleEvents
void VRMenuEventHandler::handleEvents( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, 
		menuHandle_t const rootHandle, Array< VRMenuEvent > const & events ) const
{
    VRMenuObject * root = menuMgr.toObject( rootHandle );
    if ( root == NULL )
    {
        return;
    }

    // find the list of all objects that are in the focused path
    Array< menuHandle_t > focusPath;
    FindTargetPath( menuMgr, rootHandle, m_focusedHandle, focusPath );
    
    Array< menuHandle_t > targetPath;

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
                OVR_ASSERT( !"unknown dispatch type" );
                break;
        }
	}
}

//==============================
// VRMenuEventHandler::DispatchToComponents
bool VRMenuEventHandler::dispatchToComponents( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuEvent const & event, VRMenuObject * receiver ) const
{
	DROID_ASSERT( receiver != NULL, "VrMenu" );

	Array< VRMenuComponent* > const & list = receiver->componentList();
	int numComps = list.length();
	for ( int i = 0; i < numComps; ++i )
	{
		VRMenuComponent * item = list[i];
		if ( item->handlesEvent( VRMenuEventFlags_t( event.eventType ) ) )
		{
            LogEventType( event, "DispatchEvent: to '%s'", receiver->text().toCString() );

			if ( item->onEvent( app, vrFrame, menuMgr, receiver, event ) == MSG_STATUS_CONSUMED )
            {
                LogEventType( event, "DispatchEvent: receiver '%s', component %i consumed event.", receiver->text().toCString(), i );
                return true;    // consumed by component
            }
		}
	}
    return false;
}

//==============================
// VRMenuEventHandler::DispatchToPath
bool VRMenuEventHandler::dispatchToPath( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuEvent const & event, Array< menuHandle_t > const & path, bool const log ) const
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
				LOG( "%sDispatchToPath: %s, object '%s' consumed event.", &indent[64 - i * 2],
						VRMenuEvent::EventTypeNames[event.eventType], ( obj != NULL ? obj->text().toCString() : "<null>" ) );
			}	
            return true;    // consumed by a component
        }
		if ( log )
		{
			LOG( "%sDispatchToPath: %s, object '%s' passed event.", &indent[64 - i * 2],
					VRMenuEvent::EventTypeNames[event.eventType], obj != NULL ? obj->text().toCString() : "<null>" );
		}
    }
    return false;
}

//==============================
// VRMenuEventHandler::BroadcastEvent
bool VRMenuEventHandler::broadcastEvent( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, 
        VRMenuEvent const & event, VRMenuObject * receiver ) const
{
	DROID_ASSERT( receiver != NULL, "VrMenu" );

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
