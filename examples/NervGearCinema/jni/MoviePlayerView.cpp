#include <android/keycodes.h>
#include <VRMenuMgr.h>
#include "core/VTimer.h"
#include "CinemaApp.h"
#include "Native.h"
#include "CinemaStrings.h"

namespace OculusCinema
{

const int MoviePlayerView::MaxSeekSpeed = 5;
const int MoviePlayerView::ScrubBarWidth = 516;

const double MoviePlayerView::GazeTimeTimeout = 4;

MoviePlayerView::MoviePlayerView( CinemaApp &cinema ) :
	View( "MoviePlayerView" ),
	Cinema( cinema ),
	uiActive( false ),
	RepositionScreen( false ),
	SeekSpeed( 0 ),
	PlaybackPos( 0 ),
	NextSeekTime( 0 ),
	BackgroundTintTexture(),
	RWTexture(),
	RWHoverTexture(),
	RWPressedTexture(),
	FFTexture(),
	FFHoverTexture(),
	FFPressedTexture(),
	PlayTexture(),
	PlayHoverTexture(),
	PlayPressedTexture(),
	PauseTexture(),
	PauseHoverTexture(),
	PausePressedTexture(),
	CarouselTexture(),
	CarouselHoverTexture(),
	CarouselPressedTexture(),
	SeekbarBackgroundTexture(),
	SeekbarProgressTexture(),
	SeekPosition(),
	SeekFF2x(),
	SeekFF4x(),
	SeekFF8x(),
	SeekFF16x(),
	SeekFF32x(),
	SeekRW2x(),
	SeekRW4x(),
	SeekRW8x(),
	SeekRW16x(),
	SeekRW32x(),
	MoveScreenMenu( NULL ),
	MoveScreenLabel( Cinema ),
	MoveScreenAlpha(),
	PlaybackControlsMenu( NULL ),
	PlaybackControlsPosition( Cinema ),
	PlaybackControlsScale( Cinema ),
	MovieTitleLabel( Cinema ),
	SeekIcon( Cinema ),
	ControlsBackground( Cinema ),
	GazeTimer(),
	RewindButton( Cinema ),
	PlayButton( Cinema ),
	FastForwardButton( Cinema ),
	CarouselButton( Cinema ),
	SeekbarBackground( Cinema ),
	SeekbarProgress( Cinema ),
	ScrubBar(),
	CurrentTime( Cinema ),
	SeekTime( Cinema ),
	BackgroundClicked( false ),
	UIOpened( false )

{
}

MoviePlayerView::~MoviePlayerView()
{
}

//=========================================================================================

void MoviePlayerView::OneTimeInit( const VString & launchIntent )
{
	vInfo("MoviePlayerView::OneTimeInit");

    const double start = VTimer::Seconds();

	GazeUserId = vApp->gazeCursor().GenerateUserId();

	CreateMenu( vApp, vApp->vrMenuMgr(), vApp->defaultFont() );

    vInfo("MoviePlayerView::OneTimeInit:" << (VTimer::Seconds() - start) << "seconds");
}

void MoviePlayerView::OneTimeShutdown()
{
	vInfo("MoviePlayerView::OneTimeShutdown");
}

float PixelScale( const float x )
{
	return x * VRMenuObject::DEFAULT_TEXEL_SCALE;
}

V3Vectf PixelPos( const float x, const float y, const float z )
{
    return V3Vectf( PixelScale( x ), PixelScale( y ), PixelScale( z ) );
}

void PlayPressedCallback( UIButton *button, void *object )
{
	( ( MoviePlayerView * )object )->TogglePlayback();
}

void RewindPressedCallback( UIButton *button, void *object )
{
	( ( MoviePlayerView * )object )->RewindPressed();
}

void FastForwardPressedCallback( UIButton *button, void *object )
{
	( ( MoviePlayerView * )object )->FastForwardPressed();
}

void CarouselPressedCallback( UIButton *button, void *object )
{
	( ( MoviePlayerView * )object )->CarouselPressed();
}

void ScrubBarCallback( ScrubBarComponent *scrubbar, void *object, const float progress )
{
	( ( MoviePlayerView * )object )->ScrubBarClicked( progress );
}

void MoviePlayerView::CreateMenu( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font )
{
	BackgroundTintTexture.LoadTextureFromApplicationPackage( "assets/backgroundTint.png" );

	RWTexture.LoadTextureFromApplicationPackage( "assets/img_btn_rw.png" );
	RWHoverTexture.LoadTextureFromApplicationPackage( "assets/img_btn_rw_hover.png" );
	RWPressedTexture.LoadTextureFromApplicationPackage( "assets/img_btn_rw_pressed.png" );

	FFTexture.LoadTextureFromApplicationPackage( "assets/img_btn_ff.png" );
	FFHoverTexture.LoadTextureFromApplicationPackage( "assets/img_btn_ff_hover.png" );
	FFPressedTexture.LoadTextureFromApplicationPackage( "assets/img_btn_ff_pressed.png" );

	PlayTexture.LoadTextureFromApplicationPackage( "assets/img_btn_play.png" );
	PlayHoverTexture.LoadTextureFromApplicationPackage( "assets/img_btn_play_hover.png" );
	PlayPressedTexture.LoadTextureFromApplicationPackage( "assets/img_btn_play_pressed.png" );

	PauseTexture.LoadTextureFromApplicationPackage( "assets/img_btn_pause.png" );
	PauseHoverTexture.LoadTextureFromApplicationPackage( "assets/img_btn_pause_hover.png" );
	PausePressedTexture.LoadTextureFromApplicationPackage( "assets/img_btn_pause_pressed.png" );

	CarouselTexture.LoadTextureFromApplicationPackage( "assets/img_btn_carousel.png" );
	CarouselHoverTexture.LoadTextureFromApplicationPackage( "assets/img_btn_carousel_hover.png" );
	CarouselPressedTexture.LoadTextureFromApplicationPackage( "assets/img_btn_carousel_pressed.png" );

	SeekbarBackgroundTexture.LoadTextureFromApplicationPackage( "assets/img_seekbar_background.png" );
	SeekbarProgressTexture.LoadTextureFromApplicationPackage( "assets/img_seekbar_progress_blue.png" );

	SeekPosition.LoadTextureFromApplicationPackage( "assets/img_seek_position.png" );

	SeekFF2x.LoadTextureFromApplicationPackage( "assets/img_seek_ff2x.png" );
	SeekFF4x.LoadTextureFromApplicationPackage( "assets/img_seek_ff4x.png" );
	SeekFF8x.LoadTextureFromApplicationPackage( "assets/img_seek_ff8x.png" );
	SeekFF16x.LoadTextureFromApplicationPackage( "assets/img_seek_ff16x.png" );
	SeekFF32x.LoadTextureFromApplicationPackage( "assets/img_seek_ff32x.png" );

	SeekRW2x.LoadTextureFromApplicationPackage( "assets/img_seek_rw2x.png" );
	SeekRW4x.LoadTextureFromApplicationPackage( "assets/img_seek_rw4x.png" );
	SeekRW8x.LoadTextureFromApplicationPackage( "assets/img_seek_rw8x.png" );
	SeekRW16x.LoadTextureFromApplicationPackage( "assets/img_seek_rw16x.png" );
	SeekRW32x.LoadTextureFromApplicationPackage( "assets/img_seek_rw32x.png" );

    // ==============================================================================
    //
    // reorient message
    //
	MoveScreenMenu = new UIMenu( Cinema );
	MoveScreenMenu->Create( "MoviePlayerMenu" );
	MoveScreenMenu->SetFlags( VRMenuFlags_t( VRMENU_FLAG_TRACK_GAZE ) | VRMenuFlags_t( VRMENU_FLAG_BACK_KEY_DOESNT_EXIT ) );

    MoveScreenLabel.AddToMenu( MoveScreenMenu, NULL );
    MoveScreenLabel.SetLocalPose( VQuatf( V3Vectf( 0.0f, 1.0f, 0.0f ), 0.0f ), V3Vectf( 0.0f, 0.0f, -1.8f ) );
    MoveScreenLabel.GetMenuObject()->addFlags( VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );
    MoveScreenLabel.SetFontScale( 0.5f );
    MoveScreenLabel.SetText( CinemaStrings::MoviePlayer_Reorient );
    MoveScreenLabel.SetTextOffset( V3Vectf( 0.0f, -24 * VRMenuObject::DEFAULT_TEXEL_SCALE, 0.0f ) );  // offset to be below gaze cursor
    MoveScreenLabel.SetVisible( false );

    // ==============================================================================
    //
    // Playback controls
    //
    PlaybackControlsMenu = new UIMenu( Cinema );
    PlaybackControlsMenu->Create( "PlaybackControlsMenu" );
    PlaybackControlsMenu->SetFlags( VRMenuFlags_t( VRMENU_FLAG_BACK_KEY_DOESNT_EXIT ) );

    PlaybackControlsPosition.AddToMenu( PlaybackControlsMenu );
    PlaybackControlsScale.AddToMenu( PlaybackControlsMenu, &PlaybackControlsPosition );
    PlaybackControlsScale.SetLocalPosition( V3Vectf( 0.0f, 0.0f, 0.05f ) );
    PlaybackControlsScale.SetImage( 0, SURFACE_TEXTURE_DIFFUSE, BackgroundTintTexture, 1080, 1080 );

	// ==============================================================================
    //
    // movie title
    //
    MovieTitleLabel.AddToMenu( PlaybackControlsMenu, &PlaybackControlsScale );
    MovieTitleLabel.SetLocalPosition( PixelPos( 0, 266, 0 ) );
    MovieTitleLabel.SetFontScale( 1.4f );
    MovieTitleLabel.SetText( "" );
    MovieTitleLabel.SetTextOffset( V3Vectf( 0.0f, 0.0f, 0.01f ) );
    MovieTitleLabel.SetImage( 0, SURFACE_TEXTURE_DIFFUSE, BackgroundTintTexture, 320, 120 );

	// ==============================================================================
    //
    // seek icon
    //
    SeekIcon.AddToMenu( PlaybackControlsMenu, &PlaybackControlsScale );
    SeekIcon.SetLocalPosition( PixelPos( 0, 0, 0 ) );
    SeekIcon.SetLocalScale( V3Vectf( 2.0f ) );
    SetSeekIcon( 0 );

    // ==============================================================================
    //
    // controls
    //
    ControlsBackground.AddToMenuFlags( PlaybackControlsMenu, &PlaybackControlsScale, VRMenuObjectFlags_t( VRMENUOBJECT_RENDER_HIERARCHY_ORDER ) );
    ControlsBackground.SetLocalPosition( PixelPos( 0, -288, 0 ) );
    ControlsBackground.SetImage( 0, SURFACE_TEXTURE_DIFFUSE, BackgroundTintTexture, 1004, 168 );
    ControlsBackground.AddComponent( &GazeTimer );

    RewindButton.AddToMenu( PlaybackControlsMenu, &ControlsBackground );
    RewindButton.SetLocalPosition( PixelPos( -448, 0, 1 ) );
    RewindButton.SetLocalScale( V3Vectf( 2.0f ) );
    RewindButton.SetButtonImages( RWTexture, RWHoverTexture, RWPressedTexture );
    RewindButton.SetOnClick( RewindPressedCallback, this );

	FastForwardButton.AddToMenu( PlaybackControlsMenu, &ControlsBackground );
	FastForwardButton.SetLocalPosition( PixelPos( -234, 0, 1 ) );
    FastForwardButton.SetLocalScale( V3Vectf( 2.0f ) );
	FastForwardButton.SetButtonImages( FFTexture, FFHoverTexture, FFPressedTexture );
	FastForwardButton.SetOnClick( FastForwardPressedCallback, this );
    FastForwardButton.GetMenuObject()->setLocalBoundsExpand( V3Vectf::ZERO, PixelPos( -20, 0, 0 ) );

	// playbutton created after fast forward button to fix z issues
    PlayButton.AddToMenu( PlaybackControlsMenu, &ControlsBackground );
    PlayButton.SetLocalPosition( PixelPos( -341, 0, 2 ) );
    PlayButton.SetLocalScale( V3Vectf( 2.0f ) );
    PlayButton.SetButtonImages( PauseTexture, PauseHoverTexture, PausePressedTexture );
    PlayButton.SetOnClick( PlayPressedCallback, this );

	CarouselButton.AddToMenu( PlaybackControlsMenu, &ControlsBackground );
	CarouselButton.SetLocalPosition( PixelPos( 418, 0, 1 ) );
    CarouselButton.SetLocalScale( V3Vectf( 2.0f ) );
	CarouselButton.SetButtonImages( CarouselTexture, CarouselHoverTexture, CarouselPressedTexture );
	CarouselButton.SetOnClick( CarouselPressedCallback, this );
    CarouselButton.GetMenuObject()->setLocalBoundsExpand( PixelPos( 20, 0, 0 ), V3Vectf::ZERO );

	SeekbarBackground.AddToMenu( PlaybackControlsMenu, &ControlsBackground );
	SeekbarBackground.SetLocalPosition( PixelPos( 78, 0, 2 ) );
    SeekbarBackground.SetColor( V4Vectf( 0.5333f, 0.5333f, 0.5333f, 1.0f ) );
	SeekbarBackground.SetImage( 0, SURFACE_TEXTURE_DIFFUSE, SeekbarBackgroundTexture, ScrubBarWidth + 6, 46 );
	SeekbarBackground.AddComponent( &ScrubBar );

	SeekbarProgress.AddToMenu( PlaybackControlsMenu, &SeekbarBackground );
	SeekbarProgress.SetLocalPosition( PixelPos( 0, 0, 1 ) );
	SeekbarProgress.SetImage( 0, SURFACE_TEXTURE_DIFFUSE, SeekbarProgressTexture, ScrubBarWidth, 40 );
    SeekbarProgress.GetMenuObject()->addFlags( VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );

	CurrentTime.AddToMenu( PlaybackControlsMenu, &SeekbarBackground );
	CurrentTime.SetLocalPosition( PixelPos( -234, 52, 2 ) );
    CurrentTime.SetLocalScale( V3Vectf( 1.0f ) );
	CurrentTime.SetImage( 0, SURFACE_TEXTURE_DIFFUSE, SeekPosition );
	CurrentTime.SetText( "2:33:33" );
	CurrentTime.SetTextOffset( PixelPos( 0, 6, 1 ) );
	CurrentTime.SetFontScale( 0.71f );
    CurrentTime.SetColor( V4Vectf( 0 / 255.0f, 93 / 255.0f, 219 / 255.0f, 1.0f ) );
    CurrentTime.SetTextColor( V4Vectf( 1.0f, 1.0f, 1.0f, 1.0f ) );
    CurrentTime.GetMenuObject()->addFlags( VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );

	SeekTime.AddToMenu( PlaybackControlsMenu, &SeekbarBackground );
	SeekTime.SetLocalPosition( PixelPos( -34, 52, 4 ) );
    SeekTime.SetLocalScale( V3Vectf( 1.0f ) );
	SeekTime.SetImage( 0, SURFACE_TEXTURE_DIFFUSE, SeekPosition );
	SeekTime.SetText( "2:33:33" );
	SeekTime.SetTextOffset( PixelPos( 0, 6, 1 ) );
	SeekTime.SetFontScale( 0.71f );
    SeekTime.SetColor( V4Vectf( 47.0f / 255.0f, 70 / 255.0f, 89 / 255.0f, 1.0f ) );
    SeekTime.SetTextColor( V4Vectf( 1.0f, 1.0f, 1.0f, 1.0f ) );
    SeekTime.GetMenuObject()->addFlags( VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) );

