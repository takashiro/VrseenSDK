/************************************************************************************

Filename    :   OvrGuiSys.cpp
Content     :   Manager for native GUIs.
Created     :   June 6, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#include "GuiSysLocal.h"

#include <android/keycodes.h>
#include "api/VGlOperation.h"
#include "../api/VGlShader.h"
#include "GlTexture.h"
#include "../api/VGlGeometry.h"
#include "../core/VTimer.h"
#include "App.h"
#include "GazeCursor.h"
#include "VRMenuMgr.h"
#include "VRMenuComponent.h"
#include "SoundLimiter.h"
#include "VRMenuEventHandler.h"
#include "FolderBrowser.h"
#include "Input.h"
#include "DefaultComponent.h"

namespace NervGear {

V4Vectf const OvrGuiSys::BUTTON_DEFAULT_TEXT_COLOR( 0.098f, 0.6f, 0.96f, 1.0f );
V4Vectf const OvrGuiSys::BUTTON_HILIGHT_TEXT_COLOR( 1.0f );

//==============================
// OvrGuiSysLocal::
OvrGuiSysLocal::OvrGuiSysLocal() :
	IsInitialized( false )
{
}

//==============================
// OvrGuiSysLocal::
OvrGuiSysLocal::~OvrGuiSysLocal()
{
	vAssert( IsInitialized == false ); // Shutdown should already have been called
}

//==============================
// OvrGuiSysLocal::Init
// FIXME: the default items only apply to AppMenu now, and should be moved there once AppMenu overloads VRMenu.
void OvrGuiSysLocal::init( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font )
{
	vInfo("OvrGuiSysLocal::Init");

	// get a use id for the gaze cursor
	GazeUserId = app->gazeCursor().GenerateUserId();

	IsInitialized = true;
}

//==============================
// OvrGuiSysLocal::RepositionMenus
// Reposition any open menus
void OvrGuiSysLocal::resetMenuOrientations( App * app, VR4Matrixf const & viewMatrix )
{
	for ( int i = 0; i < Menus.length(); ++i )
	{
		if ( VRMenu* menu = Menus.at( i ) )
		{
			vInfo("ResetMenuOrientation -> '" << menu->name( ) << "'");
			menu->resetMenuOrientation( app, viewMatrix );
		}
	}
}

//==============================
// OvrGuiSysLocal::AddMenu
void OvrGuiSysLocal::addMenu( VRMenu * menu )
{
	int menuIndex = FindMenuIndex( menu->name() );
	if ( menuIndex >= 0 )
	{
		vWarn("Duplicate menu name '" << menu->name() << "'");
		vAssert( menuIndex < 0 );
	}
    Menus.append( menu );
}

//==============================
// OvrGuiSysLocal::GetMenu
VRMenu * OvrGuiSysLocal::getMenu( char const * menuName ) const
{
	int menuIndex = FindMenuIndex( menuName );
	if ( menuIndex >= 0 )
	{
		return Menus[menuIndex];
	}
	return NULL;
}

//==============================
// OvrGuiSysLocal::DestroyMenu
void OvrGuiSysLocal::destroyMenu( OvrVRMenuMgr & menuMgr, VRMenu * menu )
{
	vAssert( menu != NULL );

	MakeInactive( menu );

	menu->shutdown( menuMgr );
	delete menu;

	int idx = FindMenuIndex( menu );
	if ( idx >= 0 )
	{
		Menus.removeAt( idx );
	}
}

//==============================
// OvrGuiSysLocal::Shutdown
void OvrGuiSysLocal::shutdown( OvrVRMenuMgr & menuMgr )
{
	// pointers in this list will always be in Menus list, too, so just clear it
	ActiveMenus.clear();

	// FIXME: we need to make sure we delete any child menus here -- it's not enough to just delete them
	// in the destructor of the parent, because they'll be left in the menu list since the destructor has
	// no way to call GuiSys->DestroyMenu() for them.
	for ( int i = 0; i < Menus.length(); ++i )
	{
		VRMenu * menu = Menus[i];
		menu->shutdown( menuMgr );
		delete menu;
		Menus[i] = NULL;
	}
	Menus.clear();

	IsInitialized = false;
}

//==============================
// OvrGuiSysLocal::FindMenuIndex
int OvrGuiSysLocal::FindMenuIndex( char const * menuName ) const
{
	for ( int i = 0; i < Menus.length(); ++i )
	{
        if ( strcasecmp(Menus[i]->name(), menuName ) == 0 )
		{
			return i;
		}
	}
	return -1;
}

//==============================
// OvrGuiSysLocal::FindMenuIndex
int OvrGuiSysLocal::FindMenuIndex( VRMenu const * menu ) const
{
	for ( int i = 0; i < Menus.length(); ++i )
	{
		if ( Menus[i] == menu )
		{
			return i;
		}
	}
	return -1;
}

//==============================
// OvrGuiSysLocal::FindActiveMenuIndex
int OvrGuiSysLocal::FindActiveMenuIndex( VRMenu const * menu ) const
{
	for ( int i = 0; i < ActiveMenus.length(); ++i )
	{
		if ( ActiveMenus[i] == menu )
		{
			return i;
		}
	}
	return -1;
}

//==============================
// OvrGuiSysLocal::FindActiveMenuIndex
int OvrGuiSysLocal::FindActiveMenuIndex( char const * menuName ) const
{
	for ( int i = 0; i < ActiveMenus.length(); ++i )
	{
        if ( strcasecmp( ActiveMenus[i]->name(), menuName ) == 0 )
		{
			return i;
		}
	}
	return -1;
}

//==============================
// OvrGuiSysLocal::MakeActive
void OvrGuiSysLocal::MakeActive( VRMenu * menu )
{
	int idx = FindActiveMenuIndex( menu );
	if ( idx < 0 )
	{
        ActiveMenus.append( menu );
	}
}

//==============================
// OvrGuiSysLocal::MakeInactive
void OvrGuiSysLocal::MakeInactive( VRMenu * menu )
{
	int idx = FindActiveMenuIndex( menu );
	if ( idx >= 0 )
	{
		ActiveMenus.removeAtUnordered( idx );
	}
}

//==============================
// OvrGuiSysLocal::OpenMenu
void OvrGuiSysLocal::openMenu( App * app, OvrGazeCursor & gazeCursor, char const * menuName )
{
	int menuIndex = FindMenuIndex( menuName );
	if ( menuIndex < 0 )
	{
		vWarn("No menu named '" << menuName << "'");
		vAssert( menuIndex >= 0 && menuIndex < Menus.length() );
		return;
	}
	VRMenu * menu = Menus[menuIndex];
	vAssert( menu != NULL );
	if ( !menu->isOpenOrOpening() )
	{
		menu->open( app, gazeCursor );
		MakeActive( menu );
	}
}

//==============================
// OvrGuiSysLocal::CloseMenu
void OvrGuiSysLocal::closeMenu( App * app, char const * menuName, bool const closeInstantly )
{
	int menuIndex = FindMenuIndex( menuName );
	if ( menuIndex < 0 )
	{
		vWarn("No menu named '" << menuName << "'");
		vAssert( menuIndex >= 0 && menuIndex < Menus.length() );
		return;
	}
	VRMenu * menu = Menus[menuIndex];
	closeMenu( app, menu, closeInstantly );
}

//==============================
// OvrGuiSysLocal::CloseMenu
void OvrGuiSysLocal::closeMenu( App * app, VRMenu * menu, bool const closeInstantly )
{
	vAssert( menu != NULL );
	if ( !menu->isClosedOrClosing() )
	{
		menu->close( app, app->gazeCursor(), closeInstantly );
	}
}

//==============================
// OvrGuiSysLocal::IsMenuActive
bool OvrGuiSysLocal::isMenuActive( char const * menuName ) const
{
	int idx = FindActiveMenuIndex( menuName );
	return idx >= 0;
}

//==============================
// OvrGuiSysLocal::IsAnyMenuOpen
bool OvrGuiSysLocal::isAnyMenuActive() const
{
	return ActiveMenus.length() > 0;
}

//==============================
// OvrGuiSysLocal::IsAnyMenuOpen
bool OvrGuiSysLocal::isAnyMenuOpen() const
{
	for ( int i = 0; i < ActiveMenus.length(); ++i )
	{
        if ( ActiveMenus[i]->isOpenOrOpening() )
		{
			return true;
		}
	}
	return false;
}

//==============================
// OvrGuiSysLocal::Frame
void OvrGuiSysLocal::frame( App * app, const VrFrame & vrFrame, OvrVRMenuMgr & menuMgr, BitmapFont const & font,
        BitmapFontSurface & fontSurface, VR4Matrixf const & viewMatrix )
{
	//vInfo("OvrGuiSysLocal::Frame");

	// go backwards through the list so we can use unordered remove when a menu finishes closing
	for ( int i = ActiveMenus.length() - 1; i >= 0; --i )
	{
		VRMenu * curMenu = ActiveMenus[i];
		vAssert( curMenu != NULL );

		// VRMenu::Frame() CPU performance test
		if ( 0 )
		{
			//SetCurrentThreadAffinityMask( 0xF0 );
            double start =VTimer::Seconds();
			for ( int i = 0; i < 20; i++ )
			{
				menuMgr.beginFrame();
				curMenu->frame( app, vrFrame, menuMgr, font, fontSurface, viewMatrix, GazeUserId );
			}
            double end = VTimer::Seconds();
			vInfo("20x VRMenu::Frame() =" << ( end - start ) * ( 1.0 / ( 1000.0 * 1000.0 ) ) << "ms");
		}

		curMenu->frame( app, vrFrame, menuMgr, font, fontSurface, viewMatrix, GazeUserId );

		if ( curMenu->curMenuState() == VRMenu::MENUSTATE_CLOSED )
		{
			// remove from the active list
			ActiveMenus.removeAtUnordered( i );
			continue;
		}
	}
}

//==============================
// OvrGuiSysLocal::OnKeyEvent
bool OvrGuiSysLocal::onKeyEvent( App * app, int const keyCode, KeyState::eKeyEventType const eventType )
{
	for ( int i = 0; i < ActiveMenus.length(); ++i )
	{
		VRMenu * curMenu = ActiveMenus[i];
		vAssert( curMenu != NULL );

		if ( keyCode == AKEYCODE_BACK )
		{
			vInfo("OvrGuiSysLocal back key event '" << KeyState::EventNames[eventType] << "' for menu '" << curMenu->name() << "'");
		}

		if ( curMenu->onKeyEvent( app, keyCode, eventType ) )
		{
			vInfo("VRMenu '" << curMenu->name() << "' consumed key event");
			return true;
		}
	}
	// we ignore other keys in the app menu for now
	return false;
}

} // namespace NervGear
