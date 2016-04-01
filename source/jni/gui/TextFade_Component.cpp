/************************************************************************************

Filename    :   OvrTextFade_Component.cpp
Content     :   A reusable component that fades text in and recenters it on gaze over.
Created     :   July 25, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#include "TextFade_Component.h"


#include "Input.h"
#include "BitmapFont.h"
#include "VRMenuMgr.h"

namespace NervGear {

double const OvrTextFade_Component::FADE_DELAY = 0.05;
float const  OvrTextFade_Component::FADE_DURATION = 0.25f;


//==============================
// OvrTextFade_Component::CalcIconFadeOffset
V3Vectf OvrTextFade_Component::CalcIconFadeOffset(const VString &text, BitmapFont const & font, V3Vectf const & axis, float const iconWidth )
{
    float textWidth = font.CalcTextWidth(text);
    float const fullWidth = textWidth + iconWidth;
    return axis * ( ( fullWidth * 0.5f ) - ( iconWidth * 0.5f ) );  // this is a bit odd, but that's because the icon's origin is its center
}

//==============================
// OvrTextFade_Component::OvrTextFade_Component
OvrTextFade_Component::OvrTextFade_Component( V3Vectf const & iconBaseOffset, V3Vectf const & iconFadeOffset ) :
	VRMenuComponent( VRMenuEventFlags_t( VRMENU_EVENT_FRAME_UPDATE ) | VRMENU_EVENT_FOCUS_GAINED | VRMENU_EVENT_FOCUS_LOST ),
	m_textAlphaFader( 0.0f ),
	m_startFadeInTime( -1.0 ),
	m_startFadeOutTime( -1.0 ),
	m_iconOffsetFader( 0.0f ),
	m_iconBaseOffset( iconBaseOffset ),
	m_iconFadeOffset( iconFadeOffset )
{
}

//==============================
// OvrTextFade_Component::OnEvent_Impl
eMsgStatus OvrTextFade_Component::onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
												VRMenuObject * self, VRMenuEvent const & event )
{
	vAssert( handlesEvent( VRMenuEventFlags_t( event.eventType ) ) );
	switch ( event.eventType )
	{
		case VRMENU_EVENT_FRAME_UPDATE:
		return frame( app, vrFrame, menuMgr, self, event );
		case VRMENU_EVENT_FOCUS_GAINED:
		return focusGained( app, vrFrame, menuMgr, self, event );
		case VRMENU_EVENT_FOCUS_LOST:
		return focusLost( app, vrFrame, menuMgr, self, event );
		default:
		vAssert( !"Event flags mismatch!" ); // the constructor is specifying a flag that's not handled
		return MSG_STATUS_ALIVE;
	}
}

//==============================
// OvrTextFade_Component::Frame
eMsgStatus OvrTextFade_Component::frame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
											VRMenuObject * self, VRMenuEvent const & event )
{
	double t = ovr_GetTimeInSeconds();
	if ( m_startFadeInTime >= 0.0f && t >= m_startFadeInTime )
	{
		m_textAlphaFader.startFadeIn();
		m_iconOffsetFader.startFadeIn();
		m_startFadeInTime = -1.0f;
		// start bounding all when we begin to fade out
		VRMenuObjectFlags_t flags = self->flags();
		flags |= VRMENUOBJECT_BOUND_ALL;
		self->setFlags( flags );
	}
	else if ( m_startFadeOutTime >= 0.0f && t > m_startFadeOutTime )
	{
		m_textAlphaFader.startFadeOut();
		m_iconOffsetFader.startFadeOut();
		m_startFadeOutTime = -1.0f;
		// stop bounding all when faded out
		VRMenuObjectFlags_t flags = self->flags();
		flags &= ~VRMenuObjectFlags_t( VRMENUOBJECT_BOUND_ALL );
		self->setFlags( flags );
	}

	float const FADE_RATE = 1.0f / FADE_DURATION;

	m_textAlphaFader.update( FADE_RATE, vrFrame.DeltaSeconds );
	m_iconOffsetFader.update( FADE_RATE, vrFrame.DeltaSeconds );

    V4Vectf textColor = self->textColor();
	textColor.w = m_textAlphaFader.finalAlpha();
	self->setTextColor( textColor );

    V3Vectf curOffset = m_iconBaseOffset + ( m_iconOffsetFader.finalAlpha() * m_iconFadeOffset );
	self->setLocalPosition( curOffset );

	return MSG_STATUS_ALIVE;
}

//==============================
// OvrTextFade_Component::FocusGained
eMsgStatus OvrTextFade_Component::focusGained( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
												VRMenuObject * self, VRMenuEvent const & event )
{

	m_startFadeOutTime = -1.0;
	m_startFadeInTime = FADE_DELAY + ovr_GetTimeInSeconds();

	return MSG_STATUS_ALIVE;
}

//==============================
// OvrTextFade_Component::FocusLost
eMsgStatus OvrTextFade_Component::focusLost( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
												VRMenuObject * self, VRMenuEvent const & event )
{
	m_startFadeOutTime = FADE_DELAY + ovr_GetTimeInSeconds();
	m_startFadeInTime = -1.0;

	return MSG_STATUS_ALIVE;
}

} // namespace NervGear
