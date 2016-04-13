

#include <Input.h>
#include "core/VTimer.h"

#include "SwipeHintComponent.h"
#include "CarouselBrowserComponent.h"

namespace OculusCinema {

const char * SwipeHintComponent::TYPE_NAME = "SwipeHintComponent";

bool SwipeHintComponent::ShowSwipeHints = true;

//==============================
//  SwipeHintComponent::SwipeHintComponent()
SwipeHintComponent::SwipeHintComponent( CarouselBrowserComponent *carousel, const bool isRightSwipe, const float totalTime, const float timeOffset, const float delay ) :
	VRMenuComponent( VRMenuEventFlags_t( VRMENU_EVENT_FRAME_UPDATE ) | VRMENU_EVENT_OPENING ),
	Carousel( carousel ),
	IsRightSwipe( isRightSwipe ),
    TotalTime( totalTime ),
    TimeOffset( timeOffset ),
    Delay( delay ),
    StartTime( 0 ),
    ShouldShow( false ),
    IgnoreDelay( false ),
    TotalAlpha()

{
}

//==============================
//  SwipeHintComponent::Reset
void SwipeHintComponent::Reset( VRMenuObject * self )
{
	IgnoreDelay = true;
	ShouldShow = false;
    const double now = VTimer::Seconds();
	TotalAlpha.Set( now, TotalAlpha.Value( now ), now, 0.0f );
    self->setColor( V4Vectf( 1.0f, 1.0f, 1.0f, 0.0f ) );
}

//==============================
//  SwipeHintComponent::CanSwipe
bool SwipeHintComponent::CanSwipe() const
{
	return IsRightSwipe ? Carousel->CanSwipeForward() : Carousel->CanSwipeBack();
}

//==============================
//  SwipeHintComponent::Show
void SwipeHintComponent::Show( const double now )
{
	if ( !ShouldShow )
	{
		ShouldShow = true;
		StartTime = now + TimeOffset + ( IgnoreDelay ? 0.0f : Delay );
		TotalAlpha.Set( now, TotalAlpha.Value( now ), now + 0.5f, 1.0f );
	}
}

//==============================
//  SwipeHintComponent::Hide
void SwipeHintComponent::Hide( const double now )
{
	if ( ShouldShow )
	{
		TotalAlpha.Set( now, TotalAlpha.Value( now ), now + 0.5f, 0.0f );
		ShouldShow = false;
	}
}

//==============================
//  SwipeHintComponent::OnEvent_Impl
eMsgStatus SwipeHintComponent::onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
    switch( event.eventType )
    {
    	case VRMENU_EVENT_OPENING :
    		return Opening( app, vrFrame, menuMgr, self, event );
        case VRMENU_EVENT_FRAME_UPDATE :
        	return Frame( app, vrFrame, menuMgr, self, event );
        default:
            vAssert( !"Event flags mismatch!" );
            return MSG_STATUS_ALIVE;
    }
}

//==============================
//  SwipeHintComponent::Opening
eMsgStatus SwipeHintComponent::Opening( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event )
{
	Reset( self );
	return MSG_STATUS_ALIVE;
}

//==============================
//  SwipeHintComponent::Frame
eMsgStatus SwipeHintComponent::Frame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event )
{
	if ( ShowSwipeHints && Carousel->HasSelection() && CanSwipe() )
	{
        Show( vrFrame.pose.timestamp );
	}
	else
	{
        Hide( vrFrame.pose.timestamp );
	}

	IgnoreDelay = false;

    float alpha = TotalAlpha.Value( vrFrame.pose.timestamp );
	if ( alpha > 0.0f )
	{
        double time = vrFrame.pose.timestamp - StartTime;
		if ( time < 0.0f )
		{
			alpha = 0.0f;
		}
		else
		{
			float normTime = time / TotalTime;
			alpha *= sin( M_PI * 2.0f * normTime );
			alpha = std::max( alpha, 0.0f );
		}
	}

    self->setColor( V4Vectf( 1.0f, 1.0f, 1.0f, alpha ) );

	return MSG_STATUS_ALIVE;
}

} // namespace OculusCinema
