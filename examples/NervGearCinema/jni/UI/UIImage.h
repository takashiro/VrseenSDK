#if !defined( UIImage_h )
#define UIImage_h

#include <gui/VRMenu.h>

#include "UI/UIWidget.h"

using namespace NervGear;

namespace OculusCinema {

class CinemaApp;

class UIImage : public UIWidget
{
public:
										UIImage( CinemaApp &cinema );
										~UIImage();

	void 								AddToMenu( UIMenu *menu, UIWidget *parent = NULL );
	void 								AddToMenuFlags( UIMenu *menu, UIWidget *parent, VRMenuObjectFlags_t const flags );
};

} // namespace OculusCinema

#endif // UIImage_h
