/************************************************************************************

Filename    :   PanoMenu.h
Content     :   VRMenu shown when within panorama
Created     :   September 15, 2014
Authors     :   Warsam Osman

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Oculus360Photos/ directory. An additional grant
of patent rights can be found in the PATENTS file in the same directory.

************************************************************************************/

#if !defined( OVR_PanoMenu_h )
#define OVR_PanoMenu_h

#include "gui/VRMenu.h"
#include "gui/Fader.h"

namespace NervGear {

class Oculus360Photos;
class SearchPaths;
class OvrPanoMenuRootComponent;
class OvrAttributionInfo;
class OvrMetaData;
struct OvrMetaDatum;

//==============================================================
// OvrPanoMenu
class OvrPanoMenu : public VRMenu
{
public:
	static char const *	MENU_NAME;
	static const VRMenuId_t ID_CENTER_ROOT;
	static const VRMenuId_t	ID_BROWSER_BUTTON;
	static const VRMenuId_t	ID_FAVORITES_BUTTON;

	// only one of these every needs to be created
	static  OvrPanoMenu *		Create(
		App * app,
		Oculus360Photos * photos,
		OvrVRMenuMgr & menuMgr,
		BitmapFont const & font,
		OvrMetaData & metaData,
		float fadeOutTime,
		float radius );

    void					updateButtonsState( const OvrMetaDatum * const ActivePano, bool showSwipeOverride = false );
    void					startFadeOut();

    Oculus360Photos *		photos() 					{ return m_photos; }
    OvrMetaData & 			metaData() 					{ return m_metaData; }
    menuHandle_t			loadingIconHandle() const	{ return m_loadingIconHandle; }

    float					fadeAlpha() const;
    bool					interacting() const				{ return focusedHandle().IsValid(); }

private:
	OvrPanoMenu( App * app, Oculus360Photos * photos, OvrVRMenuMgr & menuMgr, BitmapFont const & font,
		OvrMetaData & metaData, float fadeOutTime, float radius );

    ~OvrPanoMenu();

    void onItemEvent_Impl( App * app, VRMenuId_t const itemId, VRMenuEvent const & event ) override;
    void frameImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr, BitmapFont const & font,
                                    BitmapFontSurface & fontSurface, gazeCursorUserId_t const gazeUserId ) override;

	// Globals
    App *					m_app;
    OvrVRMenuMgr &			m_menuMgr;
    const BitmapFont &		m_font;
    Oculus360Photos *		m_photos;
    OvrMetaData & 			m_metaData;

    menuHandle_t			m_loadingIconHandle;
    menuHandle_t			m_attributionHandle;
    menuHandle_t			m_browserButtonHandle;
    menuHandle_t			m_favoritesButtonHandle;
    menuHandle_t			m_swipeLeftIndicatorHandle;
    menuHandle_t			m_swipeRightIndicatorHandle;

    SineFader				m_fader;
    const float				m_fadeOutTime;
    float					m_currentFadeRate;

    const float				m_radius;

    float					m_buttonCoolDown;
};

}

#endif
