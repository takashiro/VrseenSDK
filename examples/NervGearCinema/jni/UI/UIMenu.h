#if !defined( UIMenu_h )
#define UIMenu_h

#include "gui/VRMenu.h"

using namespace NervGear;

namespace OculusCinema {

class CinemaApp;

class UIMenu
{
public:
										UIMenu( CinemaApp &cinema );
										~UIMenu();

	VRMenuId_t 							AllocId();

	void 								Create( const char *menuName );

	void 								Open();
	void 								Close();

	bool								IsOpen() const { return MenuOpen; }

	VRMenu *							GetVRMenu() const { return Menu; }

    VRMenuFlags_t const &				GetFlags() const;
	void								SetFlags( VRMenuFlags_t	const & flags );

private:
    CinemaApp &							Cinema;
    String								MenuName;
	VRMenu *							Menu;

	bool								MenuOpen;

	VRMenuId_t							IdPool;
};

} // namespace OculusCinema

#endif // UIMenu_h
