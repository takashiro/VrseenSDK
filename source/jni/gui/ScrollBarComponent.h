/************************************************************************************

Filename    :   ScrollBarComponent.h
Content     :   A reusable component implementing a scroll bar.
Created     :   Jan 15, 2014
Authors     :   Warsam Osman

Copyright   :   Copyright 2014 Oculus VR, Inc. All Rights reserved.


*************************************************************************************/

#if !defined( OVR_ScrollBarComponent_h )
#define OVR_ScrollBarComponent_h

#include "VRMenuComponent.h"
#include "Fader.h"

namespace NervGear {

class VRMenu;
class App;

//==============================================================
// OvrSliderComponent
class OvrScrollBarComponent : public VRMenuComponent
{
public:
	enum eScrollBarState
	{
		SCROLL_STATE_NONE,
		SCROLL_STATE_FADE_IN,
		SCROLL_STATE_VISIBLE,
		SCROLL_STATE_FADE_OUT,
		SCROLL_STATE_HIDDEN,
		NUM_SCROLL_STATES
	};

	static const char * TYPE_NAME;

	char const *		typeName() const { return TYPE_NAME; }

	// Get the scrollbar parms and the pointer to the scrollbar component constructed
    static	void		getScrollBarParms( VRMenu & menu, float scrollBarWidth, const VRMenuId_t parentId, const VRMenuId_t rootId, const VRMenuId_t xformId,
									const VRMenuId_t baseId, const VRMenuId_t thumbId, const Posef & rootLocalPose, const Posef & xformPose, const int startElementIndex, 
									const int numElements, const bool verticalBar, const Vector4f & thumbBorder, Array< const VRMenuObjectParms* > & parms );
    void				updateScrollBar( OvrVRMenuMgr & menuMgr, VRMenuObject * self, const int numElements );
    void				setScrollFrac( OvrVRMenuMgr & menuMgr, VRMenuObject * self, const float frac );
    void				setScrollState( VRMenuObject * self, const eScrollBarState state );
    void				setBaseColor( OvrVRMenuMgr & menuMgr, VRMenuObject * self, const Vector4f & color );
    void				setScrollBarBaseWidth( float width ) { m_scrollBarBaseWidth = width; }
    void				setScrollBarBaseHeight( float height ) { m_scrollBarBaseHeight = height; }
    void				setScrollBarThumbWidth( float width ) { m_scrollBarThumbWidth = width; }
    void				setScrollBarThumbHeight( float height ) { m_scrollBarThumbHeight = height; }
    void				setVertical( bool value ) { m_isVertical = value; }
private:
	OvrScrollBarComponent( const VRMenuId_t rootId, const VRMenuId_t baseId,
		const VRMenuId_t thumbId, const int startElementIndex, const int numElements );

	virtual eMsgStatus  onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, 
								VRMenuObject * self, VRMenuEvent const & event );

    eMsgStatus			onInit( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
								VRMenuObject * self, VRMenuEvent const & event );

    eMsgStatus			onFrameUpdate( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
								VRMenuObject * self, VRMenuEvent const & event );

    SineFader			m_fader;				// Used to fade the scroll bar in and out of view
    const float			m_fadeInRate;
    const float			m_fadeOutRate;

    int					m_numOfItems;
    float 				m_scrollBarBaseWidth;
    float 				m_scrollBarBaseHeight;
    float				m_scrollBarCurrentbaseLength;
    float 				m_scrollBarThumbWidth;
    float 				m_scrollBarThumbHeight;
    float				m_scrollBarCurrentThumbLength;

    VRMenuId_t			m_scrollRootId;		// Id to the root object which holds the base and thumb
    VRMenuId_t			m_scrollBarBaseId;	// Id to get handle of the scrollbar base
    VRMenuId_t			m_scrollBarThumbId;	// Id to get the handle of the scrollbar thumb

    eScrollBarState		m_currentScrollState;
    bool				m_isVertical;
};

} // namespace NervGear

#endif // OVR_ScrollBarComponent_h
