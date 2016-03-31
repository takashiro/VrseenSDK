/************************************************************************************

Filename    :   VideoMenu.cpp
Content     :
Created     :
Authors     :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Oculus360Videos/ directory. An additional grant
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "VideoMenu.h"

#include "gui/VRMenuMgr.h"
#include "gui/GuiSys.h"
#include "gui/DefaultComponent.h"
#include "gui/ActionComponents.h"
#include "gui/FolderBrowser.h"

#include "Oculus360Videos.h"
#include "VideosMetaData.h"

namespace NervGear {

const VRMenuId_t OvrVideoMenu::ID_CENTER_ROOT( 1000 );
const VRMenuId_t OvrVideoMenu::ID_BROWSER_BUTTON( 1000 + 1011 );
const VRMenuId_t OvrVideoMenu::ID_VIDEO_BUTTON( 1000 + 1012 );

char const * OvrVideoMenu::MENU_NAME = "VideoMenu";

static const V3Vectf FWD( 0.0f, 0.0f, 1.0f );
static const V3Vectf RIGHT( 1.0f, 0.0f, 0.0f );
static const V3Vectf UP( 0.0f, 1.0f, 0.0f );
static const V3Vectf DOWN( 0.0f, -1.0f, 0.0f );

static const int BUTTON_COOL_DOWN_SECONDS = 0.25f;

//==============================
// OvrVideoMenuRootComponent
// This component is attached to the root of VideoMenu
class OvrVideoMenuRootComponent : public VRMenuComponent
{
public:
	OvrVideoMenuRootComponent( OvrVideoMenu & videoMenu )
		: VRMenuComponent( VRMenuEventFlags_t( VRMENU_EVENT_FRAME_UPDATE ) | VRMENU_EVENT_OPENING )
		, VideoMenu( videoMenu )
		, CurrentVideo( NULL )
	{
	}

private:
	virtual eMsgStatus	onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
		VRMenuObject * self, VRMenuEvent const & event )
	{
		switch ( event.eventType )
		{
		case VRMENU_EVENT_FRAME_UPDATE:
			return OnFrame( app, vrFrame, menuMgr, self, event );
		case VRMENU_EVENT_OPENING:
			return OnOpening( app, vrFrame, menuMgr, self, event );
		default:
			vAssert( !"Event flags mismatch!" ); // the constructor is specifying a flag that's not handled
			return MSG_STATUS_ALIVE;
		}
	}

	eMsgStatus OnOpening( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event )
	{
		CurrentVideo = (OvrVideosMetaDatum *)( VideoMenu.GetVideos()->GetActiveVideo() );
		// If opening VideoMenu without a Video selected, bail
		if ( CurrentVideo == NULL )
		{
			app->guiSys().closeMenu( app, &VideoMenu, false );
		}
		LoadAttribution( self );
		return MSG_STATUS_CONSUMED;
	}

	void LoadAttribution( VRMenuObject * self )
	{
		if ( CurrentVideo )
		{
			self->setText( CurrentVideo->Title );
		}
	}

	eMsgStatus OnFrame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event )
	{
		return MSG_STATUS_ALIVE;
	}

private:
	OvrVideoMenu &			VideoMenu;
	OvrVideosMetaDatum *	CurrentVideo;
};

//==============================
// OvrVideoMenu
OvrVideoMenu * OvrVideoMenu::Create( App * app, Oculus360Videos * videos, OvrVRMenuMgr & menuMgr, BitmapFont const & font, OvrMetaData & metaData, float fadeOutTime, float radius )
{
	return new OvrVideoMenu( app, videos, menuMgr, font, metaData, fadeOutTime, radius );
}

OvrVideoMenu::OvrVideoMenu( App * app, Oculus360Videos * videos, OvrVRMenuMgr & menuMgr, BitmapFont const & font,
	OvrMetaData & metaData, float fadeOutTime, float radius )
	: VRMenu( MENU_NAME )
	, AppPtr( app )
	, MenuMgr( menuMgr )
	, Font( font )
	, Videos( videos )
	, MetaData( metaData )
	, LoadingIconHandle( 0 )
	, AttributionHandle( 0 )
	, BrowserButtonHandle( 0 )
	, VideoControlButtonHandle( 0 )
	, Radius( radius )
	, ButtonCoolDown( 0.0f )
	, OpenTime( 0.0 )
{
	// Init with empty root
	init( menuMgr, font, 0.0f, VRMenuFlags_t() );

	// Create Attribution info view
	VArray< VRMenuObjectParms const * > parms;
	VArray< VRMenuComponent* > comps;
	VRMenuId_t attributionPanelId( ID_CENTER_ROOT.Get() + 10 );

	comps.append( new OvrVideoMenuRootComponent( *this ) );

    VQuatf rot( DOWN, 0.0f );
    V3Vectf dir( -FWD );
    VPosf panelPose( rot, dir * Radius );
    V3Vectf panelScale( 1.0f );

	const VRMenuFontParms fontParms( true, true, false, false, true, 0.525f, 0.45f, 1.0f );

	VRMenuObjectParms attrParms( VRMENU_STATIC, comps,
        VRMenuSurfaceParms(), "Attribution Panel", panelPose, panelScale, VPosf(), V3Vectf( 1.0f ), fontParms, attributionPanelId,
		VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

	parms.append( &attrParms );

	addItems( MenuMgr, Font, parms, rootHandle(), false );
	parms.clear();
	comps.clear();

	AttributionHandle = handleForId( MenuMgr, attributionPanelId );
	VRMenuObject * attributionObject = MenuMgr.toObject( AttributionHandle );
	vAssert( attributionObject != NULL );

	//Browser button
	float const ICON_HEIGHT = 80.0f * VRMenuObject::DEFAULT_TEXEL_SCALE;
	VArray< VRMenuSurfaceParms > surfParms;

    VPosf browserButtonPose( VQuatf(), UP * ICON_HEIGHT * 2.0f );

    comps.append( new OvrDefaultComponent( V3Vectf( 0.0f, 0.0f, 0.05f ), 1.05f, 0.25f, 0.0f, V4Vectf( 1.0f ), V4Vectf( 1.0f ) ) );
	comps.append( new OvrButton_OnUp( this, ID_BROWSER_BUTTON ) );
	comps.append( new OvrSurfaceToggleComponent( ) );
	surfParms.append( VRMenuSurfaceParms( "browser",
		"assets/nav_home_off.png", SURFACE_TEXTURE_DIFFUSE,
		NULL, SURFACE_TEXTURE_MAX, NULL, SURFACE_TEXTURE_MAX ) );
	surfParms.append( VRMenuSurfaceParms( "browser",
		"assets/nav_home_on.png", SURFACE_TEXTURE_DIFFUSE,
		NULL, SURFACE_TEXTURE_MAX, NULL, SURFACE_TEXTURE_MAX ) );
	VRMenuObjectParms browserButtonParms( VRMENU_BUTTON, comps, surfParms, "",
        browserButtonPose, V3Vectf( 1.0f ), VPosf(), V3Vectf( 1.0f ), fontParms,
		ID_BROWSER_BUTTON, VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_TEXT ),
		VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );
	parms.append( &browserButtonParms );

	addItems( MenuMgr, Font, parms, AttributionHandle, false );
	parms.clear();
	comps.clear();
	surfParms.clear();

	BrowserButtonHandle = attributionObject->childHandleForId( MenuMgr, ID_BROWSER_BUTTON );
	VRMenuObject * browserButtonObject = MenuMgr.toObject( BrowserButtonHandle );
	vAssert( browserButtonObject != NULL );
	NV_UNUSED( browserButtonObject );

	//Video control button
    VPosf videoButtonPose( VQuatf(), DOWN * ICON_HEIGHT * 2.0f );

    comps.append( new OvrDefaultComponent( V3Vectf( 0.0f, 0.0f, 0.05f ), 1.05f, 0.25f, 0.0f, V4Vectf( 1.0f ), V4Vectf( 1.0f ) ) );
	comps.append( new OvrButton_OnUp( this, ID_VIDEO_BUTTON ) );
	comps.append( new OvrSurfaceToggleComponent( ) );
	surfParms.append( VRMenuSurfaceParms( "browser",
		"assets/nav_restart_off.png", SURFACE_TEXTURE_DIFFUSE,
		NULL, SURFACE_TEXTURE_MAX, NULL, SURFACE_TEXTURE_MAX ) );
	surfParms.append( VRMenuSurfaceParms( "browser",
		"assets/nav_restart_on.png", SURFACE_TEXTURE_DIFFUSE,
		NULL, SURFACE_TEXTURE_MAX, NULL, SURFACE_TEXTURE_MAX ) );
	VRMenuObjectParms controlButtonParms( VRMENU_BUTTON, comps, surfParms, "",
        videoButtonPose, V3Vectf( 1.0f ), VPosf(), V3Vectf( 1.0f ), fontParms,
		ID_VIDEO_BUTTON, VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_TEXT ),
		VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );
	parms.append( &controlButtonParms );

	addItems( MenuMgr, Font, parms, AttributionHandle, false );
	parms.clear();
	comps.clear();

	VideoControlButtonHandle = attributionObject->childHandleForId( MenuMgr, ID_VIDEO_BUTTON );
	VRMenuObject * controlButtonObject = MenuMgr.toObject( VideoControlButtonHandle );
	vAssert( controlButtonObject != NULL );
	NV_UNUSED( controlButtonObject );

}

OvrVideoMenu::~OvrVideoMenu()
{

}

void OvrVideoMenu::openImpl( App * app, OvrGazeCursor & gazeCursor )
{
	ButtonCoolDown = BUTTON_COOL_DOWN_SECONDS;

	OpenTime = ovr_GetTimeInSeconds();
}

void OvrVideoMenu::frameImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, BitmapFont const & font, BitmapFontSurface & fontSurface, gazeCursorUserId_t const gazeUserId )
{
	if ( ButtonCoolDown > 0.0f )
	{
		ButtonCoolDown -= vrFrame.DeltaSeconds;
	}
}

void OvrVideoMenu::onItemEvent_Impl( App * app, VRMenuId_t const itemId, VRMenuEvent const & event )
{
	const double now = ovr_GetTimeInSeconds();
	if ( ButtonCoolDown <= 0.0f && (now - OpenTime > 0.5))
	{
		ButtonCoolDown = BUTTON_COOL_DOWN_SECONDS;

		if ( itemId.Get() == ID_BROWSER_BUTTON.Get() )
		{
			Videos->SetMenuState( Oculus360Videos::MENU_BROWSER );
		}
		else if ( itemId.Get( ) == ID_VIDEO_BUTTON.Get( ) )
		{
			Videos->SeekTo( 0 );
		}
	}
}

}
