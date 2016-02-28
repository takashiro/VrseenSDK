/************************************************************************************

Filename    :   DefaultComponent.h
Content     :   A default menu component that handles basic actions most menu items need.
Created     :   July 25, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#if !defined( Ovr_DefaultComponent_h )
#define Ovr_DefaultComponent_h

#include "VRMenuComponent.h"
#include "Fader.h"

namespace NervGear {

//==============================================================
// OvrDefaultComponent
class OvrDefaultComponent : public VRMenuComponent
{
public:
	static const int TYPE_ID = 1;

    OvrDefaultComponent( Vector3f const & hilightOffset = Vector3f( 0.0f, 0.0f, 0.05f ), 
            float const hilightScale = 1.05f, 
            float const fadeDuration = 0.25f, 
            float const fadeDelay = 0.25f,
			Vector4f const & textNormalColor = Vector4f( 1.0f ), 
			Vector4f const & textHilightColor = Vector4f( 1.0f ) );

	virtual int		typeId() const { return TYPE_ID; }

    void			setSuppressText( bool const suppress ) { m_suppressText = suppress; }

private:
    // private variables
    // We may actually want these to be static...
    SoundLimiter    m_gazeOverSoundLimiter;
    SoundLimiter    m_downSoundLimiter;
    SoundLimiter    m_upSoundLimiter;

    SineFader       m_hilightFader;
    double          m_startFadeInTime;
    double          m_startFadeOutTime;
    Vector3f        m_hilightOffset;
    float           m_hilightScale;
    float           m_fadeDuration;
    float           m_fadeDelay;
    Vector4f		m_textNormalColor;
    Vector4f		m_textHilightColor;
    bool			m_suppressText;	// true if text should not be faded in

private:
    virtual eMsgStatus      onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event );

    eMsgStatus              frame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event );
    eMsgStatus              focusGained( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event );
    eMsgStatus              focusLost( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event );
};

//==============================================================
// OvrSurfaceToggleComponent
// Toggles surfaced based on pair index, the current is default state, +1 is hover
class OvrSurfaceToggleComponent : public VRMenuComponent
{
public:
	static const char *		TYPE_NAME;
	OvrSurfaceToggleComponent()
		: VRMenuComponent( VRMenuEventFlags_t( VRMENU_EVENT_FRAME_UPDATE ) )
        , m_groupIndex( 0 )
	{}

    void setGroupIndex( const int index )	{ m_groupIndex = index; }
    int groupIndex() const				{ return m_groupIndex;  }

	virtual const char *	typeName( ) const { return TYPE_NAME; }

private:
    virtual eMsgStatus      onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
		VRMenuObject * self, VRMenuEvent const & event );

    eMsgStatus      frame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
		VRMenuObject * self, VRMenuEvent const & event );

    int m_groupIndex;
};

} // namespace NervGear

#endif // Ovr_Default_Component
