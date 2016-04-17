#include "MovieCategoryComponent.h"
#include "CinemaApp.h"


namespace OculusCinema {

const V4Vectf MovieCategoryComponent::FocusColor( 1.0f, 1.0f, 1.0f, 1.0f );
const V4Vectf MovieCategoryComponent::HighlightColor( 1.0f, 1.0f, 1.0f, 1.0f );
const V4Vectf MovieCategoryComponent::NormalColor( 82.0f / 255.0f, 101.0f / 255.0f, 120.0f / 255.06, 255.0f / 255.0f );

//==============================
//  MovieCategoryComponent::
MovieCategoryComponent::MovieCategoryComponent( MovieSelectionView * view, MovieCategory category ) :
    VRMenuComponent( VRMenuEventFlags_t( VRMENU_EVENT_TOUCH_DOWN ) |
            VRMENU_EVENT_TOUCH_UP |
            VRMENU_EVENT_FOCUS_GAINED |
            VRMENU_EVENT_FOCUS_LOST |
            VRMENU_EVENT_FRAME_UPDATE ),
    Sound(),
	HasFocus( false ),
	Category( category ),
    CallbackView( view )
{
}

//==============================
//  MovieCategoryComponent::UpdateColor
void MovieCategoryComponent::UpdateColor( VRMenuObject * self )
{
    self->setTextColor( HasFocus ? FocusColor : ( self->isHilighted() ? HighlightColor : NormalColor ) );
    self->setColor( self->isHilighted() ? V4Vectf( 1.0f ) : V4Vectf( 0.0f ) );
}

//==============================
//  MovieCategoryComponent::OnEvent_Impl
eMsgStatus MovieCategoryComponent::onEventImpl( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
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
        	if ( CallbackView != NULL )
        	{
                Sound.playSound( app, "touch_down", 0.1 );
        		return MSG_STATUS_CONSUMED;
        	}
        	return MSG_STATUS_ALIVE;
        case VRMENU_EVENT_TOUCH_UP:
        	if ( !( vrFrame.input.buttonState & BUTTON_TOUCH_WAS_SWIPE ) && ( CallbackView != NULL ) )
        	{
                Sound.playSound( app, "touch_up", 0.1 );
               	CallbackView->SetCategory( Category );
        		return MSG_STATUS_CONSUMED;
        	}
            return MSG_STATUS_ALIVE;
        default:
            vAssert( !"Event flags mismatch!" );
            return MSG_STATUS_ALIVE;
    }
}

//==============================
//  MovieCategoryComponent::Frame
eMsgStatus MovieCategoryComponent::Frame( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
	UpdateColor( self );

    return MSG_STATUS_ALIVE;
}

//==============================
//  MovieCategoryComponent::FocusGained
eMsgStatus MovieCategoryComponent::FocusGained( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
	//vInfo("FocusGained");
	HasFocus = true;
    Sound.playSound( app, "gaze_on", 0.1 );

    self->setTextColor( HighlightColor );

	return MSG_STATUS_ALIVE;
}

//==============================
//  MovieCategoryComponent::FocusLost
eMsgStatus MovieCategoryComponent::FocusLost( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
	//vInfo("FocusLost");

	HasFocus = false;
    Sound.playSound( app, "gaze_off", 0.1 );

    self->setTextColor( self->isHilighted() ? HighlightColor : NormalColor );

	return MSG_STATUS_ALIVE;
}

} // namespace OculusCinema
