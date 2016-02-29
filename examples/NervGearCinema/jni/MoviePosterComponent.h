
#if !defined( MoviePosterComponent_h )
#define MoviePosterComponent_h

#include "CarouselBrowserComponent.h"

using namespace NervGear;

namespace OculusCinema {

class UIContainer;
class UIImage;
class UILabel;

//==============================================================
// MoviePosterComponent
class MoviePosterComponent : public CarouselItemComponent
{
public:
							MoviePosterComponent();

	static bool 			ShowShadows;

	void 					SetMenuObjects( const int width, const int height, UIContainer * poster, UIImage * posterImage, UIImage * is3DIcon, UIImage * shadow );
	virtual void 			SetItem( VRMenuObject * self, const CarouselItem * item, const PanelPose &pose );

private:
    eMsgStatus onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event ) override;

    const CarouselItem * 	CurrentItem;

    int						Width;
    int						Height;

    UIContainer * 			Poster;
    UIImage * 				PosterImage;
    UIImage * 				Is3DIcon;
    UIImage * 				Shadow;
};

} // namespace OculusCinema

#endif // MoviePosterComponent_h
