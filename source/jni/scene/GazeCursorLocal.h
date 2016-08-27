/************************************************************************************

Filename    :   GazeCursorLocal.h
Content     :   Global gaze cursor.
Created     :   June 6, 2016
Authors     :   Zhangxin

Copyright   :   Copyright 2016 VRSeen VR, LLC. All Rights reserved.

*************************************************************************************/

#if !defined( V_GazeCursorLocal_h )
#define V_GazeCursorLocal_h

#include "api/VEglDriver.h"

#include "GazeCursor.h"
#include "../api/VGlShader.h"
#include "../api/VGlGeometry.h"

NV_NAMESPACE_BEGIN

//==============================================================
// VGazeCursorLocal
//
// Private implementation of the gaze cursor interface.
class VGazeCursorLocal : public VGazeCursor
{
public:
	static const float	CURSOR_MAX_DIST;
	static const int	TRAIL_GHOSTS = 16;

                                VGazeCursorLocal();
    virtual						~VGazeCursorLocal();

	// Initialize the gaze cursor system.
	virtual	void				Init();

	// Shutdown the gaze cursor system.
	virtual void				Shutdown();

	// This should be called at the beginning of the frame before and system updates the gaze cursor.
	virtual	void				BeginFrame();

	// Each system using the gaze cursor should call this once to get its own id.
	virtual gazeCursorUserId_t	GenerateUserId();

	// Updates the gaze cursor distance if ths distance passed is less than the current
	// distance.  System that use the gaze cursor should use this method so that they
	// interact civilly with other systems using the gaze cursor.
	virtual void				UpdateForUser( gazeCursorUserId_t const userId, float const d,
										eGazeCursorStateType const state );

	// Call when the scene changes or the camera moves a large amount to clear out the cursor trail
	virtual void				ClearGhosts();

	// Called once per frame to update logic.
    virtual	void				Frame( VMatrix4f const & viewMatrix, float const deltaTime );

	// Renders the gaze cursor.
    virtual void				Render( int const eye, VMatrix4f const & mvp ) const;

	// Users should call this function to determine if the gaze cursor is relevant for them.
	virtual bool				IsActiveForUser( gazeCursorUserId_t const userId ) const;

	// Returns the current info about the gaze cursor.
    virtual VGazeCursorInfo	GetInfo() const;

	// Force the distance to a specific value -- this will set the distance even if
	// it is further away than the current distance. Unless your intent is to overload
	// the distance set by all other systems that use the gaze cursor, don't use this.
	virtual void				ForceDistance( gazeCursorUserId_t const userId, float const d );

	// Sets the rate at which the gaze cursor icon will spin.
	virtual void				SetRotationRate( float const degreesPerSec );

	// Sets the scale factor for the cursor's size.
	virtual void				SetCursorScale( float const scale );

	// Hide the gaze cursor.
	virtual void				HideCursor() { Hidden = true; }

	// Show the gaze cursor.
	virtual void				ShowCursor() { Hidden = false; }

	// Hide the gaze cursor for specified frames
	virtual void				HideCursorForFrames( const int hideFrames ) { HiddenFrames = hideFrames; }

	// Sets an addition distance to offset the cursor for rendering. This can help avoid
	// z-fighting but also helps the cursor to feel more 3D by pushing it away from surfaces.
	virtual void				SetDistanceOffset( float const offset ) { DistanceOffset = offset; }

	// Start a timer that will be shown animating the cursor.
	virtual void				StartTimer( float const durationSeconds,
										float const timeBeforeShowingTimer );

	// Cancels the timer if it's active.
	virtual void				CancelTimer();

private:
	int							NextUserId;				// id of the next user to request an id
    VGazeCursorInfo             Info;					// current cursor info
	float						CursorRotation;			// current cursor rotation
	float						RotationRateRadians;	// rotation rate in radians
	float						CursorScale;			// scale of the cursor
	float						DistanceOffset;			// additional distance to offset towards the camera.
	int							HiddenFrames;			// Hide cursor for a number of frames
    VMatrix4f					CursorTransform[TRAIL_GHOSTS];	// transform for each ghost
    VMatrix4f					CursorScatterTransform[TRAIL_GHOSTS];	// transform for each depth-fail ghost
	int							CurrentTransform;		// the next CursorTransform[] to fill
    VMatrix4f					TimerTransform;			// current transform of the timing cursor
    VVect2f                     ColorTableOffset;		// offset into color table for color-cycling effects

	double						TimerShowTime;			// time when the timer cursor should show
	double						TimerEndTime;			// time when the timer will expire

	VGlGeometry					CursorGeometry;			// VBO for the cursor
	GLuint						CursorTextureHandle[CURSOR_STATE_MAX];	// handle to the cursor's texture
	GLuint						TimerTextureHandle;		// handle to the texture for the timer
	GLuint						ColorTableHandle;		// handle to the cursor's color table texture
	VGlShader					CursorProgram;			// vertex and pixel shaders for the cursor
	VGlShader					TimerProgram;			// vertex and pixel shaders for the timer

	bool						Initialized;			// true once initialized
	bool						Hidden;					// true if the cursor should not render
	bool						IsActive;				// true if any system set a distance on the cursor
	bool						HasUser;				// true if any system is currently updating the cursor

private:
	void						ResetCursor();

	bool						TimerActive() const;
};

NV_NAMESPACE_END

#endif  // OVR_GazeCursorLocal_h
