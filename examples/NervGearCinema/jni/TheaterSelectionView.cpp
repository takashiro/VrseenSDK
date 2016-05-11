/************************************************************************************

Filename    :   TheaterSelectionView.cpp
Content     :
Created     :	6/19/2014
Authors     :   Jim Dos�

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "GazeCursor.h"
#include "BitmapFont.h"
#include "gui/VRMenuMgr.h"
#include "CarouselBrowserComponent.h"
#include "TheaterSelectionComponent.h"
#include "TheaterSelectionView.h"
#include "CinemaApp.h"
#include "SwipeHintComponent.h"
#include "VApkFile.h"
#include "CinemaStrings.h"
#include "Native.h"
#include "core/VTimer.h"

#include <VEyeItem.h>

namespace OculusCinema {

VRMenuId_t TheaterSelectionView::ID_CENTER_ROOT( 1000 );
VRMenuId_t TheaterSelectionView::ID_ICONS( 1001 );
VRMenuId_t TheaterSelectionView::ID_TITLE_ROOT( 2000 );
VRMenuId_t TheaterSelectionView::ID_SWIPE_ICON_LEFT( 3000 );
VRMenuId_t TheaterSelectionView::ID_SWIPE_ICON_RIGHT( 3001 );

static const int NumSwipeTrails = 3;

TheaterSelectionView::TheaterSelectionView( CinemaApp &cinema ) :
	View( "TheaterSelectionView" ),
	Cinema( cinema ),
	Menu( NULL ),
	CenterRoot( NULL ),
	SelectionObject( NULL ),
	TheaterBrowser( NULL ),
	SelectedTheater( 0 ),
	IgnoreSelectTime( 0 )

{
// This is called at library load time, so the system is not initialized
// properly yet.
}

TheaterSelectionView::~TheaterSelectionView()
{
}

void TheaterSelectionView::OneTimeInit( const VString &launchIntent )
{
	vInfo("TheaterSelectionView::OneTimeInit");

    const double start = VTimer::Seconds();

	// Start with "Home theater" selected
	SelectedTheater = 0;

    vInfo("TheaterSelectionView::OneTimeInit:" << (VTimer::Seconds() - start) << "seconds");
}

void TheaterSelectionView::OneTimeShutdown()
{
	vInfo("TheaterSelectionView::OneTimeShutdown");
}

void TheaterSelectionView::SelectTheater(int theater)
{
	SelectedTheater = theater;

	Cinema.sceneMgr.SetSceneModel(Cinema.modelMgr.GetTheater(SelectedTheater));
    SetPosition(vApp->vrMenuMgr(), Cinema.sceneMgr.Scene.FootPos);
}

void TheaterSelectionView::OnOpen()
{
	vInfo("OnOpen");

	if ( Menu == NULL )
	{
        CreateMenu( vApp, vApp->vrMenuMgr(), vApp->defaultFont() );
	}

	SelectTheater( SelectedTheater );
	TheaterBrowser->SetSelectionIndex( SelectedTheater );

	Cinema.sceneMgr.LightsOn( 0.5f );

    vApp->kernel()->setSmoothProgram(VK_DEFAULT_CB);

	Cinema.sceneMgr.ClearGazeCursorGhosts();
    vApp->guiSys().openMenu( vApp, vApp->gazeCursor(), "TheaterSelectionBrowser" );

	// ignore clicks for 0.5 seconds to avoid accidentally clicking through
    IgnoreSelectTime = VTimer::Seconds() + 0.5;

	CurViewState = VIEWSTATE_OPEN;
}

void TheaterSelectionView::OnClose()
{
	vInfo("OnClose");

    vApp->guiSys().closeMenu( vApp, Menu, false );

	CurViewState = VIEWSTATE_CLOSED;
}

bool TheaterSelectionView::Command(const VEvent &)
{
	return false;
}

bool TheaterSelectionView::OnKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType )
{
	return false;
}

VR4Matrixf TheaterSelectionView::DrawEyeView( const int eye, const float fovDegrees )
{
	return Cinema.sceneMgr.DrawEyeView( eye, fovDegrees );
}

void TheaterSelectionView::SetPosition( OvrVRMenuMgr & menuMgr, const V3Vectf &pos )
{
    VPosf pose = CenterRoot->localPose();
    pose.Position = pos;
    CenterRoot->setLocalPose( pose );

    menuHandle_t titleRootHandle = Menu->handleForId( menuMgr, ID_TITLE_ROOT );
    VRMenuObject * titleRoot = menuMgr.toObject( titleRootHandle );
    vAssert( titleRoot != NULL );

    pose = titleRoot->localPose();
    pose.Position = pos;
    titleRoot->setLocalPose( pose );
}

void TheaterSelectionView::CreateMenu( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font )
{
	Menu = VRMenu::Create( "TheaterSelectionBrowser" );

    V3Vectf fwd( 0.0f, 0.0f, 1.0f );
    V3Vectf up( 0.0f, 1.0f, 0.0f );
    V3Vectf defaultScale( 1.0f );

    VArray< VRMenuObjectParms const * > parms;

	VRMenuFontParms fontParms( true, true, false, false, false, 1.0f );

    VQuatf orientation( V3Vectf( 0.0f, 1.0f, 0.0f ), 0.0f );
    V3Vectf centerPos( 0.0f, 0.0f, 0.0f );

	VRMenuObjectParms centerRootParms( VRMENU_CONTAINER, VArray< VRMenuComponent* >(), VRMenuSurfaceParms(), "CenterRoot",
            VPosf( orientation, centerPos ), V3Vectf( 1.0f, 1.0f, 1.0f ), fontParms,
			ID_CENTER_ROOT, VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );
    parms.append( &centerRootParms );

	// title
    const VPosf titlePose( VQuatf( V3Vectf( 0.0f, 1.0f, 0.0f ), 0.0f ), V3Vectf( 0.0f, 0.0f, 0.0f ) );

	VRMenuObjectParms titleRootParms( VRMENU_CONTAINER, VArray< VRMenuComponent* >(), VRMenuSurfaceParms(),
			"TitleRoot", titlePose, defaultScale, fontParms, ID_TITLE_ROOT,
			VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

    parms.append( &titleRootParms );

    Menu->initWithItems( menuMgr, font, 0.0f, VRMenuFlags_t(), parms );
    parms.clear();

	int count = Cinema.modelMgr.GetTheaterCount();
	for ( int i = 0 ; i < count ; i++ )
	{
		const SceneDef & theater = Cinema.modelMgr.GetTheater( i );

		CarouselItem *item = new CarouselItem();
		item->texture = theater.IconTexture;

        Theaters.append( item );
	}

	VArray<PanelPose> panelPoses;

    panelPoses.append( PanelPose( VQuatf( up, 0.0f ), V3Vectf( -2.85f, 1.8f, -5.8f ), V4Vectf( 0.0f, 0.0f, 0.0f, 0.0f ) ) );
    panelPoses.append( PanelPose( VQuatf( up, 0.0f ), V3Vectf( -1.90f, 1.8f, -5.0f ), V4Vectf( 0.25f, 0.25f, 0.25f, 1.0f ) ) );
    panelPoses.append( PanelPose( VQuatf( up, 0.0f ), V3Vectf( -0.95f, 1.8f, -4.2f ), V4Vectf( 0.45f, 0.45f, 0.45f, 1.0f ) ) );
    panelPoses.append( PanelPose( VQuatf( up, 0.0f ), V3Vectf(  0.00f, 1.8f, -3.4f ), V4Vectf( 1.0f, 1.0f, 1.0f, 1.0f ) ) );
    panelPoses.append( PanelPose( VQuatf( up, 0.0f ), V3Vectf(  0.95f, 1.8f, -4.2f ), V4Vectf( 0.45f, 0.45f, 0.45f, 1.0f ) ) );
    panelPoses.append( PanelPose( VQuatf( up, 0.0f ), V3Vectf(  1.90f, 1.8f, -5.0f ), V4Vectf( 0.25f, 0.25f, 0.25f, 1.0f ) ) );
    panelPoses.append( PanelPose( VQuatf( up, 0.0f ), V3Vectf(  2.85f, 1.8f, -5.8f ), V4Vectf( 0.0f, 0.0f, 0.0f, 0.0f ) ) );

    // the centerroot item will get touch relative and touch absolute events and use them to rotate the centerRoot
    menuHandle_t centerRootHandle = Menu->handleForId( menuMgr, ID_CENTER_ROOT );
    CenterRoot = menuMgr.toObject( centerRootHandle );
    vAssert( CenterRoot != NULL );

    TheaterBrowser = new CarouselBrowserComponent( Theaters, panelPoses );
    CenterRoot->addComponent( TheaterBrowser );

    int selectionWidth = 0;
    int selectionHeight = 0;
	GLuint selectionTexture = LoadTextureFromApplicationPackage( "assets/VoidTheater.png",
			TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), selectionWidth, selectionHeight );

    int centerIndex = panelPoses.length() / 2;
    for ( int i = 0; i < panelPoses.length(); ++i )
	{
		VRMenuSurfaceParms panelSurfParms( "",
				selectionTexture, selectionWidth, selectionHeight, SURFACE_TEXTURE_DIFFUSE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );

	    VRMenuId_t panelId = VRMenuId_t( ID_ICONS.value() + i );
        VQuatf rot( up, 0.0f );
        VPosf panelPose( rot, fwd );
		VRMenuObjectParms * p = new VRMenuObjectParms( VRMENU_BUTTON, VArray< VRMenuComponent* >(),
				panelSurfParms, NULL, panelPose, defaultScale, fontParms, panelId,
				( i == centerIndex ) ? VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_DEPTH ) : VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_FOCUS_GAINED ) | VRMENUOBJECT_FLAG_NO_DEPTH,
				VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );
        parms.append( p );
	}

    Menu->addItems( menuMgr, font, parms, centerRootHandle, false );
    DeletePointerArray( parms );
    parms.clear();

    menuHandle_t selectionHandle = CenterRoot->childHandleForId( menuMgr, VRMenuId_t( ID_ICONS.value() + centerIndex ) );
    SelectionObject = menuMgr.toObject( selectionHandle );

    V3Vectf selectionBoundsExpandMin = V3Vectf( 0.0f, -0.25f, 0.0f );
    V3Vectf selectionBoundsExpandMax = V3Vectf( 0.0f, 0.25f, 0.0f );
    SelectionObject->setLocalBoundsExpand( selectionBoundsExpandMin, selectionBoundsExpandMax );
    SelectionObject->addFlags( VRMENUOBJECT_HIT_ONLY_BOUNDS );

	VArray<VRMenuObject *> menuObjs;
	VArray<CarouselItemComponent *> menuComps;
    for ( int i = 0; i < panelPoses.length(); ++i )
	{
        menuHandle_t posterImageHandle = CenterRoot->childHandleForId( menuMgr, VRMenuId_t( ID_ICONS.value() + i ) );
        VRMenuObject *posterImage = menuMgr.toObject( posterImageHandle );
        menuObjs.append( posterImage );

		CarouselItemComponent *panelComp = new TheaterSelectionComponent( i == centerIndex ? this : NULL );
        posterImage->addComponent( panelComp );
        menuComps.append( panelComp );
	}
	TheaterBrowser->SetMenuObjects( menuObjs, menuComps );

    // ==============================================================================
    //
    // title
    //
    {
        VPosf panelPose( VQuatf( up, 0.0f ), V3Vectf( 0.0f, 2.5f, -3.4f ) );

		VRMenuFontParms titleFontParms( true, true, false, false, false, 1.3f );

		VRMenuObjectParms p( VRMENU_STATIC, VArray< VRMenuComponent* >(),
                VRMenuSurfaceParms(), CinemaStrings::TheaterSelection_Title, panelPose, defaultScale, titleFontParms, VRMenuId_t( ID_TITLE_ROOT.value() + 1 ),
				VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

        parms.append( &p );

        menuHandle_t titleRootHandle = Menu->handleForId( menuMgr, ID_TITLE_ROOT );
        Menu->addItems( menuMgr, font, parms, titleRootHandle, false );
        parms.clear();
    }

	// ==============================================================================
    //
    // swipe icons
    //
    {
    	int 	swipeIconLeftWidth = 0;
    	int		swipeIconLeftHeight = 0;
    	int 	swipeIconRightWidth = 0;
    	int		swipeIconRightHeight = 0;

    	GLuint swipeIconLeftTexture = LoadTextureFromApplicationPackage( "assets/SwipeSuggestionArrowLeft.png",
				TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), swipeIconLeftWidth, swipeIconLeftHeight );

		GLuint swipeIconRightTexture = LoadTextureFromApplicationPackage( "assets/SwipeSuggestionArrowRight.png",
				TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), swipeIconRightWidth, swipeIconRightHeight );

		VRMenuSurfaceParms swipeIconLeftSurfParms( "",
				swipeIconLeftTexture, swipeIconLeftWidth, swipeIconLeftHeight, SURFACE_TEXTURE_DIFFUSE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );

		VRMenuSurfaceParms swipeIconRightSurfParms( "",
				swipeIconRightTexture, swipeIconRightWidth, swipeIconRightHeight, SURFACE_TEXTURE_DIFFUSE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );

		float yPos = 1.8f - ( selectionHeight - swipeIconLeftHeight ) * 0.5f * VRMenuObject::DEFAULT_TEXEL_SCALE;
		for( int i = 0; i < NumSwipeTrails; i++ )
		{
            VPosf swipePose = VPosf( VQuatf( up, 0.0f ), V3Vectf( 0.0f, yPos, -3.4f ) );
			swipePose.Position.x = ( ( selectionWidth + swipeIconLeftWidth * ( i + 2 ) ) * -0.5f ) * VRMenuObject::DEFAULT_TEXEL_SCALE;
			swipePose.Position.z += 0.01f * ( float )i;

			VArray< VRMenuComponent* > leftComps;
            leftComps.append( new SwipeHintComponent( TheaterBrowser, false, 1.3333f, 0.4f + ( float )i * 0.13333f, 5.0f ) );

			VRMenuObjectParms * swipeIconLeftParms = new VRMenuObjectParms( VRMENU_BUTTON, leftComps,
				swipeIconLeftSurfParms, "", swipePose, defaultScale, fontParms, VRMenuId_t( ID_SWIPE_ICON_LEFT.value() + i ),
				VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_DEPTH ) | VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ),
				VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

            parms.append( swipeIconLeftParms );

			swipePose.Position.x = ( ( selectionWidth + swipeIconRightWidth * ( i + 2 ) ) * 0.5f ) * VRMenuObject::DEFAULT_TEXEL_SCALE;

			VArray< VRMenuComponent* > rightComps;
            rightComps.append( new SwipeHintComponent( TheaterBrowser, true, 1.3333f, 0.4f + ( float )i * 0.13333f, 5.0f ) );

			VRMenuObjectParms * swipeIconRightParms = new VRMenuObjectParms( VRMENU_STATIC, rightComps,
				swipeIconRightSurfParms, "", swipePose, defaultScale, fontParms, VRMenuId_t( ID_SWIPE_ICON_RIGHT.value() + i ),
				VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_DEPTH ) | VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ),
				VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

            parms.append( swipeIconRightParms );
		}

        Menu->addItems( menuMgr, font, parms, centerRootHandle, false );
		DeletePointerArray( parms );
        parms.clear();
    }

    vApp->guiSys().addMenu( Menu );
}

void TheaterSelectionView::SelectPressed( void )
{
    const double now = VTimer::Seconds();
	if ( now < IgnoreSelectTime )
	{
		// ignore selection for first 0.5 seconds to reduce chances of accidentally clicking through
		return;
	}

	Cinema.sceneMgr.SetSceneModel( Cinema.modelMgr.GetTheater( SelectedTheater ) );
	if ( Cinema.inLobby )
	{
		Cinema.resumeOrRestartMovie();
	}
	else
	{
		Cinema.resumeMovieFromSavedLocation();
	}
}

VR4Matrixf TheaterSelectionView::Frame( const VFrame & vrFrame )
{
	// We want 4x MSAA in the selection screen
    vApp->eyeSettings().multisamples = 4;

    if ( SelectionObject->isHilighted() )
	{
        TheaterBrowser->CheckGamepad( vApp, vrFrame, vApp->vrMenuMgr(), CenterRoot );
	}

	int selectedItem = TheaterBrowser->GetSelection();
	if ( SelectedTheater != selectedItem )
	{
        if ( ( selectedItem >= 0 ) && ( selectedItem < Theaters.length() ) )
		{
            vInfo("Select:" << selectedItem << "," << SelectedTheater << "," << Theaters.length() << "," << Cinema.modelMgr.GetTheaterCount());
			SelectedTheater = selectedItem;
			SelectTheater( SelectedTheater );
		}
	}

    if ( Menu->isClosedOrClosing() && !Menu->isOpenOrOpening() )
	{
		Cinema.setMovieSelection( true );
	}

	if ( vrFrame.input.buttonPressed & BUTTON_B )
	{
        vApp->playSound( "touch_up" );
		Cinema.setMovieSelection( true );
	}

	return Cinema.sceneMgr.Frame( vrFrame );
}

} // namespace OculusCinema
