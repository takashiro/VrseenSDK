/************************************************************************************

Filename    :   VolumePopup.cpp
Content     :   The main menu that appears in native apps when pressing the HMT button.
Created     :   July 25, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#include "VolumePopup.h"

#include <android/keycodes.h>
#include "App.h"
#include "VRMenuComponent.h"
#include "GuiSys.h"
#include "VRMenuMgr.h"
#include "DefaultComponent.h"
#include "BitmapFont.h"
#include "TextFade_Component.h"
#include "Alg.h"
#include "VApkFile.h"

namespace NervGear {

const int OvrVolumePopup::NumVolumeTics = 15;

VRMenuId_t OvrVolumePopup::ID_BACKGROUND( VRMenu::GetRootId().Get() + 1000 );
VRMenuId_t OvrVolumePopup::ID_VOLUME_ICON( VRMenu::GetRootId().Get() + 1001 );
VRMenuId_t OvrVolumePopup::ID_VOLUME_TEXT( VRMenu::GetRootId().Get() + 1002 );
VRMenuId_t OvrVolumePopup::ID_VOLUME_TICKS( VRMenu::GetRootId().Get() + 1003 );

const double OvrVolumePopup::VolumeMenuFadeDelay = 3;

const char * OvrVolumePopup::MENU_NAME = "Volume";

//==============================
// OvrVolumePopup::OvrVolumePopup
OvrVolumePopup::OvrVolumePopup() :
    VRMenu( MENU_NAME ),
	m_volumeTextOffset(),
	m_currentVolume( -1 )

{
}

//==============================
// OvrVolumePopup::~OvrVolumePopup
OvrVolumePopup::~OvrVolumePopup()
{
}

//==============================
// OvrVolumePopup::Create
OvrVolumePopup * OvrVolumePopup::Create( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font )
{
    OvrVolumePopup * menu = new OvrVolumePopup;

	VArray< VRMenuObjectParms > defaultAppMenuItems;

	{
        V3Vectf fwd( 0.0f, 0.0f, 1.0f );
        V3Vectf up( 0.0f, 1.0f, 0.0f );
        V3Vectf right( fwd.Cross( up ) * -1.0f );

		VRMenuFontParms fontParms( HORIZONTAL_LEFT, VERTICAL_CENTER, false, false, false, 0.5f );

        V3Vectf menuOffset( 0.0f, 64 * VRMenuObject::DEFAULT_TEXEL_SCALE, 0.0f );

	    int backgroundWidth = 0;
	    int backgroundHeight = 0;
		GLuint backgroundTexture = LoadTextureFromApplicationPackage( "res/raw/volume_bg.png",
			TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), backgroundWidth, backgroundHeight );

	    int volumeIconWidth = 0;
	    int volumeIconHeight = 0;
		GLuint volumeIconTexture = LoadTextureFromApplicationPackage( "res/raw/volume_icon.png",
			TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), volumeIconWidth, volumeIconHeight );

	    int volumeTickOffWidth = 0;
	    int volumeTickOffHeight = 0;
		GLuint volumeTickOffTexture = LoadTextureFromApplicationPackage( "res/raw/volume_tick_off.png",
			TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), volumeTickOffWidth, volumeTickOffHeight );

	    int volumeTickOnWidth = 0;
	    int volumeTickOnHeight = 0;
		GLuint volumeTickOnTexture = LoadTextureFromApplicationPackage( "res/raw/volume_tick_on.png",
			TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), volumeTickOnWidth, volumeTickOnHeight );

		int volumeTickPadding = 4;
		int volumeTickWidth = 6 + volumeTickPadding;
		int volumeTotalWidth = volumeTickWidth * ( NumVolumeTics - 1 );

		{
			// transparent black background
            VPosf backgroundPose( VQuatf(), menuOffset + V3Vectf( 0.0f, 0.0f, -0.02f ) );
			VRMenuSurfaceParms backgroundSurfaceParms( "background",
												 backgroundTexture, backgroundWidth, backgroundHeight, SURFACE_TEXTURE_DIFFUSE,
												 0, 0, 0, SURFACE_TEXTURE_MAX,
												 0, 0, 0, SURFACE_TEXTURE_MAX );

			VRMenuObjectParms backgroundParms( VRMENU_BUTTON, VArray< VRMenuComponent* >(), backgroundSurfaceParms, NULL,
                                         backgroundPose, V3Vectf( 1.0f ), fontParms,
										 OvrVolumePopup::ID_BACKGROUND, VRMenuObjectFlags_t( VRMENUOBJECT_BOUND_ALL ) | VRMENUOBJECT_DONT_HIT_TEXT,
										 VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );
			defaultAppMenuItems.append( backgroundParms );

			// speaker icon
			VRMenuSurfaceParms speakerIconSurfaceParms( "speakerIcon",
												volumeIconTexture, volumeIconWidth, volumeIconHeight, SURFACE_TEXTURE_DIFFUSE,
												 0, 0, 0, SURFACE_TEXTURE_MAX,
												 0, 0, 0, SURFACE_TEXTURE_MAX );

			float speakerIconX = ( backgroundWidth * -0.5f + volumeIconWidth * 0.5f ) * VRMenuObject::DEFAULT_TEXEL_SCALE;
            V3Vectf speakerIconOffset = menuOffset + right * speakerIconX;
            VPosf speakerIconPose( VQuatf(), speakerIconOffset );
			VRMenuObjectParms speakerIconParms( VRMENU_BUTTON, VArray< VRMenuComponent* >(), speakerIconSurfaceParms, NULL,
                                         speakerIconPose, V3Vectf( 1.0f ), fontParms,
										 OvrVolumePopup::ID_VOLUME_ICON, VRMenuObjectFlags_t( VRMENUOBJECT_BOUND_ALL ) | VRMENUOBJECT_DONT_HIT_TEXT,
										 VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );
			defaultAppMenuItems.append( speakerIconParms );

			// volume ticks
			VRMenuSurfaceParms volumeTickSurfaceParms( "volumeTick",
												volumeTickOffTexture, volumeTickOffWidth, volumeTickOffHeight, SURFACE_TEXTURE_DIFFUSE,
												volumeTickOnTexture, volumeTickOnWidth, volumeTickOnHeight, SURFACE_TEXTURE_ADDITIVE,
												 0, 0, 0, SURFACE_TEXTURE_MAX );

			for( int i = 0; i < NumVolumeTics; i++ )
			{
                V3Vectf volumeTickOffset = menuOffset + right * ( volumeTotalWidth * -0.5f + i * volumeTickWidth ) * VRMenuObject::DEFAULT_TEXEL_SCALE + V3Vectf( 0.0f, 0.0f, 0.0025f * ( float )i );

                VPosf volumeTickPose( VQuatf(), volumeTickOffset );

				VRMenuObjectParms volumeTickParms( VRMENU_BUTTON, VArray< VRMenuComponent* >(), volumeTickSurfaceParms, NULL,
                                             volumeTickPose, V3Vectf( 1.0f ), fontParms,
											 VRMenuId_t( OvrVolumePopup::ID_VOLUME_TICKS.Get() + i ), VRMenuObjectFlags_t( VRMENUOBJECT_BOUND_ALL ) | VRMENUOBJECT_DONT_HIT_TEXT,
											 VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

				defaultAppMenuItems.append( volumeTickParms );
			}

			// volume text
			float volumeTextWidth = volumeIconWidth;
            menu->m_volumeTextOffset = menuOffset + right * ( backgroundWidth * 0.5f - volumeTextWidth * 0.5f ) * VRMenuObject::DEFAULT_TEXEL_SCALE + V3Vectf( 0.0f, 0.5f * VRMenuObject::DEFAULT_TEXEL_SCALE, 0.02f );
            VPosf volumeTextPose( VQuatf(), menu->m_volumeTextOffset );
			VRMenuObjectParms volumeTextParms( VRMENU_BUTTON, VArray< VRMenuComponent* >(), VRMenuSurfaceParms(), "0",
                                         volumeTextPose, V3Vectf( 1.0f ), fontParms,
										 OvrVolumePopup::ID_VOLUME_TEXT, VRMenuObjectFlags_t( VRMENUOBJECT_BOUND_ALL ) | VRMENUOBJECT_DONT_HIT_TEXT,
										 VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

			defaultAppMenuItems.append( volumeTextParms );
		}
	}

	VArray< VRMenuObjectParms const * > parms;

	// add all of the default items
	for ( int i = 0; i < defaultAppMenuItems.length(); ++i )
	{
		VRMenuObjectParms * defaultParms = new VRMenuObjectParms( defaultAppMenuItems[i] );
		parms.append( defaultParms );
	}

	menu->initWithItems( menuMgr, font, 1.8f, VRMenuFlags_t( VRMENU_FLAG_TRACK_GAZE ) | VRMENU_FLAG_SHORT_PRESS_HANDLED_BY_APP, parms );
	app->guiSys().addMenu( menu );

    DeletePointerArray( parms );

    return menu;
}

//==============================
// OvrVolumePopup::Frame_Impl
void OvrVolumePopup::frameImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, BitmapFont const & font,
        BitmapFontSurface & fontSurface, gazeCursorUserId_t const gazeUserId )
{
    if ( curMenuState() == VRMenu::MENUSTATE_CLOSED )
    {
        return;
    }

    const double timeSinceLastVolumeChange = ovr_GetTimeSinceLastVolumeChange();
	if ( ( timeSinceLastVolumeChange < 0 ) || ( timeSinceLastVolumeChange > VolumeMenuFadeDelay ) )
	{
		menuHandle_t volumeTextHandle = handleForId( app->vrMenuMgr(), OvrVolumePopup::ID_VOLUME_TEXT );
		VRMenuObject *volumeText = app->vrMenuMgr().toObject( volumeTextHandle );
		volumeText->addFlags( VRMENUOBJECT_DONT_RENDER );
		app->guiSys().closeMenu( app, this, false );
	}
}

//==============================
// OvrVolumePopup::
bool OvrVolumePopup::onKeyEventImpl( App * app, int const keyCode, KeyState::eKeyEventType const eventType )
{
	return false;
}

//==============================
// OvrVolumePopup::ShowVolume
void OvrVolumePopup::showVolume( App * app, const int current )
{
	if ( m_currentVolume != current )
	{
		// highlight the volume ticks
		for( int i = 0; i < NumVolumeTics; i++ )
		{
			menuHandle_t volumeTickHandle = handleForId( app->vrMenuMgr(), VRMenuId_t( OvrVolumePopup::ID_VOLUME_TICKS.Get() + i ) );
			VRMenuObject *volumeTick = app->vrMenuMgr().toObject( volumeTickHandle );
			volumeTick->setHilighted( i < current );
		}

		// update volume text
		menuHandle_t volumeTextHandle = handleForId( app->vrMenuMgr(), OvrVolumePopup::ID_VOLUME_TEXT );
		VRMenuObject *volumeText = app->vrMenuMgr().toObject( volumeTextHandle );
        volumeText->setText(VString::number(current));

		// center the text
        VBoxf bnds = volumeText->setTextLocalBounds( app->defaultFont() );
        volumeText->setLocalPosition( m_volumeTextOffset - V3Vectf( bnds.GetSize().x * 0.5f, 0.0f, 0.0f ) );

		m_currentVolume = current;
	}

	if ( curMenuState() == VRMenu::MENUSTATE_CLOSED )
    {
		menuHandle_t volumeTextHandle = handleForId( app->vrMenuMgr(), OvrVolumePopup::ID_VOLUME_TEXT );
		VRMenuObject *volumeText = app->vrMenuMgr().toObject( volumeTextHandle );
		volumeText->removeFlags( VRMENUOBJECT_DONT_RENDER );
    	app->guiSys().openMenu( app, app->gazeCursor(), "Volume" );
    }
}

//==============================
// OvrVolumePopup::CheckForVolumeChange
void OvrVolumePopup::checkForVolumeChange( App * app )
{
	// ovr_GetTimeSinceLastVolumeChange() will return -1 if the volume listener hasn't initialized yet,
	// which sometimes takes place after a frame has run in Unity.
	double timeSinceLastVolumeChange = ovr_GetTimeSinceLastVolumeChange();
	if ( ( timeSinceLastVolumeChange != -1 ) && ( timeSinceLastVolumeChange < VolumeMenuFadeDelay ) )
	{
		showVolume( app, ovr_GetVolume() );
	}
}

//==============================
// OvrVolumePopup::Close
void OvrVolumePopup::close( App * app )
{
	menuHandle_t volumeTextHandle = handleForId( app->vrMenuMgr(), OvrVolumePopup::ID_VOLUME_TEXT );
	VRMenuObject *volumeText = app->vrMenuMgr().toObject( volumeTextHandle );
	volumeText->addFlags( VRMENUOBJECT_DONT_RENDER );
	app->guiSys().closeMenu( app, this, false );
}

} // namespace NervGear
