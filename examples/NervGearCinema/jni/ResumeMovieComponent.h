#include <VRMenuComponent.h>

#if !defined( ResumeMovieComponent_h )
#define ResumeMovieComponent_h

using namespace NervGear;

namespace OculusCinema {

class ResumeMovieView;

//==============================================================
// ResumeMovieComponent
class ResumeMovieComponent : public VRMenuComponent
{
public:
	ResumeMovieComponent( ResumeMovieView *view, int itemNum );

	VRMenuObject * 			Icon;

	static const Vector4f	HighlightColor;
	static const Vector4f	FocusColor;
	static const Vector4f	NormalColor;

private:
    SoundLimiter			Sound;

	bool					HasFocus;
    int						ItemNum;
    ResumeMovieView *		CallbackView;

private:
    eMsgStatus onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event ) override;

	void					UpdateColor( VRMenuObject * self );

    eMsgStatus              Frame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event );
    eMsgStatus              FocusGained( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event );
    eMsgStatus              FocusLost( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event );
};

} // namespace OculusCinema

#endif // ResumeMovieComponent_h
