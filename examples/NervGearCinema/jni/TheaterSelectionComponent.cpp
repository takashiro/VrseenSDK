
#include <Input.h>
#include <Vsync.h>

#include "TheaterSelectionComponent.h"
#include "TheaterSelectionView.h"

namespace OculusCinema {

//==============================
//  TheaterSelectionComponent::
TheaterSelectionComponent::TheaterSelectionComponent( TheaterSelectionView *view ) :
		CarouselItemComponent( VRMenuEventFlags_t( VRMENU_EVENT_TOUCH_DOWN ) |
            VRMENU_EVENT_TOUCH_UP |
            VRMENU_EVENT_FOCUS_GAINED |
            VRMENU_EVENT_FOCUS_LOST |
            VRMENU_EVENT_FRAME_UPDATE ),
	CallbackView( view ),
	HilightFader( 0.0f ),
	StartFadeInTime( 0.0 ),
	StartFadeOutTime( 0.0 ),
	HilightScale( 1.05f ),
	FadeDuration( 0.25f )
{
}

//==============================
//  TheaterSelectionComponent::SetItem
void TheaterSelectionComponent::SetItem( VRMenuObject * self, const CarouselItem * item, const PanelPose &pose )
{
    self->setLocalPosition( pose.Position );
    self->setLocalRotation( pose.Orientation );

	if ( item != NULL )
	{
        self->setText(item->name);
        self->setSurfaceTexture( 0, 0, SURFACE_TEXTURE_DIFFUSE,
			item->texture, item->textureWidth, item->textureHeight );
        self->setColor( pose.Color );
        self->setTextColor( pose.Color );
	}
	else
	{
        V4Vectf color( 0.0f );
        self->setColor( color );
        self->setTextColor( color );
	}
}

//==============================
//  TheaterSelectionComponent::OnEvent_Impl
eMsgStatus TheaterSelectionComponent::onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
    switch( event.eventType )
    {
        case VRMENU_EVENT_FOCUS_GAINED:
            return FocusGained( app, vrFrame, menuMgr, self, event );
        case VRMENU_EVENT_FOCUS_LOST:
            return FocusLost( app, vrFrame, menuMgr, self, event );
        case VRMENU_EVENT_FRAME_UPDATE:
            return Frame( app, vrFrame, menuMgr, self, event );
        case VRMENU_EVENT_TOUCH_DOWN:
        	if ( CallbackView != NULL )
        	{
                Sound.playSound( app, "touch_down", 0.1 );
        		return MSG_STATUS_CONSUMED;
        	}
        	return MSG_STATUS_ALIVE;
        case VRMENU_EVENT_TOUCH_UP:
        	if ( !( vrFrame.Input.buttonState & BUTTON_TOUCH_WAS_SWIPE ) && ( CallbackView != NULL ) )
        	{
                Sound.playSound( app, "touch_up", 0.1 );
                CallbackView->SelectPressed();
        		return MSG_STATUS_CONSUMED;
        	}
            return MSG_STATUS_ALIVE;
        default:
            OVR_ASSERT( !"Event flags mismatch!" );
            return MSG_STATUS_ALIVE;
    }
}

//==============================
//  TheaterSelectionComponent::FocusGained
eMsgStatus TheaterSelectionComponent::FocusGained( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
    // set the hilight flag
    self->setHilighted( true );

    StartFadeOutTime = -1.0;
    StartFadeInTime = ovr_GetTimeInSeconds();

    Sound.playSound( app, "gaze_on", 0.1 );

    return MSG_STATUS_ALIVE;
}

//==============================
//  TheaterSelectionComponent::FocusLost
eMsgStatus TheaterSelectionComponent::FocusLost( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
    // clear the hilight flag
    self->setHilighted( false );

    StartFadeInTime = -1.0;
    StartFadeOutTime = ovr_GetTimeInSeconds();

    Sound.playSound( app, "gaze_off", 0.1 );

    return MSG_STATUS_ALIVE;
}

//==============================
//  TheaterSelectionComponent::Frame
eMsgStatus TheaterSelectionComponent::Frame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
    double t = ovr_GetTimeInSeconds();
    if ( StartFadeInTime >= 0.0f && t >= StartFadeInTime )
    {
        HilightFader.startFadeIn();
        StartFadeInTime = -1.0f;
    }
    else if ( StartFadeOutTime >= 0.0f && t > StartFadeOutTime )
    {
        HilightFader.startFadeOut();
        StartFadeOutTime = -1.0f;
    }

    float const fadeRate = 1.0f / FadeDuration;
    HilightFader.update( fadeRate, vrFrame.DeltaSeconds );

    float const hilightAlpha = HilightFader.finalAlpha();

    float const scale = ( ( HilightScale - 1.0f ) * hilightAlpha ) + 1.0f;
    self->setHilightScale( scale );


#if 0
	if ( ( CallbackView != NULL ) && self->IsHilighted() && ( vrFrame.Input.buttonReleased & BUTTON_A ) )
	{
		if ( vrFrame.Input.buttonPressed & BUTTON_A )
		{
            Sound.playSound( app, "touch_down", 0.1 );
		}
		if ( vrFrame.Input.buttonReleased & BUTTON_A )
		{
            Sound.playSound( app, "touch_up", 0.1 );
			CallbackView->SelectPressed();
		}
	}
#endif

    return MSG_STATUS_ALIVE;
}

} // namespace OculusCinema
