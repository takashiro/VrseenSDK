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

	virtual void 		OneTimeInit(const VString &launchIntent );
	virtual void		OneTimeShutdown();

	virtual void 		OnOpen();
	virtual void 		OnClose();

    bool Command(const VEvent &);
	virtual bool 		OnKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType );

    virtual VR4Matrixf 	DrawEyeView( const int eye, const float fovDegrees );
    virtual VR4Matrixf 	Frame( const VFrame & vrFrame );

	void 				ResumeChoice( int itemNum );

private:
	CinemaApp &			Cinema;

	static VRMenuId_t  	ID_CENTER_ROOT;
	static VRMenuId_t  	ID_TITLE;
	static VRMenuId_t  	ID_OPTIONS;
	static VRMenuId_t  	ID_OPTION_ICONS;

	VRMenu *			Menu;

private:
    void				SetPosition( OvrVRMenuMgr & menuMgr, const V3Vectf &pos );
    void 				CreateMenu( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font );
};

} // namespace OculusCinema

#endif // ResumeMovieView_h
