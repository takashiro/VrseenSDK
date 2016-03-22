#include <GazeCursor.h>
#include <VRMenuMgr.h>

#include "CinemaApp.h"
#include "ResumeMovieView.h"
#include "ResumeMovieComponent.h"
#include "VApkFile.h"
#include "CinemaStrings.h"

namespace OculusCinema {

VRMenuId_t ResumeMovieView::ID_CENTER_ROOT( 1000 );
VRMenuId_t ResumeMovieView::ID_TITLE( 1001 );
VRMenuId_t ResumeMovieView::ID_OPTIONS( 1002 );
VRMenuId_t ResumeMovieView::ID_OPTION_ICONS( 2000 );

ResumeMovieView::ResumeMovieView( CinemaApp &cinema ) :
	View( "ResumeMovieView" ),
	Cinema( cinema ),
	Menu( NULL )
{
// This is called at library load time, so the system is not initialized
// properly yet.
}

ResumeMovieView::~ResumeMovieView()
{
}

void ResumeMovieView::OneTimeInit( const VString &launchIntent )
{
	LOG( "ResumeMovieView::OneTimeInit" );

	const double start = ovr_GetTimeInSeconds();

	CreateMenu( vApp, vApp->vrMenuMgr(), vApp->defaultFont() );

	LOG( "ResumeMovieView::OneTimeInit: %3.1f seconds", ovr_GetTimeInSeconds() - start );
}

void ResumeMovieView::OneTimeShutdown()
{
	LOG( "ResumeMovieView::OneTimeShutdown" );
}

void ResumeMovieView::OnOpen()
{
	LOG( "OnOpen" );

	Cinema.sceneMgr.LightsOn( 0.5f );

	vApp->swapParms().WarpProgram = WP_CHROMATIC;

	SetPosition(vApp->vrMenuMgr(), Cinema.sceneMgr.Scene.FootPos);

	Cinema.sceneMgr.ClearGazeCursorGhosts();
    vApp->guiSys().openMenu( vApp, vApp->gazeCursor(), "ResumeMoviePrompt" );

	CurViewState = VIEWSTATE_OPEN;
}

void ResumeMovieView::OnClose()
{
	LOG( "OnClose" );

    vApp->guiSys().closeMenu( vApp, Menu, false );

	CurViewState = VIEWSTATE_CLOSED;
}

bool ResumeMovieView::Command(const VEvent &)
{
	return false;
}

bool ResumeMovieView::OnKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType )
{
	return false;
}

VR4Matrixf ResumeMovieView::DrawEyeView( const int eye, const float fovDegrees )
{
	return Cinema.sceneMgr.DrawEyeView( eye, fovDegrees );
}

void ResumeMovieView::SetPosition( OvrVRMenuMgr & menuMgr, const V3Vectf &pos )
{
    menuHandle_t centerRootHandle = Menu->handleForId( menuMgr, ID_CENTER_ROOT );
    VRMenuObject * centerRoot = menuMgr.toObject( centerRootHandle );
    OVR_ASSERT( centerRoot != NULL );

    VPosf pose = centerRoot->localPose();
    pose.Position = pos;
    centerRoot->setLocalPose( pose );
}

