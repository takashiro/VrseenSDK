#if !defined( TheaterSelectionView_h )
#define TheaterSelectionView_h

#include <VArray.h>
#include <gui/GuiSys.h>

#include "View.h"

namespace OculusCinema {

class CinemaApp;
class CarouselBrowser;
class CarouselItem;

class TheaterSelectionView : public View
{
public:
								TheaterSelectionView( CinemaApp &cinema );
	virtual 					~TheaterSelectionView();

	virtual void 				OneTimeInit(const VString &launchIntent );
	virtual void				OneTimeShutdown();

	virtual void 				OnOpen();
	virtual void 				OnClose();

	virtual bool 				Command( const char * msg );
	virtual bool 				OnKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType );

	virtual Matrix4f 			DrawEyeView( const int eye, const float fovDegrees );
	virtual Matrix4f 			Frame( const VrFrame & vrFrame );

	void 						SelectPressed( void );
	void 						SelectTheater( int theater );

	int							GetSelectedTheater() const { return SelectedTheater; }

private:
	CinemaApp &					Cinema;

	static VRMenuId_t  			ID_CENTER_ROOT;
	static VRMenuId_t			ID_ICONS;
	static VRMenuId_t  			ID_TITLE_ROOT;
	static VRMenuId_t 			ID_SWIPE_ICON_LEFT;
	static VRMenuId_t 			ID_SWIPE_ICON_RIGHT;

	VRMenu *					Menu;
	VRMenuObject * 				CenterRoot;
	VRMenuObject * 				SelectionObject;

	CarouselBrowserComponent *	TheaterBrowser;
    VArray<CarouselItem *> 		Theaters;

	int							SelectedTheater;

	double						IgnoreSelectTime;

private:
    void						SetPosition( OvrVRMenuMgr & menuMgr, const Vector3f &pos );
    void 						CreateMenu( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font );
};

} // namespace OculusCinema

#endif // TheaterSelectionView_h
