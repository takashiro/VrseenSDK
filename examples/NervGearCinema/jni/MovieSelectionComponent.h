#if !defined( MovieSelectionComponent_h )
#define MovieSelectionComponent_h

#include <VRMenuComponent.h>

using namespace NervGear;

namespace OculusCinema {

class MovieSelectionView;

//==============================================================
// MovieSelectionComponent
class MovieSelectionComponent : public VRMenuComponent
{
public:
							MovieSelectionComponent( MovieSelectionView *view );

private:
    SoundLimiter			Sound;
    MovieSelectionView *	CallbackView;

private:
    eMsgStatus onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event ) override;

    eMsgStatus              Frame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event );
    eMsgStatus              FocusGained( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event );
    eMsgStatus              FocusLost( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event );
};

} // namespace OculusCinema

#endif // MovieSelectionComponent_h
