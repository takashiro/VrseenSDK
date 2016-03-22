/************************************************************************************

Filename    :   VRMenuFrame.h
Content     :   Menu component for handling hit tests and dispatching events.
Created     :   June 23, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#if !defined( OVR_VRMenuFrame_h )
#define OVR_VRMenuFrame_h

#include "VRMenuObject.h"
#include "VRMenuEvent.h"
#include "GazeCursor.h"
#include "SoundLimiter.h"

NV_NAMESPACE_BEGIN

struct VrFrame;
class App;

//==============================================================
// VRMenuEventHandler
class VRMenuEventHandler
{
public:
	VRMenuEventHandler();
	~VRMenuEventHandler();

    void			frame( App * app, const VrFrame & vrFrame, OvrVRMenuMgr & menuMgr, BitmapFont const & font,
                            menuHandle_t const & rootHandle, VPosf const & menuPose,
                            gazeCursorUserId_t const & gazeUserId, VArray< VRMenuEvent > & events );

    void			handleEvents( App * app, const VrFrame & vrFrame, OvrVRMenuMgr & menuMgr,
                            menuHandle_t const rootHandle, VArray< VRMenuEvent > const & events ) const;

    void			initComponents( VArray< VRMenuEvent > & events );
    void			opening( VArray< VRMenuEvent > & events );
    void			opened( VArray< VRMenuEvent > & events );
    void			closing( VArray< VRMenuEvent > & events );
    void			closed( VArray< VRMenuEvent > & events );

    menuHandle_t	focusedHandle() const { return m_focusedHandle; }

private:
    menuHandle_t	m_focusedHandle;

    SoundLimiter	m_gazeOverSoundLimiter;
    SoundLimiter	m_downSoundLimiter;
    SoundLimiter	m_upSoundLimiter;

private:
    bool            dispatchToComponents( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                            VRMenuEvent const & event, VRMenuObject * receiver ) const;
    bool            dispatchToPath( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                            VRMenuEvent const & event, VArray< menuHandle_t > const & path, bool const log ) const;
    bool            broadcastEvent( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                            VRMenuEvent const & event, VRMenuObject * receiver ) const;
};

NV_NAMESPACE_END

#endif // OVR_VRMenuFrame_h