void ResumeMovieView::CreateMenu( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font )
{
	Menu = VRMenu::Create( "ResumeMoviePrompt" );

    V3Vectf fwd( 0.0f, 0.0f, 1.0f );
    V3Vectf up( 0.0f, 1.0f, 0.0f );
    V3Vectf defaultScale( 1.0f );

    VArray< VRMenuObjectParms const * > parms;

	VRMenuFontParms fontParms( true, true, false, false, false, 1.3f );

    VQuatf orientation( V3Vectf( 0.0f, 1.0f, 0.0f ), 0.0f );
    V3Vectf centerPos( 0.0f, 0.0f, 0.0f );

	VRMenuObjectParms centerRootParms( VRMENU_CONTAINER, VArray< VRMenuComponent* >(), VRMenuSurfaceParms(), "CenterRoot",
            VPosf( orientation, centerPos ), V3Vectf( 1.0f, 1.0f, 1.0f ), fontParms,
			ID_CENTER_ROOT, VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );
    parms.append( &centerRootParms );

    Menu->initWithItems( menuMgr, font, 0.0f, VRMenuFlags_t(), parms );
    parms.clear();

    // the centerroot item will get touch relative and touch absolute events and use them to rotate the centerRoot
    menuHandle_t centerRootHandle = Menu->handleForId( menuMgr, ID_CENTER_ROOT );
    VRMenuObject * centerRoot = menuMgr.toObject( centerRootHandle );
    OVR_ASSERT( centerRoot != NULL );

    // ==============================================================================
    //
    // title
    //
    {
        VPosf panelPose( VQuatf( up, 0.0f ), V3Vectf( 0.0f, 2.2f, -3.0f ) );

		VRMenuObjectParms p( VRMENU_STATIC, VArray< VRMenuComponent* >(),
                VRMenuSurfaceParms(), CinemaStrings::ResumeMenu_Title.toCString(), panelPose, defaultScale, fontParms, VRMenuId_t( ID_TITLE.Get() ),
				VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

        parms.append( &p );

        Menu->addItems( menuMgr, font, parms, centerRootHandle, false );
        parms.clear();
    }

    // ==============================================================================
    //
    // options
    //
    VArray<const char *> options;
    options.append( CinemaStrings::ResumeMenu_Resume.toCString() );
    options.append( CinemaStrings::ResumeMenu_Restart.toCString() );

    VArray<const char *> icons;
    icons.append( "assets/resume.png" );
    icons.append( "assets/restart.png" );

    VArray<PanelPose> optionPositions;
    optionPositions.append( PanelPose( VQuatf( up, 0.0f / 180.0f * VConstantsf::Pi ), V3Vectf( -0.5f, 1.7f, -3.0f ), V4Vectf( 1.0f, 1.0f, 1.0f, 1.0f ) ) );
    optionPositions.append( PanelPose( VQuatf( up, 0.0f / 180.0f * VConstantsf::Pi ), V3Vectf(  0.5f, 1.7f, -3.0f ), V4Vectf( 1.0f, 1.0f, 1.0f, 1.0f ) ) );

    int borderWidth = 0, borderHeight = 0;
    GLuint borderTexture = LoadTextureFromApplicationPackage( "assets/resume_restart_border.png", TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), borderWidth, borderHeight );

    for ( int i = 0; i < optionPositions.length(); ++i )
	{
		ResumeMovieComponent * resumeMovieComponent = new ResumeMovieComponent( this, i );
		VArray< VRMenuComponent* > optionComps;
        optionComps.append( resumeMovieComponent );

		VRMenuSurfaceParms panelSurfParms( "",
				borderTexture, borderWidth, borderHeight, SURFACE_TEXTURE_ADDITIVE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );

        VPosf panelPose( optionPositions[ i ].Orientation, optionPositions[ i ].Position );
		VRMenuObjectParms * p = new VRMenuObjectParms( VRMENU_BUTTON, optionComps,
				panelSurfParms, options[ i ], panelPose, defaultScale, fontParms, VRMenuId_t( ID_OPTIONS.Get() + i ),
				VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

        parms.append( p );

        Menu->addItems( menuMgr, font, parms, centerRootHandle, false );
		DeletePointerArray( parms );
        parms.clear();

		// add icon
        menuHandle_t optionHandle = centerRoot->childHandleForId( menuMgr, VRMenuId_t( ID_OPTIONS.Get() + i ) );
        VRMenuObject * optionObject = menuMgr.toObject( optionHandle );
	    OVR_ASSERT( optionObject != NULL );

	    int iconWidth = 0, iconHeight = 0;
	    GLuint iconTexture = LoadTextureFromApplicationPackage( icons[ i ], TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), iconWidth, iconHeight );

		VRMenuSurfaceParms iconSurfParms( "",
				iconTexture, iconWidth, iconHeight, SURFACE_TEXTURE_DIFFUSE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );


        VBoxf textBounds = optionObject->getTextLocalBounds( font );
        optionObject->setTextLocalPosition( V3Vectf( iconWidth * VRMenuObject::DEFAULT_TEXEL_SCALE * 0.5f, 0.0f, 0.0f ) );

        VPosf iconPose( optionPositions[ i ].Orientation, optionPositions[ i ].Position + V3Vectf( textBounds.GetMins().x, 0.0f, 0.01f ) );
		p = new VRMenuObjectParms( VRMENU_STATIC, VArray< VRMenuComponent* >(),
				iconSurfParms, NULL, iconPose, defaultScale, fontParms, VRMenuId_t( ID_OPTION_ICONS.Get() + i ),
				VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

        parms.append( p );

        Menu->addItems( menuMgr, font, parms, centerRootHandle, false );
	    DeletePointerArray( parms );
        parms.clear();

        menuHandle_t iconHandle = centerRoot->childHandleForId( menuMgr, VRMenuId_t( ID_OPTION_ICONS.Get() + i ) );
        resumeMovieComponent->Icon = menuMgr.toObject( iconHandle );
	}

    vApp->guiSys().addMenu( Menu );
}

void ResumeMovieView::ResumeChoice( int itemNum )
{
	if ( itemNum == 0 )
	{
		Cinema.resumeMovieFromSavedLocation();
	}
	else
	{
		Cinema.playMovieFromBeginning();
	}
}

VR4Matrixf ResumeMovieView::Frame( const VrFrame & vrFrame )
{
	// We want 4x MSAA in the selection screen
	EyeParms eyeParms = vApp->eyeParms();
	eyeParms.multisamples = 4;
	vApp->setEyeParms( eyeParms );

    if ( Menu->isClosedOrClosing() && !Menu->isOpenOrOpening() )
	{
		if ( Cinema.inLobby )
		{
			Cinema.theaterSelection();
		}
		else
		{
			Cinema.setMovieSelection( false );
		}
	}

	return Cinema.sceneMgr.Frame( vrFrame );
}

} // namespace OculusCinema
