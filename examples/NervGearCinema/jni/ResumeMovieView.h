#if !defined( ResumeMovieView_h )
#define ResumeMovieView_h

#include <gui/VRMenu.h>

#include "View.h"

namespace OculusCinema {

class CinemaApp;

class ResumeMovieView : public View
{
public:
						ResumeMovieView( CinemaApp &cinema );
	virtual 			~ResumeMovieView();

	virtual void 		OneTimeInit( const char * launchIntent );
	virtual void		OneTimeShutdown();

	virtual void 		OnOpen();
	virtual void 		OnClose();

	virtual bool 		Command( const char * msg );
	virtual bool 		OnKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType );

	virtual Matrix4f 	DrawEyeView( const int eye, const float fovDegrees );
	virtual Matrix4f 	Frame( const VrFrame & vrFrame );

	void 				ResumeChoice( int itemNum );

private:
	CinemaApp &			Cinema;

	static VRMenuId_t  	ID_CENTER_ROOT;
	static VRMenuId_t  	ID_TITLE;
	static VRMenuId_t  	ID_OPTIONS;
	static VRMenuId_t  	ID_OPTION_ICONS;

	VRMenu *			Menu;

private:
    void				SetPosition( OvrVRMenuMgr & menuMgr, const Vector3f &pos );
    void 				CreateMenu( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font );
};

} // namespace OculusCinema

#endif // ResumeMovieView_h