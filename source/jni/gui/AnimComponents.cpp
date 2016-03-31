 /************************************************************************************

Filename    :   SurfaceAnim_Component.cpp
Content     :   A reusable component for animating VR menu object surfaces.
Created     :   Sept 23, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, Inc. All Rights reserved.


*************************************************************************************/

#include "AnimComponents.h"
#include "VAlgorithm.h"
#include "VRMenuObject.h"
#include "../api/VKernel.h"
#include "VRMenuMgr.h"

namespace NervGear {

//================================
// OvrAnimComponent::OvrAnimComponent
OvrAnimComponent::OvrAnimComponent( float const framesPerSecond, bool const looping ) :
	VRMenuComponent( VRMENU_EVENT_FRAME_UPDATE ),
	m_baseTime( 0.0 ),
	m_baseFrame( 0 ),
	m_curFrame( 0 ),
	m_framesPerSecond( framesPerSecond ),
	m_animState( ANIMSTATE_PAUSED ),
	m_looping( looping ),
	m_forceVisibilityUpdate( false ),
	m_fractionalFrame( 0.0f ),
	m_floatFrame( 0.0 )
{
}

//================================
// OvrAnimComponent::Frame
eMsgStatus OvrAnimComponent::frame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
	VRMenuObject * self, VRMenuEvent const & event )
{
	// only recalculate the current frame if playing
	if ( m_animState == ANIMSTATE_PLAYING )
	{
		double timePassed = ovr_GetTimeInSeconds() - m_baseTime;
		m_floatFrame = timePassed * m_framesPerSecond;
		int totalFrames = ( int )floor( m_floatFrame );
		m_fractionalFrame = m_floatFrame - totalFrames;
		int numFrames = getNumFrames( self );
		int frame = m_baseFrame + totalFrames;
		m_curFrame = !m_looping ? VAlgorithm::Clamp( frame, 0, numFrames - 1 ) : frame % numFrames;
		setFrameVisibilities( app, vrFrame, menuMgr, self );
	}
	else if ( m_forceVisibilityUpdate )
	{
		setFrameVisibilities( app, vrFrame, menuMgr, self );
		m_forceVisibilityUpdate = false;
	}

	return MSG_STATUS_ALIVE;
}

//================================
// OvrAnimComponent::SetFrame
void OvrAnimComponent::setFrame( VRMenuObject * self, int const frameNum )
{
	m_curFrame = VAlgorithm::Clamp( frameNum, 0, getNumFrames( self ) - 1 );
	// we must reset the base frame and the current time so that the frame calculation
	// remains correct if we're playing.  If we're not playing, this will cause the
	// next Play() to start from this frame.
	m_baseFrame = frameNum;
	m_baseTime = ovr_GetTimeInSeconds();
	m_forceVisibilityUpdate = true;	// make sure visibilities are set next frame update
}

//================================
// OvrAnimComponent::Play
void OvrAnimComponent::play()
{
	m_animState = ANIMSTATE_PLAYING;
	m_baseTime = ovr_GetTimeInSeconds();
	// on a play we offset the base frame to the current frame so a resume from pause doesn't restart
	m_baseFrame = m_curFrame;
}

//================================
// OvrAnimComponent::Pause
void OvrAnimComponent::pause()
{
	m_animState = ANIMSTATE_PAUSED;
}

eMsgStatus OvrAnimComponent::onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event )
{
	switch ( event.eventType )
	{
	case VRMENU_EVENT_FRAME_UPDATE:
		return frame( app, vrFrame, menuMgr, self, event );
	default:
		vAssert( !"Event flags mismatch!" ); // the constructor is specifying a flag that's not handled
		return MSG_STATUS_ALIVE;
	}
}

//==============================================================================================
// OvrSurfaceAnimComponent
//==============================================================================================

const char * OvrSurfaceAnimComponent::TYPE_NAME = "OvrSurfaceAnimComponent";

//================================
// OvrSurfaceAnimComponent::
OvrSurfaceAnimComponent::OvrSurfaceAnimComponent( float const framesPerSecond, bool const looping, int const surfacesPerFrame ) :
	OvrAnimComponent( framesPerSecond, looping ),
	m_surfacesPerFrame( surfacesPerFrame )
{
}

//================================
// OvrSurfaceAnimComponent::SetFrameVisibilities
void OvrSurfaceAnimComponent::setFrameVisibilities( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self ) const
{
	int minIndex = curFrame() * m_surfacesPerFrame;
	int maxIndex = ( curFrame() + 1 ) * m_surfacesPerFrame;
	for ( int i = 0; i < self->numSurfaces(); ++i )
	{
		self->setSurfaceVisible( i, i >= minIndex && i < maxIndex );
	}
}

//================================
// OvrSurfaceAnimComponent::NumFrames
int OvrSurfaceAnimComponent::getNumFrames( VRMenuObject * self ) const
{
	return self->numSurfaces() / m_surfacesPerFrame;
}

//==============================================================
// OvrChildrenAnimComponent
//
const char * OvrTrailsAnimComponent::TYPE_NAME = "OvrChildrenAnimComponent";

OvrTrailsAnimComponent::OvrTrailsAnimComponent( float const framesPerSecond, bool const looping,
	int const numFrames, int const numFramesAhead, int const numFramesBehind )
	: OvrAnimComponent( framesPerSecond, looping )
	, m_numFrames( numFrames )
	, m_framesAhead( numFramesAhead )
	, m_framesBehind( numFramesBehind )
{

}

float OvrTrailsAnimComponent::getAlphaForFrame( const int frame ) const
{
	const int currentFrame = curFrame( );
	if ( frame == currentFrame )
		return	1.0f;

	const float alpha = fractionalFrame( );
	const float aheadFactor = 1.0f / m_framesAhead;
	for ( int ahead = 1; ahead <= m_framesAhead; ++ahead )
	{
		if ( frame == ( currentFrame + ahead ) )
		{
			return ( alpha * aheadFactor ) + ( aheadFactor * ( m_framesAhead - ahead ) );
		}
	}

	const float invAlpha = 1.0f - alpha;
	const float behindFactor = 1.0f / m_framesBehind;
	for ( int behind = 1; behind < m_framesBehind; ++behind )
	{
		if ( frame == ( currentFrame - behind ) )
		{
			return ( invAlpha * behindFactor ) + ( behindFactor * ( m_framesBehind - behind ) );
		}
	}

	return 0.0f;
}

void OvrTrailsAnimComponent::setFrameVisibilities( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self ) const
{
//	LOG( "FracFrame: %f", GetFractionalFrame() );
	for ( int i = 0; i < self->numChildren(); ++i )
	{
		menuHandle_t childHandle = self->getChildHandleForIndex( i );
		if ( VRMenuObject * childObject = menuMgr.toObject( childHandle ) )
		{
            V4Vectf color = childObject->color();
			color.w = getAlphaForFrame( i );
			childObject->setColor( color );
		}
	}
}

int OvrTrailsAnimComponent::getNumFrames( VRMenuObject * self ) const
{
	return m_numFrames;
}

} // namespace NervGear
