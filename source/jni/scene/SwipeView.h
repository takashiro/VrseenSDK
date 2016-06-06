#pragma once

#include "vglobal.h"

#include "VBasicmath.h"
#include "VArray.h"
#include "../api/VGlGeometry.h"
#include "../api/VGlShader.h"
#include "VTexture.h"
#include "GazeCursor.h"
#include "VFrame.h"
#include "BitmapFont.h"

#pragma once

NV_NAMESPACE_BEGIN

class OvrGazeCursor;

// Panels are NOT freed by the SwipeView; the owner is responsible.
class SwipePanel
{
public:
	SwipePanel() : Identifier( NULL ), Text( NULL ), Texture( 0 ), Id( -1 ), SelectState( 0.0f ) {}

	void *		Identifier;
	const char * Text;
    V2Vectf	Size;			// on a unit circle (radians)
	GLuint		Texture;
	int			Id;				// unique id

	float		SelectState;	// 0.0 = on the wall, 1.0 = fully selected
};

enum SwipeViewState
{
	SVS_CLOSED,
	SVS_OPENING,
	SVS_OPEN,
	SVS_CLOSING
};

enum SwipeViewTouchState
{
	SVT_ACTIVATION,		// must release before anything else
	SVT_WILL_DRAG,		// touch down will go to drag
	SVT_WILL_ACTION,	// touch down will go to action
	SVT_DRAG,			// release will do nothing
	SVT_ACTION,			// release will do action
	// action can convert to drag, but not the other way

	SVT_NUM_STATES
};

class SwipeAction
{
public:
	SwipeViewState		ViewState;		// state of entire view

	SwipeViewTouchState	TouchState;		// change gaze cursor based on state

	bool	PlaySndSelect;			// gaze goes on panel
	bool	PlaySndDeselect;		// gaze goes off panel

	bool	PlaySndTouchActive;		// touch down on active panel
	bool	PlaySndTouchInactive;	// touch down in inactive area for drag
	bool	PlaySndActiveToDrag;	// dragged enough to change from active to drag
	bool	PlaySndRelease;			// release wihout action
	bool	PlaySndReleaseAction;	// release and action
    bool    PlaySndSwipeRelease;    // released a swipe

	int		ActivatePanelIndex;		// -1 = no action
};

class SwipeView
{
public:
	SwipeView();
	~SwipeView();

	void		Init( OvrGazeCursor & gazeCursor );
	SwipeAction	Frame( OvrGazeCursor & gazeCursor, BitmapFont const & font, BitmapFontSurface & fontSurface,
                        const VFrame & vrFrame, const VR4Matrixf & view, const bool allowSwipe );
    void		Draw( const VR4Matrixf & mvp );

	// The offset will be left where it was, unless
	// the view has never been opened or was closed far enough away from view
	void		Activate( );

	void		Close();

	void		ClearPanelSelectState();
	float		PanelAngleY( int panelY );
	int			LayoutColumns() const;

	eGazeCursorStateType	GazeCursorType() const;

	// True by default -- drags the panels near the edge of the view
	// if you look away from all of them.  Some scenes may want to
	// leave them fixed in place.
	bool		ClampPanelOffset;

	bool		CloseOnNextFrame;
	bool		ActivateOnNextFrame;

	// Swiping will adjust the offset.
	//
	// Turning will simultaneously adjust ForwardYaw and the
	// offset so the panels stay in the same world location,
	// but turning 360 degrees will display different panels.
	//
	// If the view would look far enough off the side to put
	// all panels off the screen, the key yaw will be dragged with
	// the view without changing the offset, pulling the panels
	// with it.
	//
	// Subsequent panels are drawn to the right of previous panels,
	// which is more negative yaw.
    V3Vectf	ForwardAtOffset;		// world space vector
	float		ForwardYaw;				// increases with rotation to the left, can wrapped around
	float		Offset;					// Offset in the panel list at ForwardYaw
	float		Velocity;				// on a unit circle (radians)

	// The touchpad events don't come in very synchronized with the frames, so
	// average over several frames for velocity.
	static const int MAX_TOUCH_HISTORY = 4;
    V2Vectf	TouchPos[MAX_TOUCH_HISTORY];
	double		TimeHistory[MAX_TOUCH_HISTORY];
	int			HistoryIndex;

    V3Vectf	StartViewOrigin;

	VGlShader	ProgPanel;
	VGlShader	ProgHighlight;
	VGlGeometry	GeoPanel;
	VTexture	BorderTexture2_1;
	VTexture	BorderTexture1_1;

	SwipeViewState		State;
	SwipeViewTouchState	TouchState;

	double		PressTime;			// time when touch went down, not needed now, perhaps for long-press

    V2Vectf	TouchPoint;			// Where touch went down
    V2Vectf	TouchGazePos;		// Where touch went down
    V2Vectf	PrevTouch;			// Touch position at last frame
	bool		PreviousButtonState;// union of touch and joystick button


	bool		HasMoved;			// a release will be a tap if not moved
	bool		ActivationPress;	// no drag until release

    V2Vectf	GazePos;			// on a unit circle (radians)

    V2Vectf	Radius;				// if == distance, view is in center of curve. If > distance, view is near edge

    V2Vectf	SlotSize;			// on a unit circle (radians)
	int			LayoutRows;
	float		RowOffset;
	VArray<SwipePanel>	Panels;

	bool		EverOpened;			// if it has been opened before, subsequent opens can return to same position
	int			AnimationCenterPanel[2];
	float		AnimationFraction;	// goes 0 to 1 for opening and 1 to 0 for closing
	double		AnimationStartTime;

	int			SelectedPanel;	// put the highlight behind this one

	// Page-swipe
	float		PageSwipeSeconds;
	float		PageSwipeDir;

	// Behavior tuning, can be adjusted on a per-object basis
	float		Distance;
	float		SelectDistance;		// when selected, Radius will be reduced by this much
	float		TapMaxDistance;		// before transitioning from action to drag
	float		GazeMaxDistance;	// before transitioning from action to drag
	float		SpeedScale;
	float		Friction;
	float		SnapSpeed;
	float		SelectTime;			// time to go to or from selected state
	float		HighlightSlotFraction;
	float		MaxVelocity;		// radians per second
	float		MinVelocity;		// radians per second
	float		VelocityMixTime;	// half of the velocity is mixed in this time
	float		OpenAnimationTime;

	gazeCursorUserId_t	GazeUserId;	// id unique to this swipe view for interacting with gaze cursor

	class PanelRenderInfo
	{
	public:
		PanelRenderInfo() :
			PanelIndex( -1 ),
			Selected( false )
		{
		}

        VR4Matrixf		Mat;
		int				PanelIndex;
		bool			Selected;
	};

	VArray< PanelRenderInfo >	PanelRenderList;	// transforms for all panels, calculated in Frame() so it's not done for each eye in Draw()
    VR4Matrixf					SelectionTransform;	// transform of the selection highlight
};

void	LoadTestSwipeTextures();
void	PopulateTestSwipeView( SwipeView & sv, const int rows );

NV_NAMESPACE_END


