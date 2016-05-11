#include <VRMenuMgr.h>

#include "UI/UIMenu.h"
#include "CinemaApp.h"

namespace OculusCinema {

UIMenu::UIMenu( CinemaApp &cinema ) :
	Cinema( cinema ),
	MenuName(),
	Menu( NULL ),
	MenuOpen( false ),
	IdPool( 1 )

{
	// This is called at library load time, so the system is not initialized
	// properly yet.
}

UIMenu::~UIMenu()
{
	//DeletePointerArray( MovieBrowserItems );
}

VRMenuId_t UIMenu::AllocId()
{
	VRMenuId_t id = IdPool;
	IdPool = VRMenuId_t( IdPool.value() + 1 );

	return id;
}

void UIMenu::Open()
{
	vInfo("Open");
    vApp->guiSys().openMenu(vApp, vApp->gazeCursor(), MenuName);
	MenuOpen = true;
}

void UIMenu::Close()
{
	vInfo("Close");
    vApp->guiSys().closeMenu(vApp, Menu, false);
	MenuOpen = false;
}

//=======================================================================================

void UIMenu::Create( const char *menuName )
{
	MenuName = menuName;
	Menu = VRMenu::Create( menuName );
    Menu->init( vApp->vrMenuMgr(), vApp->defaultFont(), 0.0f, VRMenuFlags_t() );
    vApp->guiSys().addMenu( Menu );
}

VRMenuFlags_t const & UIMenu::GetFlags() const
{
    return Menu->flags();
}

void UIMenu::SetFlags( VRMenuFlags_t const & flags )
{
    Menu->setFlags( flags );
}

} // namespace OculusCinema
