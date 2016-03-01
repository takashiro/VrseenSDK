/************************************************************************************

Filename    :   VolumePopup.h
Content     :   Popup dialog for when user changes sound volume.
Created     :   September 18, 2014
Authors     :   Jim Dose

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#if !defined( OVR_VolumePopup_h )
#define OVR_VolumePopup_h

#include "VRMenu.h"

NV_NAMESPACE_BEGIN

//==============================================================
// OvrVolumePopup
class OvrVolumePopup : public VRMenu
{
public:
	static const int		NumVolumeTics;

	static	VRMenuId_t		ID_BACKGROUND;
	static	VRMenuId_t		ID_VOLUME_ICON;
	static	VRMenuId_t		ID_VOLUME_TEXT;
	static	VRMenuId_t		ID_VOLUME_TICKS;

	static const char *		MENU_NAME;

	static const double		VolumeMenuFadeDelay;

    OvrVolumePopup();
    virtual ~OvrVolumePopup();

    // only one of these every needs to be created
    static  OvrVolumePopup * Create( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font );

    void 					checkForVolumeChange( App * app );
    void					close( App * app );

private:
    Vector3f				m_volumeTextOffset;
    int						m_currentVolume;

private:
    void					showVolume( App * app, const int current );

    // overloads
    virtual void    		frameImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, BitmapFont const & font,
                                    BitmapFontSurface & fontSurface, gazeCursorUserId_t const gazeUserId );
    virtual bool			onKeyEventImpl( App * app, int const keyCode, KeyState::eKeyEventType const eventType );

    void					createSubMenus( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font );
};

NV_NAMESPACE_END

#endif // OVR_VolumePopup_h