	ScrubBar.SetWidgets( &SeekbarBackground, &SeekbarProgress, &CurrentTime, &SeekTime, ScrubBarWidth );
	ScrubBar.SetOnClick( ScrubBarCallback, this );
}

void MoviePlayerView::SetSeekIcon( const int seekSpeed )
{
	const UITexture * texture = NULL;

	switch( seekSpeed )
	{
		case -5 : texture = &SeekRW32x; break;
		case -4 : texture = &SeekRW16x; break;
		case -3 : texture = &SeekRW8x; break;
		case -2 : texture = &SeekRW4x; break;
		case -1 : texture = &SeekRW2x; break;

		default:
		case 0 : texture = NULL; break;

		case 1 : texture = &SeekFF2x; break;
		case 2 : texture = &SeekFF4x; break;
		case 3 : texture = &SeekFF8x; break;
		case 4 : texture = &SeekFF16x; break;
		case 5 : texture = &SeekFF32x; break;
	}

	if ( texture == NULL )
	{
	    SeekIcon.SetVisible( false );
	}
	else
	{
	    SeekIcon.SetVisible( true );
	    SeekIcon.SetImage( 0, SURFACE_TEXTURE_DIFFUSE, *texture );
	}
}

void MoviePlayerView::OnOpen()
{
	vInfo("OnOpen");
	CurViewState = VIEWSTATE_OPEN;

    Cinema.sceneMgr.ClearMovie();

	SeekSpeed = 0;
	PlaybackPos = 0;
	NextSeekTime = 0;

	SetSeekIcon( SeekSpeed );

	ScrubBar.SetProgress( 0.0f );

	RepositionScreen = false;
	MoveScreenAlpha.Set( 0, 0, 0, 0.0f );

	HideUI();
    Cinema.sceneMgr.LightsOff( 1.5f );

    Cinema.startMoviePlayback();

    MovieTitleLabel.SetText( Cinema.currentMovie()->Title );
    VBoxf titleBounds = MovieTitleLabel.GetTextLocalBounds( vApp->defaultFont() ) * VRMenuObject::TEXELS_PER_METER;
	MovieTitleLabel.SetImage( 0, SURFACE_TEXTURE_DIFFUSE, BackgroundTintTexture, titleBounds.GetSize().x + 88, titleBounds.GetSize().y + 32 );

	PlayButton.SetButtonImages( PauseTexture, PauseHoverTexture, PausePressedTexture );
}

