#include "MovieSelectionComponent.h"
#include "VFrame.h"
#include "MovieSelectionView.h"

namespace OculusCinema {

//==============================
//  MovieSelectionComponent::
MovieSelectionComponent::MovieSelectionComponent( MovieSelectionView *view ) :
	VRMenuComponent( VRMenuEventFlags_t( VRMENU_EVENT_FRAME_UPDATE ) |
		VRMENU_EVENT_TOUCH_DOWN |
		VRMENU_EVENT_TOUCH_UP |
        VRMENU_EVENT_FOCUS_GAINED |
        VRMENU_EVENT_FOCUS_LOST ),
    CallbackView( view )

{
}

//==============================
//  MovieSelectionComponent::OnEvent_Impl
eMsgStatus MovieSelectionComponent::onEventImpl( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
    switch( event.eventType )
    {
		case VRMENU_EVENT_FRAME_UPDATE:
			return Frame( app, vrFrame, menuMgr, self, event );
        case VRMENU_EVENT_FOCUS_GAINED:
            return FocusGained( app, vrFrame, menuMgr, self, event );
        case VRMENU_EVENT_FOCUS_LOST:
            return FocusLost( app, vrFrame, menuMgr, self, event );
        case VRMENU_EVENT_TOUCH_DOWN:
            Sound.playSound( app, "touch_down", 0.1 );
       		return MSG_STATUS_CONSUMED;
        case VRMENU_EVENT_TOUCH_UP:
        	if ( !( vrFrame.input.buttonState & BUTTON_TOUCH_WAS_SWIPE ) )
			{
                Sound.playSound( app, "touch_up", 0.1 );
        		CallbackView->SelectMovie();
        		return MSG_STATUS_CONSUMED;
        	}
            return MSG_STATUS_ALIVE;
        default:
            vAssert( !"Event flags mismatch!" );
            return MSG_STATUS_ALIVE;
    }
}

//==============================
//  MovieSelectionComponent::Frame
eMsgStatus MovieSelectionComponent::Frame( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
    CallbackView->SelectionHighlighted( self->isHilighted() );

#if 0
	if ( self->IsHilighted() )
	{
		if ( vrFrame.Input.buttonPressed & BUTTON_A )
		{
			Sound.playSound( app, "touch_down", 0.1 );
		}
		if ( vrFrame.Input.buttonReleased & BUTTON_A )
		{
			Sound.playSound( app, "touch_up", 0.1 );
			CallbackView->SelectMovie();
		}
	}
#endif

    return MSG_STATUS_ALIVE;
}

//==============================
//  MovieSelectionComponent::FocusGained
eMsgStatus MovieSelectionComponent::FocusGained( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
	vInfo("FocusGained");
    // set the hilight flag
    self->setHilighted( true );
    CallbackView->SelectionHighlighted( true );

    Sound.playSound( app, "gaze_on", 0.1 );

    return MSG_STATUS_ALIVE;
}

//==============================
//  MovieSelectionComponent::FocusLost
eMsgStatus MovieSelectionComponent::FocusLost( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
	vInfo("FocusLost");
    // clear the hilight flag
    self->setHilighted( false );
    CallbackView->SelectionHighlighted( false );

    Sound.playSound( app, "gaze_off", 0.1 );

    return MSG_STATUS_ALIVE;
}

} // namespace OculusCinema
