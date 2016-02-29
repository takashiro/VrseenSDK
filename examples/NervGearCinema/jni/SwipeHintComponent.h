#if !defined( SwipeHintComponent_h )
#define SwipeHintComponent_h

#include <VRMenuComponent.h>
#include "Lerp.h"

using namespace NervGear;

namespace OculusCinema {

class CarouselBrowserComponent;

//==============================================================
// SwipeHintComponent
class SwipeHintComponent : public VRMenuComponent
{
public:
	static const char *			TYPE_NAME;

	static bool					ShowSwipeHints;

	SwipeHintComponent( CarouselBrowserComponent *carousel, const bool isRightSwipe, const float totalTime, const float timeOffset, const float delay );

    const char *typeName( ) const override { return TYPE_NAME; }

	void						Reset( VRMenuObject * self );

private:
    CarouselBrowserComponent *	Carousel;
    bool 						IsRightSwipe;
    float 						TotalTime;
    float						TimeOffset;
    float 						Delay;
    double 						StartTime;
    bool						ShouldShow;
    bool						IgnoreDelay;
    Lerp						TotalAlpha;

private:
    bool 						CanSwipe() const;
    void 						Show( const double now );
    void 						Hide( const double now );
    eMsgStatus      	onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event ) override;
    eMsgStatus              	Opening( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event );
    eMsgStatus              	Frame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event );
};

} // namespace OculusCinema

#endif // SwipeHintComponent_h