void MoviePlayerView::OnClose()
{
	vInfo("OnClose");
	CurViewState = VIEWSTATE_CLOSED;
	HideUI();
	vApp->gazeCursor().ShowCursor();

	if ( MoveScreenMenu->IsOpen() )
	{
		MoveScreenLabel.SetVisible( false );
		MoveScreenMenu->Close();
	}

    Cinema.sceneMgr.ClearMovie();

    if ( Cinema.sceneMgr.VoidedScene )
	{
        Cinema.sceneMgr.SetSceneModel( Cinema.currentTheater() );
	}
}

/*
 * Command
 *
 * Actions that need to be performed on the render thread.
 */
bool MoviePlayerView::Command(const VEvent &event )
{
	if ( CurViewState != VIEWSTATE_OPEN )
	{
		return false;
	}

    if (event.name == "resume")
	{
        Cinema.setMovieSelection( false );
		return false;	// allow VrLib to handle it, too
    } else if (event.name == "pause") {
		Native::StopMovie( vApp );
		return false;	// allow VrLib to handle it, too
	}

	return false;
}

void MoviePlayerView::MovieLoaded( const int width, const int height, const int duration )
{
	ScrubBar.SetDuration( duration );
}

void MoviePlayerView::BackPressed()
{
	vInfo("BackPressed");
	HideUI();
    if ( Cinema.allowTheaterSelection() )
	{
		vInfo("Opening TheaterSelection");
        Cinema.theaterSelection();
	}
	else
	{
		vInfo("Opening MovieSelection");
        Cinema.setMovieSelection( true );
	}
}

