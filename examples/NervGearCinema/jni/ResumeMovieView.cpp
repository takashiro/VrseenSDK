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

void ResumeMovieView::OneTimeInit( const char * launchIntent )
{
	LOG( "ResumeMovieView::OneTimeInit" );

	const double start = ovr_GetTimeInSeconds();

	CreateMenu( Cinema.app, Cinema.app->GetVRMenuMgr(), Cinema.app->GetDefaultFont() );

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

	Cinema.app->GetSwapParms().WarpProgram = WP_CHROMATIC;

	SetPosition(Cinema.app->GetVRMenuMgr(), Cinema.sceneMgr.Scene.FootPos);

	Cinema.sceneMgr.ClearGazeCursorGhosts();
    Cinema.app->GetGuiSys().openMenu( Cinema.app, Cinema.app->GetGazeCursor(), "ResumeMoviePrompt" );

	CurViewState = VIEWSTATE_OPEN;
}

void ResumeMovieView::OnClose()
{
	LOG( "OnClose" );

    Cinema.app->GetGuiSys().closeMenu( Cinema.app, Menu, false );

	CurViewState = VIEWSTATE_CLOSED;
}

bool ResumeMovieView::Command( const char * msg )
{
	return false;
}

bool ResumeMovieView::OnKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType )
{
	return false;
}

Matrix4f ResumeMovieView::DrawEyeView( const int eye, const float fovDegrees )
{
	return Cinema.sceneMgr.DrawEyeView( eye, fovDegrees );
}

void ResumeMovieView::SetPosition( OvrVRMenuMgr & menuMgr, const Vector3f &pos )
{
    menuHandle_t centerRootHandle = Menu->handleForId( menuMgr, ID_CENTER_ROOT );
    VRMenuObject * centerRoot = menuMgr.toObject( centerRootHandle );
    OVR_ASSERT( centerRoot != NULL );

    Posef pose = centerRoot->localPose();
    pose.Position = pos;
    centerRoot->setLocalPose( pose );
}

void ResumeMovieView::CreateMenu( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font )
{
	Menu = VRMenu::Create( "ResumeMoviePrompt" );

    Vector3f fwd( 0.0f, 0.0f, 1.0f );
	Vector3f up( 0.0f, 1.0f, 0.0f );
	Vector3f defaultScale( 1.0f );

    Array< VRMenuObjectParms const * > parms;

	VRMenuFontParms fontParms( true, true, false, false, false, 1.3f );

	Quatf orientation( Vector3f( 0.0f, 1.0f, 0.0f ), 0.0f );
	Vector3f centerPos( 0.0f, 0.0f, 0.0f );

	VRMenuObjectParms centerRootParms( VRMENU_CONTAINER, Array< VRMenuComponent* >(), VRMenuSurfaceParms(), "CenterRoot",
			Posef( orientation, centerPos ), Vector3f( 1.0f, 1.0f, 1.0f ), fontParms,
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
        Posef panelPose( Quatf( up, 0.0f ), Vector3f( 0.0f, 2.2f, -3.0f ) );

		VRMenuObjectParms p( VRMENU_STATIC, Array< VRMenuComponent* >(),
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
    Array<const char *> options;
    options.append( CinemaStrings::ResumeMenu_Resume.toCString() );
    options.append( CinemaStrings::ResumeMenu_Restart.toCString() );

    Array<const char *> icons;
    icons.append( "assets/resume.png" );
    icons.append( "assets/restart.png" );

    Array<PanelPose> optionPositions;
    optionPositions.append( PanelPose( Quatf( up, 0.0f / 180.0f * Mathf::Pi ), Vector3f( -0.5f, 1.7f, -3.0f ), Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) ) );
    optionPositions.append( PanelPose( Quatf( up, 0.0f / 180.0f * Mathf::Pi ), Vector3f(  0.5f, 1.7f, -3.0f ), Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) ) );

    int borderWidth = 0, borderHeight = 0;
    GLuint borderTexture = LoadTextureFromApplicationPackage( "assets/resume_restart_border.png", TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), borderWidth, borderHeight );

    for ( int i = 0; i < optionPositions.sizeInt(); ++i )
	{
		ResumeMovieComponent * resumeMovieComponent = new ResumeMovieComponent( this, i );
		Array< VRMenuComponent* > optionComps;
        optionComps.append( resumeMovieComponent );

		VRMenuSurfaceParms panelSurfParms( "",
				borderTexture, borderWidth, borderHeight, SURFACE_TEXTURE_ADDITIVE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );

		Posef panelPose( optionPositions[ i ].Orientation, optionPositions[ i ].Position );
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


        Bounds3f textBounds = optionObject->getTextLocalBounds( font );
        optionObject->setTextLocalPosition( Vector3f( iconWidth * VRMenuObject::DEFAULT_TEXEL_SCALE * 0.5f, 0.0f, 0.0f ) );

		Posef iconPose( optionPositions[ i ].Orientation, optionPositions[ i ].Position + Vector3f( textBounds.GetMins().x, 0.0f, 0.01f ) );
		p = new VRMenuObjectParms( VRMENU_STATIC, Array< VRMenuComponent* >(),
				iconSurfParms, NULL, iconPose, defaultScale, fontParms, VRMenuId_t( ID_OPTION_ICONS.Get() + i ),
				VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

        parms.append( p );

        Menu->addItems( menuMgr, font, parms, centerRootHandle, false );
	    DeletePointerArray( parms );
        parms.clear();

        menuHandle_t iconHandle = centerRoot->childHandleForId( menuMgr, VRMenuId_t( ID_OPTION_ICONS.Get() + i ) );
        resumeMovieComponent->Icon = menuMgr.toObject( iconHandle );
	}

    Cinema.app->GetGuiSys().addMenu( Menu );
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

Matrix4f ResumeMovieView::Frame( const VrFrame & vrFrame )
{
	// We want 4x MSAA in the selection screen
	EyeParms eyeParms = Cinema.app->GetEyeParms();
	eyeParms.multisamples = 4;
	Cinema.app->SetEyeParms( eyeParms );

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
