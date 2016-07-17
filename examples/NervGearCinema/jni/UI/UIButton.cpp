#include <VRMenuMgr.h>

#include "UI/UIButton.h"
#include "UI/UIMenu.h"
#include "CinemaApp.h"

namespace OculusCinema {

UIButton::UIButton( CinemaApp &cinema ) :
	UIWidget( cinema ),
	ButtonComponent( *this ),
	Normal(),
	Hover(),
	Pressed(),
	OnClickFunction( NULL ),
	OnClickObject( NULL )

{
}

UIButton::~UIButton()
{
}

void UIButton::AddToMenu( UIMenu *menu, UIWidget *parent )
{
    const VPosf pose( VQuatf( V3Vectf( 0.0f, 1.0f, 0.0f ), 0.0f ), V3Vectf( 0.0f, 0.0f, 0.0f ) );

    V3Vectf defaultScale( 1.0f );
	VRMenuFontParms fontParms( true, true, false, false, false, 1.0f );

	VRMenuObjectParms parms( VRMENU_BUTTON, VArray< VRMenuComponent* >(), VRMenuSurfaceParms(),
			"", pose, defaultScale, fontParms, menu->AllocId(),
			VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

	AddToMenuWithParms( menu, parent, parms );

	VRMenuObject * object = GetMenuObject();
	vAssert( object );
    object->addComponent( &ButtonComponent );
}

void UIButton::SetButtonImages( const UITexture &normal, const UITexture &hover, const UITexture &pressed )
{
	Normal 	= normal;
	Hover 	= hover;
	Pressed = pressed;

	UpdateButtonState();
}

void UIButton::SetOnClick( void ( *callback )( UIButton *, void * ), void *object )
{
	OnClickFunction = callback;
	OnClickObject = object;
}

void UIButton::OnClick()
{
	if ( OnClickFunction != NULL )
	{
		( *OnClickFunction )( this, OnClickObject );
	}
}

//==============================
//  UIButton::UpdateButtonState
void UIButton::UpdateButtonState()
{
	if ( ButtonComponent.IsPressed() )
	{
		SetImage( 0, SURFACE_TEXTURE_DIFFUSE, Pressed );
	}
    else if ( GetMenuObject()->isHilighted() )
	{
		SetImage( 0, SURFACE_TEXTURE_DIFFUSE, Hover );
	}
	else
	{
		SetImage( 0, SURFACE_TEXTURE_DIFFUSE, Normal );
	}
}

/*************************************************************************************/

//==============================
//  UIButtonComponent::
UIButtonComponent::UIButtonComponent( UIButton &button ) :
    VRMenuComponent( VRMenuEventFlags_t( VRMENU_EVENT_TOUCH_DOWN ) |
            VRMENU_EVENT_TOUCH_UP |
            VRMENU_EVENT_FOCUS_GAINED |
            VRMENU_EVENT_FOCUS_LOST ),
    Button( button ),
	TouchDown( false )

{
}

//==============================
//  UIButtonComponent::OnEvent_Impl
eMsgStatus UIButtonComponent::onEventImpl( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
    switch( event.eventType )
    {
        case VRMENU_EVENT_FOCUS_GAINED:
            return FocusGained( app, vrFrame, menuMgr, self, event );
        case VRMENU_EVENT_FOCUS_LOST:
            return FocusLost( app, vrFrame, menuMgr, self, event );
        case VRMENU_EVENT_TOUCH_DOWN:
        	TouchDown = true;
        	Button.UpdateButtonState();
            DownSoundLimiter.playSound( app, "touch_down", 0.1 );
            return MSG_STATUS_ALIVE;
        case VRMENU_EVENT_TOUCH_UP:
        	TouchDown = false;
        	Button.UpdateButtonState();
        	Button.OnClick();
            UpSoundLimiter.playSound( app, "touch_up", 0.1 );
            return MSG_STATUS_ALIVE;
        default:
            vAssert( !"Event flags mismatch!" );
            return MSG_STATUS_ALIVE;
    }
}

//==============================
//  UIButtonComponent::FocusGained
eMsgStatus UIButtonComponent::FocusGained( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
    // set the hilight flag
    self->setHilighted( true );
    Button.UpdateButtonState();
    GazeOverSoundLimiter.playSound( app, "gaze_on", 0.1 );
    return MSG_STATUS_ALIVE;
}

//==============================
//  UIButtonComponent::FocusLost
eMsgStatus UIButtonComponent::FocusLost( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
    // clear the hilight flag
    self->setHilighted( false );
    TouchDown = false;
    Button.UpdateButtonState();
    GazeOverSoundLimiter.playSound( app, "gaze_off", 0.1 );
    return MSG_STATUS_ALIVE;
}

} // namespace OculusCinema
