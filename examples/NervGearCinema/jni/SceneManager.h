#pragma once

#include "MovieManager.h"
#include "ModelView.h"
#include "Lerp.h"

using namespace NervGear;

namespace OculusCinema {

class CinemaApp;

class SceneManager
{
public:
						SceneManager( CinemaApp &app_ );

    void 				OneTimeInit( const VString &launchIntent );
	void				OneTimeShutdown();

    VR4Matrixf 			DrawEyeView( const int eye, const float fovDegrees );

    bool Command(const VEvent &event);

    VR4Matrixf 			Frame( const VrFrame & vrFrame );

	void 				SetSeat( int newSeat );
	bool 				ChangeSeats( const VrFrame & vrFrame );

	void 				ClearMovie();
	void 				PutScreenInFront();

	void				ClearGazeCursorGhosts();  	// clear gaze cursor to avoid seeing it lerp

	void 				ToggleLights( const float duration );
	void 				LightsOn( const float duration );
	void 				LightsOff( const float duration );

	void				SetSceneModel( const SceneDef &sceneDef );
	void				SetSceneProgram( const sceneProgram_t opaqueProgram, const sceneProgram_t additiveProgram );

    VPosf				GetScreenPose() const;
    V2Vectf			GetScreenSize() const;

    void 				SetFreeScreenAngles( const V3Vectf &angles );
    V3Vectf			GetFreeScreenScale() const;

    VR4Matrixf			FreeScreenMatrix() const;
    VR4Matrixf 			BoundsScreenMatrix( const VBoxf & bounds, const float movieAspect ) const;
    VR4Matrixf 			ScreenMatrix() const;

	void				AllowMovement( bool allow ) { AllowMove = allow; }
	bool				MovementAllowed() const { return AllowMove; }

	bool				GetUseOverlay() const;

public:
	CinemaApp &			Cinema;

	// Allow static lighting to be faded up or down
	Lerp				StaticLighting;

	bool				UseOverlay;

	SurfaceTexture	* 	MovieTexture;
	long long			MovieTextureTimestamp;

	// FreeScreen mode allows the screen to be oriented arbitrarily, rather
	// than on a particular surface in the scene.
	bool				FreeScreenActive;
	float				FreeScreenScale;
	float				FreeScreenDistance;
    VR4Matrixf			FreeScreenOrientation;
    V3Vectf			FreeScreenAngles;

	// don't make these bool, or sscanf %i will trash adjacent memory!
	int					ForceMono;			// only show the left eye of 3D movies

	// Set when MediaPlayer knows what the stream size is.
	// current is the aspect size, texture may be twice as wide or high for 3D content.
	int					CurrentMovieWidth;	// set to 0 when a new movie is started, don't render until non-0
	int					CurrentMovieHeight;
	int					MovieTextureWidth;
	int					MovieTextureHeight;
	MovieFormat			CurrentMovieFormat;
	int					MovieRotation;
	int					MovieDuration;

	bool				FrameUpdateNeeded;
	int					ClearGhostsFrames;

	VGlGeometry			UnitSquare;		// -1 to 1

	// We can't directly create a mip map on the OES_external_texture, so
	// it needs to be copied to a conventional texture.
	// It must be triple buffered for use as a TimeWarp overlay plane.
	int					CurrentMipMappedMovieTexture;	// 0 - 2
	GLuint				MipMappedMovieTextures[3];
	GLuint				MipMappedMovieFBOs[3];

	GLuint				ScreenVignetteTexture;
	GLuint				ScreenVignetteSbsTexture;	// for side by side 3D

	sceneProgram_t		SceneProgramIndex;

	OvrSceneView		Scene;
	SceneDef			SceneInfo;
	SurfaceDef *		SceneScreenSurface;		// override this to the movie texture
	const ModelTag *	SceneScreenTag;			//

	static const int	MAX_SEATS = 8;
    V3Vectf			SceneSeatPositions[MAX_SEATS];
	int					SceneSeatCount;
	int					SeatPosition;

    VR4Matrixf			SceneScreenMatrix;
    VBoxf			SceneScreenBounds;

	bool 				AllowMove;

	bool				VoidedScene;

private:
	GLuint 				BuildScreenVignetteTexture( const int horizontalTile ) const;
	int 				BottomMipLevel( const int width, const int height ) const;
	void 				ClampScreenToView();
};

} // namespace OculusCinema