bool MoviePlayerView::OnKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType )
{
	switch ( keyCode )
	{
		case AKEYCODE_BACK:
		{
			switch ( eventType )
			{
				case KeyState::KEY_EVENT_SHORT_PRESS:
				vInfo("KEY_EVENT_SHORT_PRESS");
				BackPressed();
				return true;
				break;
				default:
				//vInfo("unexpected back key state" << eventType);
				break;
			}
		}
		break;
		case AKEYCODE_MEDIA_NEXT:
			if ( eventType == KeyState::KEY_EVENT_UP )
			{
                Cinema.setMovie( Cinema.nextMovie() );
                Cinema.resumeOrRestartMovie();
			}
			break;
		case AKEYCODE_MEDIA_PREVIOUS:
			if ( eventType == KeyState::KEY_EVENT_UP )
			{
                Cinema.setMovie( Cinema.previousMovie() );
                Cinema.resumeOrRestartMovie();
			}
			break;
		break;
	}
	return false;
}

//=========================================================================================

static bool InsideUnit( const V2Vectf v )
{
	return v.x > -1.0f && v.x < 1.0f && v.y > -1.0f && v.y < 1.0f;
}

void MoviePlayerView::ShowUI()
{
	vInfo("ShowUI");
    Cinema.sceneMgr.ForceMono = true;
	vApp->gazeCursor().ShowCursor();

	PlaybackControlsMenu->Open();
	GazeTimer.SetGazeTime();

    PlaybackControlsScale.SetLocalScale( V3Vectf( Cinema.sceneMgr.GetScreenSize().y * ( 500.0f / 1080.0f ) ) );
    PlaybackControlsPosition.SetLocalPose( Cinema.sceneMgr.GetScreenPose() );

	uiActive = true;
}

void MoviePlayerView::HideUI()
{
	vInfo("HideUI");
	PlaybackControlsMenu->Close();

	vApp->gazeCursor().HideCursor();
    Cinema.sceneMgr.ForceMono = false;
	uiActive = false;

	SeekSpeed = 0;
	PlaybackPos = 0;
	NextSeekTime = 0;

	BackgroundClicked = false;

	SetSeekIcon( SeekSpeed );
}

