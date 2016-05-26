/************************************************************************************

Filename    :   SwipeHintComponent.cpp
Content     :
Created     :   Feb 12, 2015
Authors     :   Madhu Kalva, Jim Dose

Copyright   :   Copyright 2014 Oculus VR, Inc. All Rights reserved.

*************************************************************************************/

#include "SwipeHintComponent.h"
#include "VRMenuMgr.h"
#include "VRMenu.h"
#include "VZipFile.h"
#include "App.h"
#include "core/VTimer.h"

namespace NervGear
{
	const char * OvrSwipeHintComponent::TYPE_NAME = "OvrSwipeHintComponent";
	bool OvrSwipeHintComponent::ShowSwipeHints = true;

	//==============================
	//  OvrSwipeHintComponent::OvrSwipeHintComponent()
	OvrSwipeHintComponent::OvrSwipeHintComponent( const bool isRightSwipe, const float totalTime, const float timeOffset, const float delay )
	: VRMenuComponent( VRMenuEventFlags_t( VRMENU_EVENT_FRAME_UPDATE ) | VRMENU_EVENT_OPENING )
	, m_isRightSwipe( isRightSwipe )
	, m_totalTime( totalTime )
	, m_timeOffset( timeOffset )
	, m_delay( delay )
	, m_startTime( 0 )
	, m_shouldShow( false )
	, m_ignoreDelay( false )
	, m_totalAlpha()
	{
	}

	//=======================================================
	//  OvrSwipeHintComponent::CreateSwipeSuggestionIndicator
    menuHandle_t OvrSwipeHintComponent::CreateSwipeSuggestionIndicator( App * appPtr, VRMenu * rootMenu, const menuHandle_t rootHandle, const int menuId, const char * img, const VPosf pose, const V3Vectf direction )
	{
		const int NumSwipeTrails = 3;
		int imgWidth, imgHeight;
		OvrVRMenuMgr & menuManager = appPtr->vrMenuMgr();
		VRMenuFontParms fontParms( HORIZONTAL_CENTER, VERTICAL_CENTER, false, false, false, 1.0f );

		// Create Menu item for scroll hint root
		VRMenuId_t swipeHintId( menuId );
		VArray< VRMenuObjectParms const * > parms;
		VRMenuObjectParms parm( VRMENU_CONTAINER, VArray< VRMenuComponent* >(), VRMenuSurfaceParms(),
                                "swipe hint root", pose, V3Vectf( 1.0f ), VRMenuFontParms(),
								swipeHintId, VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ),
								VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );
		parms.append( &parm );
		rootMenu->addItems( menuManager, appPtr->defaultFont(), parms, rootHandle, false );
		parms.clear();

		menuHandle_t scrollHintHandle = rootMenu->handleForId( menuManager, swipeHintId );
		vAssert( scrollHintHandle.isValid() );
		GLuint swipeHintTexture = LoadTextureFromApplicationPackage( img, TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), imgWidth, imgHeight );
		VRMenuSurfaceParms swipeHintSurfParms( "", swipeHintTexture, imgWidth, imgHeight, SURFACE_TEXTURE_DIFFUSE,
												0, 0, 0, SURFACE_TEXTURE_MAX,
												0, 0, 0, SURFACE_TEXTURE_MAX );

        VPosf swipePose = pose;
		for( int i = 0; i < NumSwipeTrails; i++ )
		{
			swipePose.Position.y = ( imgHeight * ( i + 2 ) ) * 0.5f * direction.y * VRMenuObject::DEFAULT_TEXEL_SCALE;
			swipePose.Position.z = 0.01f * ( float )i;

			VArray< VRMenuComponent* > hintArrowComps;
			OvrSwipeHintComponent* hintArrowComp = new OvrSwipeHintComponent(false, 1.3333f, 0.4f + (float)i * 0.13333f, 5.0f);
			hintArrowComps.append( hintArrowComp );
            hintArrowComp->show( VTimer::Seconds() );

			VRMenuObjectParms * swipeIconLeftParms = new VRMenuObjectParms( VRMENU_STATIC, hintArrowComps,
                swipeHintSurfParms, "", swipePose, V3Vectf( 1.0f ), fontParms, VRMenuId_t(),
				VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_DEPTH ) | VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ),
				VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );
			parms.append( swipeIconLeftParms );
		}

		rootMenu->addItems( menuManager, appPtr->defaultFont(), parms, scrollHintHandle, false );
		DeletePointerArray( parms );
		parms.clear();

		return scrollHintHandle;
	}

	//==============================
	//  OvrSwipeHintComponent::Reset
	void OvrSwipeHintComponent::reset( VRMenuObject * self )
	{
		m_ignoreDelay 		= true;
		m_shouldShow 			= false;
        const double now 	= VTimer::Seconds();
		m_totalAlpha.set( now, m_totalAlpha.value( now ), now, 0.0f );
        self->setColor( V4Vectf( 1.0f, 1.0f, 1.0f, 0.0f ) );
	}

	//==============================
	//  OvrSwipeHintComponent::Show
	void OvrSwipeHintComponent::show( const double now )
	{
		if ( !m_shouldShow )
		{
			m_shouldShow 	= true;
			m_startTime 	= now + m_timeOffset + ( m_ignoreDelay ? 0.0f : m_delay );
			m_totalAlpha.set( now, m_totalAlpha.value( now ), now + 0.5f, 1.0f );
		}
	}

	//==============================
	//  OvrSwipeHintComponent::Hide
	void OvrSwipeHintComponent::hide( const double now )
	{
		if ( m_shouldShow )
		{
			m_totalAlpha.set( now, m_totalAlpha.value( now ), now + 0.5f, 0.0f );
			m_shouldShow = false;
		}
	}

	//==============================
	//  OvrSwipeHintComponent::OnEvent_Impl
	eMsgStatus OvrSwipeHintComponent::onEventImpl( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
													VRMenuObject * self, VRMenuEvent const & event )
	{
		switch ( event.eventType )
		{
		case VRMENU_EVENT_OPENING:
			return opening( app, vrFrame, menuMgr, self, event );
		case VRMENU_EVENT_FRAME_UPDATE:
			return frame( app, vrFrame, menuMgr, self, event );
		default:
			vAssert( !"Event flags mismatch!" );
			return MSG_STATUS_ALIVE;
		}
	}

	//==============================
	//  OvrSwipeHintComponent::Opening
	eMsgStatus OvrSwipeHintComponent::opening( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event )
	{
		reset( self );
		return MSG_STATUS_ALIVE;
	}

	//==============================
	//  OvrSwipeHintComponent::Frame
	eMsgStatus OvrSwipeHintComponent::frame( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event )
	{
		if ( ShowSwipeHints /* && Carousel->HasSelection() && CanSwipe() */ )
		{
            show( vrFrame.pose.timestamp );
		}
		else
		{
            hide( vrFrame.pose.timestamp );
		}

		m_ignoreDelay = false;

        float alpha = m_totalAlpha.value( vrFrame.pose.timestamp );
		if ( alpha > 0.0f )
		{
            double time = vrFrame.pose.timestamp - m_startTime;
			if ( time < 0.0f )
			{
				alpha = 0.0f;
			}
			else
			{
				float normTime = time / m_totalTime;
				alpha *= sin( M_PI * 2.0f * normTime );
				alpha = std::max( alpha, 0.0f );
			}
		}

        self->setColor( V4Vectf( 1.0f, 1.0f, 1.0f, alpha ) );

		return MSG_STATUS_ALIVE;
	}
}
