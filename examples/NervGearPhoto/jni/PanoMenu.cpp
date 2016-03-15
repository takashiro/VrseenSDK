/************************************************************************************

Filename    :   PanoMenu.cpp
Content     :
Created     :
Authors     :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Oculus360Photos/ directory. An additional grant
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "PanoMenu.h"

#include "gui/VRMenuMgr.h"
#include "gui/GuiSys.h"
#include "gui/DefaultComponent.h"
#include "gui/ActionComponents.h"
#include "gui/AnimComponents.h"
#include "gui/FolderBrowser.h"

#include "Oculus360Photos.h"
#include "PhotosMetaData.h"

namespace NervGear {

const VRMenuId_t OvrPanoMenu::ID_CENTER_ROOT( 1000 );
const VRMenuId_t OvrPanoMenu::ID_BROWSER_BUTTON( 1000 + 1011 );
const VRMenuId_t OvrPanoMenu::ID_FAVORITES_BUTTON( 1000 + 1012 );

char const * OvrPanoMenu::MENU_NAME = "PanoMenu";

static const Vector3f FWD( 0.0f, 0.0f, 1.0f );
static const Vector3f RIGHT( 1.0f, 0.0f, 0.0f );
static const Vector3f UP( 0.0f, 1.0f, 0.0f );
static const Vector3f DOWN( 0.0f, -1.0f, 0.0f );

static const int BUTTON_COOL_DOWN_SECONDS = 0.25f;

//==============================
// OvrPanoMenuRootComponent
// This component is attached to the root of PanoMenu
class OvrPanoMenuRootComponent : public VRMenuComponent
{
public:
	OvrPanoMenuRootComponent( OvrPanoMenu & panoMenu )
		: VRMenuComponent( VRMenuEventFlags_t( VRMENU_EVENT_FRAME_UPDATE ) | VRMENU_EVENT_OPENING )
		, PanoMenu( panoMenu )
		, CurrentPano( NULL )
	{
	}

private:
    eMsgStatus	onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event ) override
	{
		switch ( event.eventType )
		{
		case VRMENU_EVENT_FRAME_UPDATE:
			return OnFrame( app, vrFrame, menuMgr, self, event );
		case VRMENU_EVENT_OPENING:
			return OnOpening( app, vrFrame, menuMgr, self, event );
		default:
			OVR_ASSERT( !"Event flags mismatch!" ); // the constructor is specifying a flag that's not handled
			return MSG_STATUS_ALIVE;
		}
	}

	eMsgStatus OnOpening( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event )
	{
        CurrentPano = PanoMenu.photos()->activePano();
		// If opening PanoMenu without a Pano selected, bail
		if ( CurrentPano == NULL )
		{
			app->GetGuiSys( ).closeMenu( app, &PanoMenu, false );
		}
		LoadAttribution( self );
		return MSG_STATUS_CONSUMED;
	}

	void LoadAttribution( VRMenuObject * self )
	{
		if ( CurrentPano )
		{
			VString attribution = CurrentPano->title + "\n";
			attribution += "by ";
			attribution += CurrentPano->author;
            self->setText(attribution);
		}
	}

	eMsgStatus OnFrame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event )
	{
		Vector4f selfColor = self->color( );
		Vector4f selfTextColor = self->textColor();

		VRMenuObjectFlags_t attributionFlags = self->flags();

        const float fadeInAlpha = PanoMenu.photos( )->fadeLevel( );
        const float fadeOutAlpha = PanoMenu.fadeAlpha( );
        switch ( PanoMenu.photos()->currentState() )
		{
		case Oculus360Photos::MENU_PANO_LOADING:
            OVR_ASSERT( PanoMenu.photos() );
            if ( CurrentPano != PanoMenu.photos()->activePano() )
			{
                CurrentPano = PanoMenu.photos()->activePano();
				LoadAttribution( self );
			}
			// Hide attribution
			attributionFlags |= VRMenuObjectFlags_t( VRMENUOBJECT_DONT_RENDER );
			break;
		case Oculus360Photos::MENU_PANO_REOPEN_FADEIN:
		case Oculus360Photos::MENU_PANO_FADEIN:
			// Show attribution
			attributionFlags &= ~VRMenuObjectFlags_t( VRMENUOBJECT_DONT_RENDER );
			// Fade in burger
			selfColor.w = fadeInAlpha;
			selfTextColor.w = fadeInAlpha;
			break;
		case Oculus360Photos::MENU_PANO_FULLY_VISIBLE:
			// Show attribution
			attributionFlags &= ~VRMenuObjectFlags_t( VRMENUOBJECT_DONT_RENDER );
			break;
		case Oculus360Photos::MENU_PANO_FADEOUT:
			// Fade out burger
			selfColor.w = fadeOutAlpha;
			selfTextColor.w = fadeOutAlpha;
			if ( fadeOutAlpha == 0.0f )
			{
				app->GetGuiSys().closeMenu( app, &PanoMenu, false );
			}
			break;
		default:
			// Hide attribution
			attributionFlags |= VRMenuObjectFlags_t( VRMENUOBJECT_DONT_RENDER );
			break;
		}

		self->setFlags( attributionFlags );
		self->setTextColor( selfTextColor );
		self->setColor( selfColor );

		return MSG_STATUS_ALIVE;
	}

private:
	OvrPanoMenu &				PanoMenu;
	const OvrPhotosMetaDatum *	CurrentPano;
};

//==============================
// OvrPanoMenu
OvrPanoMenu * OvrPanoMenu::Create( App * app, Oculus360Photos * photos, OvrVRMenuMgr & menuMgr,
		BitmapFont const & font, OvrMetaData & metaData, float fadeOutTime, float radius )
{
	return new OvrPanoMenu( app, photos, menuMgr, font, metaData, fadeOutTime, radius );
}

OvrPanoMenu::OvrPanoMenu( App * app, Oculus360Photos * photos, OvrVRMenuMgr & menuMgr, BitmapFont const & font,
		OvrMetaData & metaData, float fadeOutTime, float radius )
	: VRMenu( MENU_NAME )
	, m_app( app )
	, m_menuMgr( menuMgr )
	, m_font( font )
	, m_photos( photos )
	, m_metaData( metaData )
	, m_loadingIconHandle( 0 )
	, m_attributionHandle( 0 )
	, m_browserButtonHandle( 0 )
	, m_swipeLeftIndicatorHandle( 0 )
	, m_swipeRightIndicatorHandle( 0 )
	, m_fader( 1.0f )
	, m_fadeOutTime( fadeOutTime )
	, m_currentFadeRate( 0.0f )
	, m_radius( radius )
	, m_buttonCoolDown( 0.0f )
{
	m_currentFadeRate = 1.0f / m_fadeOutTime;

	// Init with empty root
	init( menuMgr, font, 0.0f, VRMenuFlags_t() );

	// Create Attribution info view
	VArray< VRMenuObjectParms const * > parms;
	VArray< VRMenuComponent* > comps;
	VRMenuId_t attributionPanelId( ID_CENTER_ROOT.Get() + 10 );

	comps.append( new OvrPanoMenuRootComponent( *this ) );

	Quatf rot( DOWN, 0.0f );
	Vector3f dir( -FWD );
	Posef panelPose( rot, dir * m_radius );
	Vector3f panelScale( 1.0f );

	//const Posef textPose( Quatf(), Vector3f( 0.0f, 0.0f, 0.0f ) );

	const VRMenuFontParms fontParms( true, true, false, false, true, 0.525f, 0.45f, 1.0f );

	VRMenuObjectParms attrParms( VRMENU_STATIC, comps,
		VRMenuSurfaceParms(), "Attribution Panel", panelPose, panelScale, Posef(), Vector3f( 1.0f ), fontParms, attributionPanelId,
		VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_TEXT ), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

	parms.append( &attrParms );

	addItems( m_menuMgr, m_font, parms, rootHandle(), false );
	parms.clear();
	comps.clear();

	m_attributionHandle = handleForId( m_menuMgr, attributionPanelId );
	VRMenuObject * attributionObject = m_menuMgr.toObject( m_attributionHandle );
	OVR_ASSERT( attributionObject != NULL );

	//Browser button
	float const ICON_HEIGHT = 80.0f * VRMenuObject::DEFAULT_TEXEL_SCALE;
	VArray< VRMenuSurfaceParms > surfParms;

	Posef browserButtonPose( Quatf( ), UP * ICON_HEIGHT * 2.0f );

	comps.append( new OvrDefaultComponent( Vector3f( 0.0f, 0.0f, 0.05f ), 1.05f, 0.25f, 0.0f, Vector4f( 1.0f ), Vector4f( 1.0f ) ) );
	comps.append( new OvrButton_OnUp( this, ID_BROWSER_BUTTON ) );
	comps.append( new OvrSurfaceToggleComponent( ) );
	surfParms.append( VRMenuSurfaceParms ( "browser",
		"assets/nav_home_off.png", SURFACE_TEXTURE_DIFFUSE,
		NULL, SURFACE_TEXTURE_MAX, NULL, SURFACE_TEXTURE_MAX ) );
	surfParms.append( VRMenuSurfaceParms( "browser",
		"assets/nav_home_on.png", SURFACE_TEXTURE_DIFFUSE,
		NULL, SURFACE_TEXTURE_MAX, NULL, SURFACE_TEXTURE_MAX ) );
	VRMenuObjectParms browserButtonParms( VRMENU_BUTTON, comps, surfParms, "",
		browserButtonPose, Vector3f( 1.0f ), Posef( ), Vector3f( 1.0f ), fontParms,
		ID_BROWSER_BUTTON, VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_TEXT ),
		VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );
	parms.append( &browserButtonParms );

	addItems( m_menuMgr, m_font, parms, m_attributionHandle, false );
	parms.clear();
	comps.clear();
	surfParms.clear();

	m_browserButtonHandle = attributionObject->childHandleForId( m_menuMgr, ID_BROWSER_BUTTON );
	VRMenuObject * browserButtonObject = m_menuMgr.toObject( m_browserButtonHandle );
	OVR_ASSERT( browserButtonObject != NULL );
	OVR_UNUSED( browserButtonObject );

	//Favorites button
	Posef favoritesButtonPose( Quatf( ), DOWN * ICON_HEIGHT * 2.0f );

	comps.append( new OvrDefaultComponent( Vector3f( 0.0f, 0.0f, 0.05f ), 1.05f, 0.25f, 0.0f, Vector4f( 1.0f ), Vector4f( 1.0f ) ) );
	comps.append( new OvrButton_OnUp( this, ID_FAVORITES_BUTTON ) );
	comps.append( new OvrSurfaceToggleComponent() );

	surfParms.append( VRMenuSurfaceParms( "favorites_off",
		"assets/nav_star_off.png", SURFACE_TEXTURE_DIFFUSE,
		NULL, SURFACE_TEXTURE_MAX, NULL, SURFACE_TEXTURE_MAX ) );

	surfParms.append( VRMenuSurfaceParms( "favorites_on",
		"assets/nav_star_on.png", SURFACE_TEXTURE_DIFFUSE,
		NULL, SURFACE_TEXTURE_MAX, NULL, SURFACE_TEXTURE_MAX ) );

	surfParms.append( VRMenuSurfaceParms( "favorites_active_off",
		"assets/nav_star_active_off.png", SURFACE_TEXTURE_DIFFUSE,
		NULL, SURFACE_TEXTURE_MAX, NULL, SURFACE_TEXTURE_MAX ) );

	surfParms.append( VRMenuSurfaceParms( "favorites_active_on",
		"assets/nav_star_active_on.png", SURFACE_TEXTURE_DIFFUSE,
		NULL, SURFACE_TEXTURE_MAX, NULL, SURFACE_TEXTURE_MAX ) );

	VRMenuObjectParms favoritesButtonParms( VRMENU_BUTTON, comps, surfParms, "",
		favoritesButtonPose, Vector3f( 1.0f ), Posef( ), Vector3f( 1.0f ), fontParms,
		ID_FAVORITES_BUTTON, VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_TEXT ),
		VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );
	parms.append( &favoritesButtonParms );

	addItems( m_menuMgr, m_font, parms, m_attributionHandle, false );
	parms.clear();
	comps.clear();

	m_favoritesButtonHandle = attributionObject->childHandleForId( m_menuMgr, ID_FAVORITES_BUTTON );
	VRMenuObject * favoritesButtonObject = m_menuMgr.toObject( m_favoritesButtonHandle );
	OVR_ASSERT( favoritesButtonObject != NULL );
	OVR_UNUSED( favoritesButtonObject );

	// Swipe icons
	const int numFrames = 10;
	const int numTrails = 3;
	const int numChildren = 5;
	const float swipeFPS = 3.0f;
	const float factor = 1.0f / 8.0f;

	// Right container
	VRMenuId_t swipeRightId( ID_CENTER_ROOT.Get() + 401 );
	Quatf rotRight( DOWN, ( Mathf::TwoPi * factor ) );
	Vector3f rightDir( -FWD * rotRight );
	comps.append( new OvrTrailsAnimComponent( swipeFPS, true, numFrames, numTrails, numTrails ) );
	VRMenuObjectParms swipeRightRoot( VRMENU_CONTAINER, comps, VRMenuSurfaceParms( ), "",
		Posef( rotRight, rightDir * m_radius ), Vector3f( 1.0f ), Posef( ), Vector3f( 1.0f ), fontParms, swipeRightId,
		VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );
	parms.append( &swipeRightRoot );
	addItems( m_menuMgr, m_font, parms, m_attributionHandle, false );
	parms.clear();
	comps.clear();

	m_swipeRightIndicatorHandle = attributionObject->childHandleForId( m_menuMgr, swipeRightId );
	VRMenuObject * swipeRightRootObject = m_menuMgr.toObject( m_swipeRightIndicatorHandle );
	OVR_ASSERT( swipeRightRootObject != NULL );

	// Left container
	VRMenuId_t swipeLeftId( ID_CENTER_ROOT.Get( ) + 402 );
	Quatf rotLeft( DOWN, ( Mathf::TwoPi * -factor ) );
	Vector3f leftDir( -FWD * rotLeft );
	comps.append( new OvrTrailsAnimComponent( swipeFPS, true, numFrames, numTrails, numTrails ) );
	VRMenuObjectParms swipeLeftRoot( VRMENU_CONTAINER, comps, VRMenuSurfaceParms( ), "",
		Posef( rotLeft, leftDir * m_radius ), Vector3f( 1.0f ), Posef( ), Vector3f( 1.0f ), fontParms, swipeLeftId,
		VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );
	parms.append( &swipeLeftRoot );
	addItems( m_menuMgr, m_font, parms, m_attributionHandle, false );
	parms.clear();
	comps.clear();

	m_swipeLeftIndicatorHandle = attributionObject->childHandleForId( m_menuMgr, swipeLeftId );
	VRMenuObject * swipeLeftRootObject = m_menuMgr.toObject( m_swipeLeftIndicatorHandle );
	OVR_ASSERT( swipeLeftRootObject != NULL );

	// Arrow frame children
	const char * swipeRightIcon = "assets/nav_arrow_right.png";
	const char * swipeLeftIcon = "assets/nav_arrow_left.png";

	VRMenuSurfaceParms rightIndicatorSurfaceParms( "swipeRightSurface",
		swipeRightIcon, SURFACE_TEXTURE_DIFFUSE,
		NULL, SURFACE_TEXTURE_MAX, NULL, SURFACE_TEXTURE_MAX );

	VRMenuSurfaceParms leftIndicatorSurfaceParms( "swipeLeftSurface",
		swipeLeftIcon, SURFACE_TEXTURE_DIFFUSE,
		NULL, SURFACE_TEXTURE_MAX, NULL, SURFACE_TEXTURE_MAX );

	const float surfaceWidth = 25 * VRMenuObject::DEFAULT_TEXEL_SCALE;

	for ( int i = 0; i < numChildren; ++i )
	{
 		//right frame
		const Vector3f rightPos = ( RIGHT * surfaceWidth * i ) - ( FWD * i * 0.1f );
		VRMenuObjectParms swipeRightFrame( VRMENU_STATIC, VArray< VRMenuComponent* >(), rightIndicatorSurfaceParms, "",
			Posef( Quatf( ), rightPos ), Vector3f( 1.0f ), Posef( ), Vector3f( 1.0f ), fontParms, VRMenuId_t( ),
			VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );
		parms.append( &swipeRightFrame );
		addItems( m_menuMgr, m_font, parms, m_swipeRightIndicatorHandle, false );
		parms.clear();

		// left frame
		const Vector3f leftPos = ( (-RIGHT) * surfaceWidth * i ) - ( FWD * i * 0.1f );
		VRMenuObjectParms swipeLeftFrame( VRMENU_STATIC, VArray< VRMenuComponent* >(), leftIndicatorSurfaceParms, "",
			Posef( Quatf( ), leftPos ), Vector3f( 1.0f ), Posef( ), Vector3f( 1.0f ), fontParms, VRMenuId_t( ),
			VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );
		parms.append( &swipeLeftFrame );
		addItems( m_menuMgr, m_font, parms, m_swipeLeftIndicatorHandle, false );
		parms.clear();
	}

	if ( OvrTrailsAnimComponent* animRightComp = swipeRightRootObject->GetComponentByName< OvrTrailsAnimComponent >( ) )
	{
		animRightComp->play( );
	}

	if ( OvrTrailsAnimComponent* animLeftComp = swipeLeftRootObject->GetComponentByName< OvrTrailsAnimComponent >( ) )
	{
		animLeftComp->play( );
	}
}

OvrPanoMenu::~OvrPanoMenu()
{

}

void OvrPanoMenu::frameImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, BitmapFont const & font, BitmapFontSurface & fontSurface, gazeCursorUserId_t const gazeUserId )
{
	m_fader.update( m_currentFadeRate, vrFrame.DeltaSeconds );

	if ( m_buttonCoolDown > 0.0f )
	{
		m_buttonCoolDown -= vrFrame.DeltaSeconds;
	}
}

void OvrPanoMenu::onItemEvent_Impl( App * app, VRMenuId_t const itemId, VRMenuEvent const & event )
{
	if ( m_buttonCoolDown <= 0.0f )
	{
		m_buttonCoolDown = BUTTON_COOL_DOWN_SECONDS;

        if ( m_photos->allowPanoInput() )
		{
			if ( itemId.Get() == ID_BROWSER_BUTTON.Get() )
			{
				m_photos->SetMenuState( Oculus360Photos::MENU_BROWSER );
			}
			else if ( itemId.Get() == ID_FAVORITES_BUTTON.Get() )
			{
                const TagAction actionTaken = static_cast< TagAction >( m_photos->toggleCurrentAsFavorite() );
                const bool forceShowSwipeIndicators = ( actionTaken == TAG_REMOVED ) && ( m_photos->numPanosInActiveCategory() == 1 );
                updateButtonsState( m_photos->activePano( ), forceShowSwipeIndicators );
			}
		}
	}
}

void OvrPanoMenu::startFadeOut()
{
	m_fader.reset( );
	m_fader.startFadeOut();
}

float OvrPanoMenu::fadeAlpha() const
{
	return m_fader.finalAlpha();
}

void OvrPanoMenu::updateButtonsState( const OvrMetaDatum * const ActivePano, bool showSwipeOverride /*= false*/ )
{
	// Reset button time
	m_buttonCoolDown = BUTTON_COOL_DOWN_SECONDS;

	// Update favo
	bool isFavorite = false;

	for ( int i = 0; i < ActivePano->tags.length( ); ++i )
	{
		if ( ActivePano->tags[ i ] == "Favorites" )
		{
			isFavorite = true;
			break;
		}
	}

	VRMenuObject * favoritesButtonObject = m_menuMgr.toObject( m_favoritesButtonHandle );
	OVR_ASSERT( favoritesButtonObject != NULL );

	if ( OvrSurfaceToggleComponent * favToggleComp = favoritesButtonObject->GetComponentByName<OvrSurfaceToggleComponent>() )
	{
		const int fav = isFavorite ? 2 : 0;
		favToggleComp->setGroupIndex( fav );
	}

	VRMenuObject * swipeRight = m_menuMgr.toObject( m_swipeRightIndicatorHandle );
	OVR_ASSERT( swipeRight != NULL );

	VRMenuObject * swipeLeft = m_menuMgr.toObject( m_swipeLeftIndicatorHandle );
	OVR_ASSERT( swipeLeft != NULL );

    const bool showSwipeIndicators = showSwipeOverride || ( m_photos->numPanosInActiveCategory( ) > 1 );

	VRMenuObjectFlags_t flagsLeft = swipeRight->flags( );
	VRMenuObjectFlags_t flagsRight = swipeRight->flags( );

	if ( showSwipeIndicators )
	{
		flagsLeft &= ~VRMenuObjectFlags_t( VRMENUOBJECT_DONT_RENDER );
		flagsRight &= ~VRMenuObjectFlags_t( VRMENUOBJECT_DONT_RENDER );
	}
	else
	{
		flagsLeft |= VRMenuObjectFlags_t( VRMENUOBJECT_DONT_RENDER );
		flagsRight |= VRMenuObjectFlags_t( VRMENUOBJECT_DONT_RENDER );
	}

	swipeLeft->setFlags( flagsLeft );
	swipeRight->setFlags( flagsRight );
}

}