void MoviePlayerView::CheckDebugControls( const VrFrame & vrFrame )
{
    if ( !Cinema.allowDebugControls )
	{
		return;
	}

#if 0
	if ( !( vrFrame.Input.buttonState & BUTTON_Y ) )
	{
		Cinema.SceneMgr.ChangeSeats( vrFrame );
	}
#endif

	if ( vrFrame.Input.buttonPressed & BUTTON_X )
	{
        Cinema.sceneMgr.ToggleLights( 0.5f );
	}

	if ( vrFrame.Input.buttonPressed & BUTTON_SELECT )
	{
        Cinema.sceneMgr.UseOverlay = !Cinema.sceneMgr.UseOverlay;
        vApp->createToast( "Overlay: %i", Cinema.sceneMgr.UseOverlay );
	}

	// Press Y to toggle FreeScreen mode, while holding the scale and distance can be adjusted
	if ( vrFrame.Input.buttonPressed & BUTTON_Y )
	{
        Cinema.sceneMgr.FreeScreenActive = !Cinema.sceneMgr.FreeScreenActive || Cinema.sceneMgr.SceneInfo.UseFreeScreen;
        Cinema.sceneMgr.PutScreenInFront();
	}

    if ( Cinema.sceneMgr.FreeScreenActive && ( vrFrame.Input.buttonState & BUTTON_Y ) )
	{
        Cinema.sceneMgr.FreeScreenDistance -= vrFrame.Input.sticks[0][1] * vrFrame.DeltaSeconds * 3;
        Cinema.sceneMgr.FreeScreenDistance = NervGear::VAlgorithm::Clamp( Cinema.sceneMgr.FreeScreenDistance, 1.0f, 4.0f );
        Cinema.sceneMgr.FreeScreenScale += vrFrame.Input.sticks[0][0] * vrFrame.DeltaSeconds * 3;
        Cinema.sceneMgr.FreeScreenScale = NervGear::VAlgorithm::Clamp( Cinema.sceneMgr.FreeScreenScale, 1.0f, 4.0f );

		if ( vrFrame.Input.buttonReleased & (BUTTON_LSTICK_UP|BUTTON_LSTICK_DOWN|BUTTON_LSTICK_LEFT|BUTTON_LSTICK_RIGHT) )
		{
            vApp->createToast( "FreeScreenDistance:%3.1f  FreeScreenScale:%3.1f", Cinema.sceneMgr.FreeScreenDistance, Cinema.sceneMgr.FreeScreenScale );
		}
	}
}

static V3Vectf	MatrixOrigin( const VR4Matrixf & m )
{
    return V3Vectf( -m.M[0][3], -m.M[1][3], -m.M[2][3] );
}

static V3Vectf	MatrixForward( const VR4Matrixf & m )
{
    return V3Vectf( -m.M[2][0], -m.M[2][1], -m.M[2][2] );
}

// -1 to 1 range on screenMatrix, returns -2,-2 if looking away from the screen
V2Vectf MoviePlayerView::GazeCoordinatesOnScreen( const VR4Matrixf & viewMatrix, const VR4Matrixf screenMatrix ) const
{
	// project along -Z in the viewMatrix onto the Z = 0 plane of screenMatrix
    const V3Vectf viewForward = MatrixForward( viewMatrix ).Normalized();

    V3Vectf screenForward;
    if ( Cinema.sceneMgr.FreeScreenActive )
	{
		// FIXME: free screen matrix is inverted compared to bounds screen matrix.
        screenForward = -V3Vectf( screenMatrix.M[0][2], screenMatrix.M[1][2], screenMatrix.M[2][2] ).Normalized();
	}
	else
	{
		screenForward = -MatrixForward( screenMatrix ).Normalized();
	}

	const float approach = viewForward.Dot( screenForward );
	if ( approach <= 0.1f )
	{
		// looking away
        return V2Vectf( -2.0f, -2.0f );
	}

    const VR4Matrixf panelInvert = screenMatrix.Inverted();
    const VR4Matrixf viewInvert = viewMatrix.Inverted();

    const V3Vectf viewOrigin = viewInvert.Transform( V3Vectf( 0.0f ) );
    const V3Vectf panelOrigin = MatrixOrigin( screenMatrix );

	// Should we disallow using panels from behind?
	const float d = panelOrigin.Dot( screenForward );
	const float t = -( viewOrigin.Dot( screenForward ) + d ) / approach;

    const V3Vectf impact = viewOrigin + viewForward * t;
    const V3Vectf localCoordinate = panelInvert.Transform( impact );

    return V2Vectf( localCoordinate.x, localCoordinate.y );
}

