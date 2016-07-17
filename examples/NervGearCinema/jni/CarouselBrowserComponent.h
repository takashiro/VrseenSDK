#pragma once

#include "gui/VRMenu.h"
#include "gui/VRMenuComponent.h"

using namespace NervGear;

namespace OculusCinema {

class CarouselItem
{
public:
	VString		name;
	GLuint		texture;
	int			textureWidth;
	int			textureHeight;
	uint		userFlags;

				CarouselItem() : texture( 0 ), textureWidth( 0 ), textureHeight( 0 ), userFlags( 0 ) {}
};

class PanelPose
{
public:
    VQuatf    	Orientation;
    V3Vectf 	Position;
    V4Vectf	Color;

				PanelPose() {};
                PanelPose( VQuatf orientation, V3Vectf position, V4Vectf color ) :
					Orientation( orientation ), Position( position ), Color( color ) {}
};

class CarouselItemComponent : public VRMenuComponent
{
public:
	explicit						CarouselItemComponent( VRMenuEventFlags_t const & eventFlags ) :
										VRMenuComponent( eventFlags )
									{
									}

	virtual							~CarouselItemComponent() { }

	virtual void 					SetItem( VRMenuObject * self, const CarouselItem * item, const PanelPose &pose ) = 0;
};

class CarouselBrowserComponent : public VRMenuComponent
{
public:
									CarouselBrowserComponent( const VArray<CarouselItem *> &items, const VArray<PanelPose> &panelPoses );

	void							SetPanelPoses( OvrVRMenuMgr & menuMgr, VRMenuObject * self, const VArray<PanelPose> &panelPoses );
	void 							SetMenuObjects( const VArray<VRMenuObject *> &menuObjs, const VArray<CarouselItemComponent *> &menuComps );
	void							SetItems( const VArray<CarouselItem *> &items );
	void							SetSelectionIndex( const int selectedIndex );
    int 							GetSelection() const;
	bool							HasSelection() const;
	bool							IsSwiping() const { return Swiping; }
	bool							CanSwipeBack() const;
	bool							CanSwipeForward() const;

	void 							CheckGamepad( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self );

private:
    eMsgStatus onEventImpl( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event ) override;
    PanelPose 						GetPosition( const float t );
    void 							UpdatePanels( OvrVRMenuMgr & menuMgr, VRMenuObject * self );

    eMsgStatus 						Frame( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event );
    eMsgStatus 						SwipeForward( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self );
    eMsgStatus 						SwipeBack( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self );
	eMsgStatus 						TouchDown( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event );
	eMsgStatus 						TouchUp( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event );
	eMsgStatus 						Opened( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event );
	eMsgStatus 						Closed( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event );

public:
    bool							SelectPressed;

private:
    V3Vectf						PositionScale;
    float							Position;
	double							TouchDownTime;			// the time in second when a down even was received, < 0 if touch is not down

	int 							ItemWidth;
    int 							ItemHeight;

    VArray<CarouselItem *> 			Items;
    VArray<VRMenuObject *> 			MenuObjs;
    VArray<CarouselItemComponent *> 	MenuComps;
	VArray<PanelPose>				PanelPoses;

	double 							StartTime;
	double 							EndTime;
	float							PrevPosition;
	float							NextPosition;

	bool							Swiping;
	bool							PanelsNeedUpdate;
};

} // namespace OculusCinema

