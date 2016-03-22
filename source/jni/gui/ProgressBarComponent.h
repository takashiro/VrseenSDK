/************************************************************************************

Filename    :   ProgressBarComponent.h
Content     :   A reusable component implementing a progress bar.
Created     :   Mar 30, 2015
Authors     :   Warsam Osman

Copyright   :   Copyright 2015 Oculus VR, Inc. All Rights reserved.


*************************************************************************************/

#if !defined( OVR_ProgressBarComponent_h )
#define OVR_ProgressBarComponent_h

#include "VRMenuComponent.h"
#include "Fader.h"

NV_NAMESPACE_BEGIN

class VRMenu;
class App;

//==============================================================
// OvrProgressBarComponent
class OvrProgressBarComponent : public VRMenuComponent
{
public:
	enum eProgressBarState
	{
		PROGRESSBAR_STATE_NONE,
		PROGRESSBAR_STATE_FADE_IN,
		PROGRESSBAR_STATE_VISIBLE,
		PROGRESSBAR_STATE_FADE_OUT,
		PROGRESSBAR_STATE_HIDDEN,
		PROGRESSBAR_STATES_COUNT
	};

	static const char * TYPE_NAME;

	char const *		typeName() const { return TYPE_NAME; }

	// Get the scrollbar parms and the pointer to the scrollbar component constructed
    static	void		getProgressBarParms( VRMenu & menu, const int width, const int height, const VRMenuId_t parentId,
									const VRMenuId_t rootId, const VRMenuId_t xformId, const VRMenuId_t baseId,
									const VRMenuId_t thumbId, const VRMenuId_t animId,
                                    const VPosf & rootLocalPose, const VPosf & xformPose,
									const char * baseImage, const char * barImage, const char * animImage,
									VArray< const VRMenuObjectParms* > & outParms );

    void				setProgressFrac( OvrVRMenuMgr & menuMgr, VRMenuObject * self, const float frac );
    void				setProgressbarState( VRMenuObject * self, const eProgressBarState state );
    void				oneTimeInit( OvrVRMenuMgr & menuMgr, VRMenuObject * self, const V4Vectf & color );
    void				setProgressBarBaseWidth( int width ) { m_progressBarBaseWidth = static_cast< float >( width ); }
    void				setProgressBarBaseHeight( int height ) { m_progressBarBaseHeight = static_cast< float >( height ); }
    void				setProgressBarThumbWidth( int width ) { m_progressBarThumbWidth = static_cast< float >( width ); }
    void				setProgressBarThumbHeight( int height ) { m_progressBarThumbHeight = static_cast< float >( height ); }

private:
	OvrProgressBarComponent( const VRMenuId_t rootId, const VRMenuId_t baseId, const VRMenuId_t thumbId, const VRMenuId_t animId );

	virtual eMsgStatus  onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, 
								VRMenuObject * self, VRMenuEvent const & event );

    eMsgStatus			onInit( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
								VRMenuObject * self, VRMenuEvent const & event );

    eMsgStatus			onFrameUpdate( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
								VRMenuObject * self, VRMenuEvent const & event );

    SineFader			m_fader;				// Used to fade the scroll bar in and out of view
    const float			m_fadeInRate;
    const float			m_fadeOutRate;

    float				m_frac;

    float 				m_progressBarBaseWidth;
    float 				m_progressBarBaseHeight;
    float				m_progressBarCurrentbaseLength;
    float 				m_progressBarThumbWidth;
    float 				m_progressBarThumbHeight;
    float				m_progressBarCurrentThumbLength;

    VRMenuId_t			m_progressBarRootId;		// Id to the root object which holds the base and thumb
    VRMenuId_t			m_progressBarBaseId;		// Id to get handle of the base
    VRMenuId_t			m_progressBarThumbId;		// Id to get the handle of the thumb
    VRMenuId_t			m_progressBarAnimId;		// Id to get the handle of the progress animation

    eProgressBarState	m_currentProgressBarState;
};

NV_NAMESPACE_END

#endif // OVR_ProgressBarComponent_h
