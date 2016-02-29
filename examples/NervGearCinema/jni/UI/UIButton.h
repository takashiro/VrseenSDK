#if !defined( UIButton_h )
#define UIButton_h

#include <gui/VRMenu.h>
#include <gui/VRMenuComponent.h>

#include "UI/UIWidget.h"
#include "UI/UITexture.h"

using namespace NervGear;

namespace OculusCinema {

class CinemaApp;
class UIButton;

//==============================================================
// UIButtonComponent
class UIButtonComponent : public VRMenuComponent
{
public:
	static const int TYPE_ID = 159493;

					UIButtonComponent( UIButton &button );

    int	typeId() const override { return TYPE_ID; }

	bool			IsPressed() const { return TouchDown; }

private:
	UIButton &		Button;

    SoundLimiter    GazeOverSoundLimiter;
    SoundLimiter    DownSoundLimiter;
    SoundLimiter    UpSoundLimiter;

    bool			TouchDown;

private:
    eMsgStatus onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event ) override;

    eMsgStatus FocusGained( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event );
    eMsgStatus FocusLost( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event );
};

//==============================================================
// UIButton

class UIButton : public UIWidget
{
	friend class UIButtonComponent;

public:
										UIButton( CinemaApp &cinema );
										~UIButton();

	void 								AddToMenu( UIMenu *menu, UIWidget *parent = NULL );

	void								SetButtonImages( const UITexture &normal, const UITexture &hover, const UITexture &pressed );

	void								SetOnClick( void ( *callback )( UIButton *, void * ), void *object );

	void								UpdateButtonState();

private:
	UIButtonComponent					ButtonComponent;
	UITexture 							Normal;
	UITexture 							Hover;
	UITexture 							Pressed;

	void 								( *OnClickFunction )( UIButton *button, void *object );
	void *								OnClickObject;

	void 								OnClick();

};

} // namespace OculusCinema

#endif // UIButton_h
