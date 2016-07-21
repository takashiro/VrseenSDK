#pragma once

#include "VRect3.h"
#include "ModelView.h"

NV_USING_NAMESPACE

class PanoCinema;

enum MovieFormat
{
	VT_UNKNOWN,
	VT_2D,
	VT_LEFT_RIGHT_3D,			// Left & right are scaled horizontally by 50%.
	VT_LEFT_RIGHT_3D_FULL,		// Left & right are unscaled.
	VT_TOP_BOTTOM_3D,			// Top & bottom are scaled vertically by 50%.
	VT_TOP_BOTTOM_3D_FULL,		// Top & bottom are unscaled.
};

class SceneManager
{
public:
						SceneManager( PanoCinema &app_ );

    void 				OneTimeInit(const VString &);
	void				OneTimeShutdown();

    VMatrix4f 			DrawEyeView( const int eye, const float fovDegrees );

    bool Command(const VEvent &event);

    VMatrix4f 			Frame( const VFrame & vrFrame );

	void 				SetSeat( int newSeat );
	bool 				ChangeSeats( const VFrame & vrFrame );

	void 				ClearMovie();
	void 				PutScreenInFront();

	void				ClearGazeCursorGhosts();  	// clear gaze cursor to avoid seeing it lerp

    VVect2f			GetScreenSize() const;

    void 				SetFreeScreenAngles( const VVect3f &angles );
    VVect3f			GetFreeScreenScale() const;

    VMatrix4f			FreeScreenMatrix() const;
    VMatrix4f 			BoundsScreenMatrix(const VRect3f &bounds, const float movieAspect) const;
    VMatrix4f 			ScreenMatrix() const;

	void				AllowMovement( bool allow ) { AllowMove = allow; }
	bool				MovementAllowed() const { return AllowMove; }

	bool				GetUseOverlay() const;

public:
	PanoCinema &			Cinema;


	bool				UseOverlay;

	SurfaceTexture	* 	MovieTexture;
	long long			MovieTextureTimestamp;

	// FreeScreen mode allows the screen to be oriented arbitrarily, rather
	// than on a particular surface in the scene.
	bool				FreeScreenActive;
	float				FreeScreenScale;
	float				FreeScreenDistance;
    VMatrix4f			FreeScreenOrientation;
    VVect3f			FreeScreenAngles;

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

	OvrSceneView		Scene;

	static const int	MAX_SEATS = 8;
    VVect3f SceneSeatPositions[MAX_SEATS];
	int					SceneSeatCount;
	int					SeatPosition;

    VMatrix4f SceneScreenMatrix;
    VRect3f SceneScreenBounds;

	bool 				AllowMove;
private:
	GLuint 				BuildScreenVignetteTexture( const int horizontalTile ) const;
	int 				BottomMipLevel( const int width, const int height ) const;
	void 				ClampScreenToView();
};