void MoviePlayerView::CheckInput( const VrFrame & vrFrame )
{
	if ( !uiActive && !RepositionScreen )
	{
		if ( ( vrFrame.Input.buttonPressed & BUTTON_A ) || ( ( vrFrame.Input.buttonReleased & BUTTON_TOUCH ) && !( vrFrame.Input.buttonState & BUTTON_TOUCH_WAS_SWIPE ) ) )
		{
			// open ui if it's not visible
			vApp->playSound( "touch_up" );
			ShowUI();

			// ignore button A or touchpad until release so we don't close the UI immediately after opening it
			UIOpened = true;
		}
	}

	if ( vrFrame.Input.buttonPressed & ( BUTTON_DPAD_LEFT | BUTTON_SWIPE_BACK ) )
	{
		if ( ( vrFrame.Input.buttonPressed & BUTTON_DPAD_LEFT ) || !GazeTimer.IsFocused() )
		{
			ShowUI();
			if ( SeekSpeed == 0 )
			{
				PauseMovie();
			}

			SeekSpeed--;
			if ( ( SeekSpeed == 0 ) || ( SeekSpeed < -MaxSeekSpeed ) )
			{
				SeekSpeed = 0;
				PlayMovie();
			}
			SetSeekIcon( SeekSpeed );

			vApp->playSound( "touch_up" );
		}
	}

	if ( vrFrame.Input.buttonPressed & ( BUTTON_DPAD_RIGHT | BUTTON_SWIPE_FORWARD ) )
	{
		if ( ( vrFrame.Input.buttonPressed & BUTTON_DPAD_RIGHT ) || !GazeTimer.IsFocused() )
		{
			ShowUI();
			if ( SeekSpeed == 0 )
			{
				PauseMovie();
			}

			SeekSpeed++;
			if ( ( SeekSpeed == 0 ) || ( SeekSpeed > MaxSeekSpeed ) )
			{
				SeekSpeed = 0;
				PlayMovie();
			}
			SetSeekIcon( SeekSpeed );

			vApp->playSound( "touch_up" );
		}
	}

    if ( Cinema.sceneMgr.FreeScreenActive )
	{
        const V2Vectf screenCursor = GazeCoordinatesOnScreen( Cinema.sceneMgr.Scene.CenterViewMatrix(), Cinema.sceneMgr.ScreenMatrix() );
		bool onscreen = false;
		if ( InsideUnit( screenCursor ) )
		{
			onscreen = true;
		}
		else if ( uiActive )
		{
			onscreen = GazeTimer.IsFocused();
		}

		if ( !onscreen )
		{
			// outside of screen, so show reposition message
            const double now = VTimer::Seconds();
			float alpha = MoveScreenAlpha.Value( now );
			if ( alpha > 0.0f )
			{
				MoveScreenLabel.SetVisible( true );
                MoveScreenLabel.SetTextColor( V4Vectf( alpha ) );
			}

			if ( vrFrame.Input.buttonPressed & ( BUTTON_A | BUTTON_TOUCH ) )
			{
				RepositionScreen = true;
			}
		}
		else
		{
			// onscreen, so hide message
            const double now = VTimer::Seconds();
			MoveScreenAlpha.Set( now, -1.0f, now + 1.0f, 1.0f );
			MoveScreenLabel.SetVisible( false );
		}
	}

	// while we're holding down the button or touchpad, reposition screen
	if ( RepositionScreen )
	{
		if ( vrFrame.Input.buttonState & ( BUTTON_A | BUTTON_TOUCH ) )
		{
            Cinema.sceneMgr.PutScreenInFront();
		}
		else
		{
			RepositionScreen = false;
		}
	}

	if ( vrFrame.Input.buttonPressed & BUTTON_START )
	{
		TogglePlayback();
	}

	if ( vrFrame.Input.buttonPressed & BUTTON_SELECT )
	{
		// movie select
		vApp->playSound( "touch_up" );
        Cinema.setMovieSelection( false );
	}

	if ( vrFrame.Input.buttonPressed & BUTTON_B )
	{
		if ( !uiActive )
		{
			BackPressed();
		}
		else
		{
			vInfo("User pressed button 2");
			vApp->playSound( "touch_up" );
			HideUI();
			PlayMovie();
		}
	}
}

void MoviePlayerView::TogglePlayback()
{
	const bool isPlaying = Native::IsPlaying( vApp );
	if ( isPlaying )
	{
		PauseMovie();
	}
	else
	{
		PlayMovie();
	}
}

void MoviePlayerView::PauseMovie()
{
	Native::PauseMovie( vApp );
	PlaybackPos = Native::GetPosition( vApp );
	PlayButton.SetButtonImages( PlayTexture, PlayHoverTexture, PlayPressedTexture );
}

void MoviePlayerView::PlayMovie()
{
	SeekSpeed = 0;
	SetSeekIcon( SeekSpeed );
	Native::ResumeMovie( vApp );
	PlayButton.SetButtonImages( PauseTexture, PauseHoverTexture, PausePressedTexture );
}

void MoviePlayerView::RewindPressed()
{
	// rewind
	if ( SeekSpeed == 0 )
	{
		PauseMovie();
	}

	SeekSpeed--;
	if ( ( SeekSpeed == 0 ) || ( SeekSpeed < -MaxSeekSpeed ) )
	{
		SeekSpeed = 0;
		PlayMovie();
	}
	SetSeekIcon( SeekSpeed );
}

void MoviePlayerView::FastForwardPressed()
{
	// fast forward
	if ( SeekSpeed == 0 )
	{
		PauseMovie();
	}

	SeekSpeed++;
	if ( ( SeekSpeed == 0 ) || ( SeekSpeed > MaxSeekSpeed ) )
	{
		SeekSpeed = 0;
		PlayMovie();
	}
	SetSeekIcon( SeekSpeed );
}

void MoviePlayerView::CarouselPressed()
{
    Cinema.setMovieSelection( false );
}

void MoviePlayerView::ScrubBarClicked( const float progress )
{
	// if we're rw/ff'ing, then stop and resume playback
	if ( SeekSpeed != 0 )
	{
		SeekSpeed = 0;
		PlayMovie();
		SetSeekIcon( SeekSpeed );
		NextSeekTime = 0;
	}

	// choke off the amount position changes we send to the media player
    const double now = VTimer::Seconds();
	if ( now <= NextSeekTime )
	{
		return;
	}

    int position = Cinema.sceneMgr.MovieDuration * progress;
	Native::SetPosition( vApp, position );

	ScrubBar.SetProgress( progress );

    NextSeekTime = VTimer::Seconds() + 0.1;
}

