/************************************************************************************

Filename    :   SurfaceAnim_Component.h
Content     :   A reusable component for animating VR menu object surfaces.
Created     :   Sept 23, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, Inc. All Rights reserved.


*************************************************************************************/

#if !defined( OVR_AnimComponents_h )
#define OVR_AnimComponents_h

#include "VRMenuComponent.h"

NV_NAMESPACE_BEGIN

//==============================================================
// OvrAnimComponent
//
// A component that contains the logic for animating a 
class OvrAnimComponent : public VRMenuComponent
{
public:
	enum eAnimState
	{
		ANIMSTATE_PAUSED,
		ANIMSTATE_PLAYING
	};

	OvrAnimComponent( float const framesPerSecond, bool const looping );
	virtual ~OvrAnimComponent() { }

    virtual eMsgStatus      onEventImpl( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
										  VRMenuObject * self, VRMenuEvent const & event );

    eMsgStatus frame( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self, VRMenuEvent const & event );

    void			setFrame( VRMenuObject * self, int const frameNum );
    void			play();
    void			pause();
    void			setLooping( bool const loop ) { m_looping = loop; }
    bool			isLooping() const { return m_looping; }
    void			setRate( float const fps ) { m_framesPerSecond = fps; }
    float			rate() const { return m_framesPerSecond; }
    int				curFrame() const { return m_curFrame; }

protected:
    virtual void	setFrameVisibilities( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self ) const = 0;
    virtual int		getNumFrames( VRMenuObject * self ) const = 0;

    double			baseTime() const { return m_baseTime;  }
    double			floatFrame() const { return m_fractionalFrame; }
    float			fractionalFrame() const { return m_fractionalFrame; }

private:
    double			m_baseTime;			// time when animation started or resumed
    int				m_baseFrame;			// base frame animation started on (for unpausing, this can be > 0 )
    int				m_curFrame;			// current frame the animation is on
    float			m_framesPerSecond;	// the playback rate of the animatoin
    eAnimState		m_animState;			// the current animation state
    bool			m_looping;			// true if the animation should loop
    bool			m_forceVisibilityUpdate;	// set to force visibilities to update even when paused (used by SetFrame )
    float			m_fractionalFrame;	// 0-1
    double			m_floatFrame;			// Animation floating point time
};

//==============================================================
// OvrSurfaceAnimComponent
//
class OvrSurfaceAnimComponent : public OvrAnimComponent
{
public:
	static const char *		TYPE_NAME;

	// Surfaces per frame must be set to the number of surfaces that should be renderered
	// for each frame. If only a single surface is shown per frame, surfacesPerFrame should be 1.
	// If multiple surfaces (for instance one diffuse and one additive) are shown then
	// surfacesPerFrame should be set to 2.
	// Frame numbers never
	OvrSurfaceAnimComponent( float const framesPerSecond, bool const looping, int const surfacesPerFrame );

    char const *	typeName() const { return TYPE_NAME; }

protected:
    virtual void	setFrameVisibilities( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self ) const;
    virtual int		getNumFrames( VRMenuObject * self ) const;

private:
    int				m_surfacesPerFrame;	// how many surfaces are shown per frame
};


//==============================================================
// OvrChildrenAnimComponent
//
class OvrTrailsAnimComponent : public OvrAnimComponent
{
public:
	static const char *		TYPE_NAME;

	OvrTrailsAnimComponent( float const framesPerSecond, bool const looping, int const numFrames,
		int const numFramesAhead, int const numFramesBehind );

    virtual const char *	typeName( ) const
	{
		return TYPE_NAME;
	}

protected:
    virtual void	setFrameVisibilities( App * app, VFrame const & vrFrame, OvrVRMenuMgr & menuMgr, VRMenuObject * self ) const;
    virtual int		getNumFrames( VRMenuObject * self ) const;

private:
    float			getAlphaForFrame( const int frame ) const;
    int				m_numFrames;
    int				m_framesAhead;
    int				m_framesBehind;
};

NV_NAMESPACE_END

#endif // OVR_AnimComponents_h
