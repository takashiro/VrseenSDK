/************************************************************************************

Filename    :   DefaultComponent.h
Content     :   A default menu component that handles basic actions most menu items need.
Created     :   July 25, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#include "DefaultComponent.h"


#include "../Input.h"

namespace NervGear {

//==============================
//  OvrDefaultComponent::
OvrDefaultComponent::OvrDefaultComponent( V3Vectf const & hilightOffset, float const hilightScale,
        float const fadeDuration, float const fadeDelay, V4Vectf const & textNormalColor,
        V4Vectf const & textHilightColor ) :
    VRMenuComponent( VRMenuEventFlags_t( VRMENU_EVENT_TOUCH_DOWN ) |
            VRMENU_EVENT_TOUCH_UP |
            VRMENU_EVENT_FOCUS_GAINED |
            VRMENU_EVENT_FOCUS_LOST |
            VRMENU_EVENT_FRAME_UPDATE ),
    m_hilightFader( 0.0f ),
    m_startFadeInTime( -1.0 ),
    m_startFadeOutTime( -1.0 ),
    m_hilightOffset( hilightOffset ),
    m_hilightScale( hilightScale ),
    m_fadeDuration( fadeDuration ),
    m_fadeDelay( fadeDelay ),
	m_textNormalColor( textNormalColor ),
	m_textHilightColor( textHilightColor ),
	m_suppressText( false )
{
}

//==============================
//  OvrDefaultComponent::OnEvent_Impl
eMsgStatus OvrDefaultComponent::onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
    switch( event.eventType )
    {
        case VRMENU_EVENT_FRAME_UPDATE:
            return frame( app, vrFrame, menuMgr, self, event );
        case VRMENU_EVENT_FOCUS_GAINED:
            return focusGained( app, vrFrame, menuMgr, self, event );
        case VRMENU_EVENT_FOCUS_LOST:
            return focusLost( app, vrFrame, menuMgr, self, event );
        case VRMENU_EVENT_TOUCH_DOWN:
            m_downSoundLimiter.playSound( app, "sv_panel_touch_down", 0.1 );
            return MSG_STATUS_ALIVE;
        case VRMENU_EVENT_TOUCH_UP:
            m_upSoundLimiter.playSound( app, "sv_panel_touch_up", 0.1 );
            return MSG_STATUS_ALIVE;
        default:
            vAssert( !"Event flags mismatch!" );
            return MSG_STATUS_ALIVE;
    }
}

//==============================
//  OvrDefaultComponent::Frame
eMsgStatus OvrDefaultComponent::frame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
    double t = ovr_GetTimeInSeconds();
    if ( m_startFadeInTime >= 0.0f && t >= m_startFadeInTime )
    {
        m_hilightFader.startFadeIn();
        m_startFadeInTime = -1.0f;
    }
    else if ( m_startFadeOutTime >= 0.0f && t > m_startFadeOutTime )
    {
        m_hilightFader.startFadeOut();
        m_startFadeOutTime = -1.0f;
    }

    float const fadeRate = 1.0f / m_fadeDuration;
    m_hilightFader.update( fadeRate, vrFrame.DeltaSeconds );

    float const hilightAlpha = m_hilightFader.finalAlpha();
    V3Vectf offset = m_hilightOffset * hilightAlpha;
    self->setHilightPose( VPosf( VQuatf(), offset ) );

	int additiveSurfIndex = self->findSurfaceWithTextureType( SURFACE_TEXTURE_ADDITIVE, true );
	if ( additiveSurfIndex >= 0 )
	{
        V4Vectf surfColor = self->getSurfaceColor( additiveSurfIndex );
		surfColor.w = hilightAlpha;
		self->setSurfaceColor( additiveSurfIndex, surfColor );
	}

    float const scale = ( ( m_hilightScale - 1.0f ) * hilightAlpha ) + 1.0f;
    self->setHilightScale( scale );

	if ( m_suppressText )
	{
        self->setTextColor( V4Vectf( 0.0f ) );
	}
	else
	{
        V4Vectf colorDelta = m_textHilightColor - m_textNormalColor;
        V4Vectf curColor = m_textNormalColor + ( colorDelta * hilightAlpha );
		self->setTextColor( curColor );
	}

    return MSG_STATUS_ALIVE;
}

//==============================
//  OvrDefaultComponent::FocusGained
eMsgStatus OvrDefaultComponent::focusGained( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
    // set the hilight flag
    self->setHilighted( true );
	m_gazeOverSoundLimiter.playSound( app, "sv_focusgained", 0.1 );

    m_startFadeOutTime = -1.0;
    m_startFadeInTime = m_fadeDelay + ovr_GetTimeInSeconds();
    return MSG_STATUS_ALIVE;
}

//==============================
//  OvrDefaultComponent::FocusLost
eMsgStatus OvrDefaultComponent::focusLost( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
    // clear the hilight flag
    self->setHilighted( false );

    m_startFadeInTime = -1.0;
    m_startFadeOutTime = m_fadeDelay + ovr_GetTimeInSeconds();
    return MSG_STATUS_ALIVE;
}

const char * OvrSurfaceToggleComponent::TYPE_NAME = "OvrSurfaceToggleComponent";


//==============================
//  OvrSurfaceToggleComponent::OnEvent_Impl
eMsgStatus OvrSurfaceToggleComponent::onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
	VRMenuObject * self, VRMenuEvent const & event )
{
	switch ( event.eventType )
	{
	case VRMENU_EVENT_FRAME_UPDATE:
		return frame( app, vrFrame, menuMgr, self, event );
	default:
		vAssert( !"Event flags mismatch!" );
		return MSG_STATUS_ALIVE;
	}
}

//==============================
//  OvrSurfaceToggleComponent::FocusLost
eMsgStatus OvrSurfaceToggleComponent::frame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event )
{
	const int numSurfaces = self->numSurfaces();
	for ( int i = 0; i < numSurfaces; ++i )
	{
		self->setSurfaceVisible( i, false );
	}

	if ( self->isHilighted() )
	{
		self->setSurfaceVisible( m_groupIndex + 1, true );
	}
	else
	{
		self->setSurfaceVisible( m_groupIndex, true );
	}
	return MSG_STATUS_ALIVE;
}

} // namespace NervGear