void MoviePlayerView::UpdateUI( const VrFrame & vrFrame )
{
	if ( uiActive )
	{
        double timeSinceLastGaze = VTimer::Seconds() - GazeTimer.GetLastGazeTime();
		if ( !ScrubBar.IsScrubbing() && ( SeekSpeed == 0 ) && ( timeSinceLastGaze > GazeTimeTimeout ) )
		{
			vInfo("Gaze timeout");
			HideUI();
			PlayMovie();
		}

		// if we press the touchpad or a button outside of the playback controls, then close the UI
		if ( ( ( vrFrame.Input.buttonPressed & BUTTON_A ) != 0 ) || ( ( vrFrame.Input.buttonPressed & BUTTON_TOUCH ) != 0 ) )
		{
			// ignore button A or touchpad until release so we don't close the UI immediately after opening it
			BackgroundClicked = !GazeTimer.IsFocused() && !UIOpened;
		}

		if ( ( ( vrFrame.Input.buttonReleased & BUTTON_A ) != 0 ) ||
			( ( ( vrFrame.Input.buttonReleased & BUTTON_TOUCH ) != 0 ) && ( ( vrFrame.Input.buttonState & BUTTON_TOUCH_WAS_SWIPE ) == 0 ) )	)
		{
			if ( !GazeTimer.IsFocused() && BackgroundClicked )
			{
				vInfo("Clicked outside playback controls");
				vApp->playSound( "touch_up" );
				HideUI();
				PlayMovie();
			}
			BackgroundClicked = false;
		}

        if ( Cinema.sceneMgr.MovieDuration > 0 )
		{
			const int currentPosition = Native::GetPosition( vApp );
            float progress = ( float )currentPosition / ( float )Cinema.sceneMgr.MovieDuration;
			ScrubBar.SetProgress( progress );
		}

        if ( Cinema.sceneMgr.FreeScreenActive )
		{
			// update the screen position & size;
            PlaybackControlsScale.SetLocalScale( V3Vectf( Cinema.sceneMgr.GetScreenSize().y * ( 500.0f / 1080.0f ) ) );
            PlaybackControlsPosition.SetLocalPose( Cinema.sceneMgr.GetScreenPose() );
		}
	}

	// clear the flag for ignoring button A or touchpad until release
	UIOpened = false;
}

/*
 * DrawEyeView
 */
VR4Matrixf MoviePlayerView::DrawEyeView( const int eye, const float fovDegrees )
{
    return Cinema.sceneMgr.DrawEyeView( eye, fovDegrees );
}

/*
 * Frame()
 *
 * App override
 */
VR4Matrixf MoviePlayerView::Frame( const VrFrame & vrFrame )
{
	// Drop to 2x MSAA during playback, people should be focused
	// on the high quality screen.
    VEyeBuffer::Settings eyeParms = vApp->eyeParms();
	eyeParms.multisamples = 2;
	vApp->setEyeParms( eyeParms );

	if ( Native::HadPlaybackError( vApp ) )
	{
		vInfo("Playback failed");
        Cinema.unableToPlayMovie();
	}
	else if ( Native::IsPlaybackFinished( vApp ) )
	{
		vInfo("Playback finished");
        Cinema.movieFinished();
	}

	CheckInput( vrFrame );
	CheckDebugControls( vrFrame );
	UpdateUI( vrFrame );

    if ( Cinema.sceneMgr.FreeScreenActive && !MoveScreenMenu->IsOpen() )
	{
		MoveScreenMenu->Open();
	}
    else if ( !Cinema.sceneMgr.FreeScreenActive && MoveScreenMenu->IsOpen() )
	{
		MoveScreenMenu->Close();
	}

	if ( SeekSpeed != 0 )
	{
        const double now = VTimer::Seconds();
		if ( now > NextSeekTime )
		{
			int PlaybackSpeed = ( SeekSpeed < 0 ) ? -( 1 << -SeekSpeed ) : ( 1 << SeekSpeed );
			PlaybackPos += 250 * PlaybackSpeed;
			Native::SetPosition( vApp, PlaybackPos );
			NextSeekTime = now + 0.25;
		}
	}

    return Cinema.sceneMgr.Frame( vrFrame );
}

/*************************************************************************************/

ControlsGazeTimer::ControlsGazeTimer() :
	VRMenuComponent( VRMenuEventFlags_t( VRMENU_EVENT_FRAME_UPDATE ) |
			VRMENU_EVENT_FOCUS_GAINED |
            VRMENU_EVENT_FOCUS_LOST ),
    LastGazeTime( 0 ),
    HasFocus( false )

{
}

void ControlsGazeTimer::SetGazeTime()
{
    LastGazeTime = VTimer::Seconds();
}

eMsgStatus ControlsGazeTimer::onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
    switch( event.eventType )
    {
    	case VRMENU_EVENT_FRAME_UPDATE:
    		if ( HasFocus )
    		{
                LastGazeTime = VTimer::Seconds();
    		}
    		return MSG_STATUS_ALIVE;
        case VRMENU_EVENT_FOCUS_GAINED:
        	HasFocus = true;
            LastGazeTime = VTimer::Seconds();
    		return MSG_STATUS_ALIVE;
        case VRMENU_EVENT_FOCUS_LOST:
        	HasFocus = false;
    		return MSG_STATUS_ALIVE;
        default:
            vAssert( !"Event flags mismatch!" );
            return MSG_STATUS_ALIVE;
    }
}

/*************************************************************************************/

ScrubBarComponent::ScrubBarComponent() :
	VRMenuComponent( VRMenuEventFlags_t( VRMENU_EVENT_TOUCH_DOWN ) |
		VRMENU_EVENT_TOUCH_DOWN |
		VRMENU_EVENT_FRAME_UPDATE |
		VRMENU_EVENT_FOCUS_GAINED |
        VRMENU_EVENT_FOCUS_LOST ),
	HasFocus( false ),
	TouchDown( false ),
	Progress( 0.0f ),
	Duration( 0 ),
	Background( NULL ),
	ScrubBar( NULL ),
	CurrentTime( NULL ),
	SeekTime( NULL ),
	OnClickFunction( NULL ),
	OnClickObject( NULL )

