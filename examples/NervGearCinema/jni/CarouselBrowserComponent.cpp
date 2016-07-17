#include "CarouselBrowserComponent.h"
#include "App.h"
#include "core/VTimer.h"

using namespace NervGear;

namespace OculusCinema {

//==============================================================
// CarouselBrowserComponent
CarouselBrowserComponent::CarouselBrowserComponent( const VArray<CarouselItem *> &items, const VArray<PanelPose> &panelPoses ) :
	VRMenuComponent( VRMenuEventFlags_t( VRMENU_EVENT_FRAME_UPDATE ) | 	VRMENU_EVENT_TOUCH_DOWN |
		VRMENU_EVENT_SWIPE_FORWARD | VRMENU_EVENT_SWIPE_BACK | VRMENU_EVENT_TOUCH_UP | VRMENU_EVENT_OPENED | VRMENU_EVENT_CLOSED ),
		SelectPressed( false ), PositionScale( 1.0f ), Position( 0.0f ), TouchDownTime( -1.0 ),
		ItemWidth( 0 ), ItemHeight( 0 ), Items(), MenuObjs(), MenuComps(), PanelPoses( panelPoses ),
		StartTime( 0.0 ), EndTime( 0.0 ), PrevPosition( 0.0f ), NextPosition( 0.0f ), Swiping( false ), PanelsNeedUpdate( false )

{
	SetItems( items );
}

eMsgStatus CarouselBrowserComponent::onEventImpl( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
	VRMenuObject * self, VRMenuEvent const & event )
{
	vAssert( handlesEvent( VRMenuEventFlags_t( event.eventType ) ) );

    switch( event.eventType )
	{
		case VRMENU_EVENT_FRAME_UPDATE:
			return Frame( app, vrFrame, menuMgr, self, event );
		case VRMENU_EVENT_TOUCH_DOWN:
			return TouchDown( app, vrFrame, menuMgr, self, event);
		case VRMENU_EVENT_TOUCH_UP:
			return TouchUp( app, vrFrame, menuMgr, self, event );
		case VRMENU_EVENT_OPENED:
			return Opened( app, vrFrame, menuMgr, self, event );
		case VRMENU_EVENT_CLOSED:
			return Closed( app, vrFrame, menuMgr, self, event );
		case VRMENU_EVENT_SWIPE_FORWARD:
			return SwipeForward( app, vrFrame, menuMgr, self );
		case VRMENU_EVENT_SWIPE_BACK:
			return SwipeBack( app, vrFrame, menuMgr, self );
		default:
			vAssert( !"Event flags mismatch!" ); // the constructor is specifying a flag that's not handled
			return MSG_STATUS_ALIVE;
	}
}

void CarouselBrowserComponent::SetPanelPoses( OvrVRMenuMgr & menuMgr, VRMenuObject * self, const VArray<PanelPose> &panelPoses )
{
	PanelPoses = panelPoses;
	UpdatePanels( menuMgr, self );
}

void CarouselBrowserComponent::SetMenuObjects( const VArray<VRMenuObject *> &menuObjs, const VArray<CarouselItemComponent *> &menuComps )
{
	MenuObjs = menuObjs;
	MenuComps = menuComps;

    assert( MenuObjs.length() == MenuObjs.length() );
}

PanelPose CarouselBrowserComponent::GetPosition( const float t )
{
	int index = ( int )floor( t );
	float frac = t - ( float )index;

	PanelPose pose;

	if ( index < 0 )
	{
		pose = PanelPoses[ 0 ];
	}
    else if ( ( index == PanelPoses.length() - 1 ) && ( fabs( frac ) <= 0.00001f ) )
	{
        pose = PanelPoses[ PanelPoses.length() - 1 ];
	}
    else if ( index >= PanelPoses.length() - 1 )
	{
        pose.Orientation = VQuatf();
        pose.Position = V3Vectf( 0.0f, 0.0f, 0.0f );
        pose.Color = V4Vectf( 0.0f, 0.0f, 0.0f, 0.0f );
	}
	else
	{
		pose.Orientation = PanelPoses[ index + 1 ].Orientation.Nlerp( PanelPoses[ index ].Orientation, frac ); // NLerp has the frac inverted
		pose.Position = PanelPoses[ index ].Position.Lerp( PanelPoses[ index + 1 ].Position, frac );
		pose.Color = PanelPoses[ index ].Color * ( 1.0f - frac ) + PanelPoses[ index + 1 ].Color * frac;
	}

	pose.Position = pose.Position * PositionScale;

	return pose;
}

void CarouselBrowserComponent::SetSelectionIndex( const int selectedIndex )
{
    if ( ( selectedIndex >= 0 ) && ( selectedIndex < Items.length() ) )
	{
		Position = selectedIndex;
	}
	else
	{
		Position = 0.0f;
	}

	NextPosition = Position;
	Swiping = false;
	PanelsNeedUpdate = true;
}

int CarouselBrowserComponent::GetSelection() const
{
	int itemIndex = floor( Position + 0.5f );
    if ( ( itemIndex >= 0 ) && ( itemIndex < Items.length() ) )
	{
		return itemIndex;
	}

	return -1;
}

bool CarouselBrowserComponent::HasSelection() const
{
    if ( Items.size() == 0 )
	{
		return false;
	}

	return !Swiping;
}

bool CarouselBrowserComponent::CanSwipeBack() const
{
	float nextPos = floor( Position ) - 1.0f;
	return ( nextPos >= 0.0f );
}

bool CarouselBrowserComponent::CanSwipeForward() const
{
	float nextPos = floor( Position ) + 1.0f;
    return ( nextPos < Items.length() );
}

void CarouselBrowserComponent::UpdatePanels( OvrVRMenuMgr & menuMgr, VRMenuObject * self )
{
	int centerIndex = floor( Position );
	float offset = centerIndex - Position;
    int leftIndex = centerIndex - PanelPoses.length() / 2;

	int itemIndex = leftIndex;
    for( int i = 0; i < MenuObjs.length(); i++, itemIndex++ )
	{
		PanelPose pose = GetPosition( ( float )i + offset );
        if ( ( itemIndex < 0 ) || ( itemIndex >= Items.length() ) || ( ( offset < 0.0f ) && ( i == 0 ) ) )
		{
			MenuComps[ i ]->SetItem( MenuObjs[ i ], NULL, pose );
		}
		else
		{
			MenuComps[ i ]->SetItem( MenuObjs[ i ], Items[ itemIndex ], pose );
		}
	}

	PanelsNeedUpdate = false;
}

void CarouselBrowserComponent::CheckGamepad( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self )
{
	if ( Swiping )
	{
		return;
	}

	if ( CanSwipeBack() && ( ( vrFrame.input.buttonState & BUTTON_DPAD_LEFT ) || ( vrFrame.input.sticks[0][0] < -0.5f ) ) )
	{
		SwipeBack( app, vrFrame, menuMgr, self );
		return;
	}

	if ( CanSwipeForward() && ( ( vrFrame.input.buttonState & BUTTON_DPAD_RIGHT ) || ( vrFrame.input.sticks[0][0] > 0.5f ) ) )
	{
		SwipeForward( app, vrFrame, menuMgr, self );
		return;
	}
}

eMsgStatus CarouselBrowserComponent::Frame( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event )
{
	if ( Swiping )
	{
        float frac = ( vrFrame.pose.timestamp - StartTime ) / ( EndTime - StartTime );
		if ( frac >= 1.0f )
		{
			frac = 1.0f;
			Swiping = false;
		}

		float easeOutQuad = -1.0f * frac * ( frac - 2.0f );

		Position = PrevPosition * ( 1.0f - easeOutQuad ) + NextPosition * easeOutQuad;

		PanelsNeedUpdate = true;
	}

	if ( PanelsNeedUpdate )
	{
		UpdatePanels( menuMgr, self );
	}

	return MSG_STATUS_ALIVE;
}

eMsgStatus CarouselBrowserComponent::SwipeForward( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self )
{
	if ( !Swiping )
	{
		float nextPos = floor( Position ) + 1.0f;
        if ( nextPos < Items.length() )
		{
			app->playSound( "carousel_move" );
			PrevPosition = Position;
            StartTime = vrFrame.pose.timestamp;
			EndTime = StartTime + 0.25;
			NextPosition = nextPos;
			Swiping = true;
		}
	}

	return MSG_STATUS_CONSUMED;
}

eMsgStatus CarouselBrowserComponent::SwipeBack( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self )
{
	if ( !Swiping )
	{
		float nextPos = floor( Position ) - 1.0f;
		if ( nextPos >= 0.0f )
		{
			app->playSound( "carousel_move" );
			PrevPosition = Position;
            StartTime = vrFrame.pose.timestamp;
			EndTime = StartTime + 0.25;
			NextPosition = nextPos;
			Swiping = true;
		}
	}

	return MSG_STATUS_CONSUMED;
}

eMsgStatus CarouselBrowserComponent::TouchDown( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event )
{
	//vInfo("TouchDown");
    TouchDownTime = VTimer::Seconds();

	if ( Swiping )
	{
		return MSG_STATUS_CONSUMED;
	}

	return MSG_STATUS_ALIVE;	// don't consume -- we're just listening
}

eMsgStatus CarouselBrowserComponent::TouchUp( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event )
{
	//vInfo("TouchUp");

    float const timeTouchHasBeenDown = (float)( VTimer::Seconds() - TouchDownTime );
	TouchDownTime = -1.0;

    float dist = event.floatValue.LengthSq();
	if ( !Swiping && ( dist < 20.0f ) && ( timeTouchHasBeenDown < 1.0f ) )
	{
		vInfo("Selectmovie");
		SelectPressed = true;
	}
	else if ( Swiping )
	{
		return MSG_STATUS_CONSUMED;
	}

	//vInfo("Ignore:" << RotationalVelocity << "," << ( float )timeTouchHasBeenDown);
	return MSG_STATUS_ALIVE; // don't consume -- we are just listening
}

eMsgStatus CarouselBrowserComponent::Opened( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event )
{
	Swiping = false;
	Position = floor( Position );
	SelectPressed = false;
	return MSG_STATUS_ALIVE;
}

eMsgStatus CarouselBrowserComponent::Closed( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event )
{
	SelectPressed = false;
	return MSG_STATUS_ALIVE;
}

void CarouselBrowserComponent::SetItems( const VArray<CarouselItem *> &items )
{
	Items = items;
	SelectPressed = false;
	Position = 0.0f;
	TouchDownTime = -1.0;
	StartTime = 0.0;
	EndTime = 0.0;
	PrevPosition = 0.0f;
	NextPosition = 0.0f;
	PanelsNeedUpdate = true;
}

} // namespace OculusCinema
