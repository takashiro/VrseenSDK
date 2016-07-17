#include <VRMenuMgr.h>

#include "UI/UIContainer.h"
#include "UI/UIMenu.h"
#include "CinemaApp.h"

namespace OculusCinema {

UIContainer::UIContainer( CinemaApp &cinema ) :
	UIWidget( cinema )

{
}

UIContainer::~UIContainer()
{
}

void UIContainer::AddToMenu( UIMenu *menu, UIWidget *parent )
{
    const VPosf pose( VQuatf( V3Vectf( 0.0f, 1.0f, 0.0f ), 0.0f ), V3Vectf( 0.0f, 0.0f, 0.0f ) );

    V3Vectf defaultScale( 1.0f );
	VRMenuFontParms fontParms( true, true, false, false, false, 1.0f );
	
	VRMenuObjectParms parms( VRMENU_CONTAINER, VArray< VRMenuComponent* >(), VRMenuSurfaceParms(),
			"Container", pose, defaultScale, fontParms, menu->AllocId(),
			VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

	AddToMenuWithParms( menu, parent, parms );
}

} // namespace OculusCinema
