/************************************************************************************

Filename    :   OvrTextFade_Component.h
Content     :   A reusable component that fades text in and recenters it on gaze over.
Created     :   July 25, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#if !defined( OVR_TextFade_Component_h )
#define OVR_TextFade_Component_h

#include "VRMenuComponent.h"
#include "Fader.h"

NV_NAMESPACE_BEGIN

//==============================================================
// OvrTextFade_Component
class OvrTextFade_Component : public VRMenuComponent
{
public:
	static double const FADE_DELAY;
	static float const  FADE_DURATION;

	OvrTextFade_Component( Vector3f const & iconBaseOffset, Vector3f const & iconFadeOffset );

	virtual eMsgStatus      onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
										  VRMenuObject * self, VRMenuEvent const & event );

    eMsgStatus frame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event );

    eMsgStatus focusGained( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event );

    eMsgStatus focusLost( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event );

    static Vector3f CalcIconFadeOffset(const VString &text, BitmapFont const & font, Vector3f const & axis, float const iconWidth );


private:
    SineFader   m_textAlphaFader;
    double      m_startFadeInTime;    // when focus is gained, this is set to the time when the fade in should begin
    double      m_startFadeOutTime;   // when focus is lost, this is set to the time when the fade out should begin
    SineFader   m_iconOffsetFader;
    Vector3f    m_iconBaseOffset;     // base offset for text
    Vector3f    m_iconFadeOffset;     // text offset when fully faded
};

NV_NAMESPACE_END

#endif // OVR_TextFade_Component_h
