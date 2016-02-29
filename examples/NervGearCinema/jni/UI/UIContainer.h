#if !defined( UIContainer_h )
#define UIContainer_h

#include <gui/VRMenu.h>

#include "UI/UIWidget.h"

using namespace NervGear;

namespace OculusCinema {

class CinemaApp;

class UIContainer : public UIWidget
{
public:
										UIContainer( CinemaApp &cinema );
										~UIContainer();

	void 								AddToMenu( UIMenu *menu, UIWidget *parent = NULL );
};

} // namespace OculusCinema

#endif // UIContainer_h