{
}

void ScrubBarComponent::SetDuration( const int duration )
{
	Duration = duration;

	SetProgress( Progress );
}

void ScrubBarComponent::SetOnClick( void ( *callback )( ScrubBarComponent *, void *, float ), void *object )
{
	OnClickFunction = callback;
	OnClickObject = object;
}

void ScrubBarComponent::SetWidgets( UIWidget *background, UIWidget *scrubBar, UILabel *currentTime, UILabel *seekTime, const int scrubBarWidth )
{
	Background 		= background;
	ScrubBar 		= scrubBar;
	CurrentTime 	= currentTime;
	SeekTime 		= seekTime;
	ScrubBarWidth	= scrubBarWidth;

	SeekTime->SetVisible( false );
}

void ScrubBarComponent::SetProgress( const float progress )
{
	Progress = progress;
	const float seekwidth = ScrubBarWidth * progress;

    V3Vectf pos = ScrubBar->GetLocalPosition();
	pos.x = PixelScale( ( ScrubBarWidth - seekwidth ) * -0.5f );
	ScrubBar->SetLocalPosition( pos );
    ScrubBar->SetSurfaceDims( 0, V2Vectf( seekwidth, 40.0f ) );
	ScrubBar->RegenerateSurfaceGeometry( 0, false );

	pos = CurrentTime->GetLocalPosition();
	pos.x = PixelScale( ScrubBarWidth * -0.5f + seekwidth );
	CurrentTime->SetLocalPosition( pos );
	SetTimeText( CurrentTime, Duration * progress );
}

void ScrubBarComponent::SetTimeText( UILabel *label, const int time )
{
	int seconds = time / 1000;
	int minutes = seconds / 60;
	int hours = minutes / 60;
	seconds = seconds % 60;
	minutes = minutes % 60;

    VString text;
    if (hours > 0) {
        text.sprintf("%d:%02d:%02d", hours, minutes, seconds);
    } else if (minutes > 0) {
        text.sprintf("%d:%02d", minutes, seconds);
    } else {
        text.sprintf("0:%02d", seconds);
	}
    label->SetText(text);
}

eMsgStatus ScrubBarComponent::onEventImpl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
    switch( event.eventType )
    {
		case VRMENU_EVENT_FOCUS_GAINED:
			HasFocus = true;
			return MSG_STATUS_ALIVE;

		case VRMENU_EVENT_FOCUS_LOST:
			HasFocus = false;
			return MSG_STATUS_ALIVE;

    	case VRMENU_EVENT_TOUCH_DOWN:
    		TouchDown = true;
    		OnClick( app, vrFrame, event );
    		return MSG_STATUS_ALIVE;

    	case VRMENU_EVENT_FRAME_UPDATE:
    		return OnFrame( app, vrFrame, menuMgr, self, event );

        default:
            vAssert( !"Event flags mismatch!" );
            return MSG_STATUS_ALIVE;
    }
}

eMsgStatus ScrubBarComponent::OnFrame( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
	if ( TouchDown )
	{
		if ( ( vrFrame.Input.buttonState & ( BUTTON_A | BUTTON_TOUCH ) ) != 0 )
		{
			OnClick( app, vrFrame, event );
		}
		else
		{
			TouchDown = false;
		}
	}

	SeekTime->SetVisible( HasFocus );
	if ( HasFocus )
	{
        V3Vectf hitPos = event.hitResult.RayStart + event.hitResult.RayDir * event.hitResult.t;

		// move hit position into local space
        const VPosf modelPose = Background->GetWorldPose();
        V3Vectf localHit = modelPose.Orientation.Inverted().Rotate( hitPos - modelPose.Position );

        VBoxf bounds = Background->GetMenuObject()->getTextLocalBounds( app->defaultFont() ) * Background->GetParent()->GetWorldScale();
		const float progress = ( localHit.x - bounds.GetMins().x ) / bounds.GetSize().x;

		if ( ( progress >= 0.0f ) && ( progress <= 1.0f ) )
		{
			const float seekwidth = ScrubBarWidth * progress;
            V3Vectf pos = SeekTime->GetLocalPosition();
			pos.x = PixelScale( ScrubBarWidth * -0.5f + seekwidth );
			SeekTime->SetLocalPosition( pos );

			SetTimeText( SeekTime, Duration * progress );
		}
	}

	return MSG_STATUS_ALIVE;
}

void ScrubBarComponent::OnClick( App * app, VrFrame const & vrFrame, VRMenuEvent const & event )
{
	if ( OnClickFunction == NULL )
	{
		return;
	}

    V3Vectf hitPos = event.hitResult.RayStart + event.hitResult.RayDir * event.hitResult.t;

	// move hit position into local space
    const VPosf modelPose = Background->GetWorldPose();
    V3Vectf localHit = modelPose.Orientation.Inverted().Rotate( hitPos - modelPose.Position );

    VBoxf bounds = Background->GetMenuObject()->getTextLocalBounds( app->defaultFont() ) * Background->GetParent()->GetWorldScale();
	const float progress = ( localHit.x - bounds.GetMins().x ) / bounds.GetSize().x;
	if ( ( progress >= 0.0f ) && ( progress <= 1.0f ) )
	{
		( *OnClickFunction )( this, OnClickObject, progress );
	}
}

} // namespace OculusCinema
