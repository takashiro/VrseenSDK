/************************************************************************************

Filename    :   MovieCategoryComponent.h
Content     :   Menu component for the movie category menu.
Created     :   August 13, 2014
Authors     :   Jim Dos�

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "gui/VRMenuComponent.h"
#include "MovieManager.h"

#if !defined( MovieCategoryComponent_h )
#define MovieCategoryComponent_h

using namespace NervGear;

namespace OculusCinema {

class MovieSelectionView;

//==============================================================
// MovieCategoryComponent
class MovieCategoryComponent : public VRMenuComponent
{
public:
							MovieCategoryComponent( MovieSelectionView *view, MovieCategory category );

    static const V4Vectf	HighlightColor;
    static const V4Vectf	FocusColor;
    static const V4Vectf	NormalColor;

private:
    SoundLimiter			Sound;

	bool					HasFocus;

	MovieCategory			Category;

    MovieSelectionView *	CallbackView;

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

#endif // MovieCategoryComponent_h
