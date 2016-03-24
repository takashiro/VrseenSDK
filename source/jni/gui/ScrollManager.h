/************************************************************************************

Filename    :   ScrollManager.h
Content     :
Created     :   December 12, 2014
Authors     :   Jonathan E. Wright, Warsam Osman, Madhu Kalva

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#if !defined( OVR_ScrollManager_h )
#define OVR_ScrollManager_h

#include "VBasicmath.h"

NV_NAMESPACE_BEGIN

template< typename _type_, int _size_ >
class CircularArrayT
{
public:
	static const int MAX = _size_;

	CircularArrayT() :
        m_headIndex( 0 ),
        m_num( 0 )
	{
	}

    int				getIndex( const int offset ) const
	{
        int idx = m_headIndex + offset;
		if ( idx >= _size_ )
		{
			idx -= _size_;
		}
		else if ( idx < 0 )
		{
			idx += _size_;
		}
		return idx;
	}

	_type_ &		operator[] ( int const idx )
	{
        return m_buffer[getIndex( -idx )];
	}
	_type_ const &	operator[] ( int const idx ) const
	{
        return m_buffer[getIndex( -idx )];
	}

    bool			isEmpty() const { return m_num == 0; }
    int				headIndex() const { return getIndex( 0 ); }
    int				tailIndex() const { return isEmpty() ? getIndex( 0 ) : getIndex( - ( m_num - 1 ) ); }

    _type_ const &	head() const { return operator[]( 0 ); }
    _type_ const &	tail() const { return operator[]( m_num - 1 ); }

    int				num() const { return m_num; }

    void			clear()
	{
        m_headIndex = 0;
        m_num = 0;
	}

	void			Append( _type_ const & obj )
	{
        if ( m_num > 0 )
		{
            m_headIndex = getIndex( 1 );
		}
        m_buffer[m_headIndex] = obj;
        if ( m_num < _size_ )
		{
            m_num++;
		}
	}

private:
    _type_	m_buffer[_size_];
    int		m_headIndex;
    int		m_num;
};

enum eScrollType
{
	HORIZONTAL_SCROLL,
	VERTICAL_SCROLL
};

enum eScrollDirectionLockType
{
	NO_LOCK = 0,
	HORIZONTAL_LOCK,
	VERTICAL_LOCK
};

enum eScrollState
{
	SMOOTH_SCROLL,			// Regular smooth scroll
	WRAP_AROUND_SCROLL,		// Wrap around scroll : begin to end or end to begin
	BOUNCE_SCROLL			// Bounce back scroll, when user pulls
};

struct OvrScrollBehavior
{
    float 	frictionPerSec;
    float 	padding; 				// Padding at the beginning and end of the scroll bounds
    float 	wrapAroundScrollOffset; // Tells how scrolling should be pulled beyond its bounds to consider for wrap around
    float 	wrapAroundHoldTime; 	// Tells how long you need to hold at WrapAroundScrollOffset, in order to consider as wrap around event
    float 	wrapAroundSpeed;
    float 	autoVerticalSpeed;

	OvrScrollBehavior()
    : frictionPerSec( 5.0f )
    , padding( 1.0f )
    , wrapAroundHoldTime( 0.4f )
    , wrapAroundSpeed( 7.0f )
    , autoVerticalSpeed( 5.5f )
	{
        wrapAroundScrollOffset = padding * 0.75f; // 75% of Padding
	}

    void setPadding( float value )
	{
        padding = value;
        wrapAroundScrollOffset = padding * 0.75f;
	}
};

class OvrScrollManager
{
public:
			OvrScrollManager( eScrollType type );

    void 	frame( float deltaSeconds, unsigned int controllerState );

    void 	touchDown();
    void 	touchUp();
    void 	touchRelative( V3Vectf touchPos );

    bool 	isOutOfBounds() 		const;
    bool 	isOutOfWrapBounds()		const;
    bool 	isScrolling() 			const 			{ return m_touchIsDown || m_autoScrollInFwdDir || m_autoScrollBkwdDir; }
    bool	isTouchIsDown()			const			{ return m_touchIsDown; }
    bool	isWrapAroundEnabled() 	const			{ return m_wrapAroundEnabled; }

    void 	setMaxPosition( float position ) 		{ m_maxPosition = position; }
    void 	setPosition( float position ) 			{ m_position = position; }
    void 	setVelocity( float velocity ) 			{ m_velocity = velocity; }
    void	setTouchIsDown( bool value )			{ m_touchIsDown = value; }
    void	setScrollPadding( float value )			{ m_scrollBehavior.setPadding( value ); }
    void	setWrapAroundEnable( bool value )		{ m_wrapAroundEnabled = value; }
    void	setRestrictedScrollingData( bool isRestricted, eScrollDirectionLockType touchLock, eScrollDirectionLockType controllerLock );

    float 	position() 					const 	{ return m_position; }
    float 	maxPosition()				const 	{ return m_maxPosition; }
    float	velocity()					const	{ return m_velocity; }
    bool 	isWrapAroundTimeInitiated() 	const 	{ return m_isWrapAroundTimeInitiated; }
    float 	remainingTimeForWrapAround() const 	{ return m_remainingTimeForWrapAround; }
    float 	wrapAroundScrollOffset()		const	{ return m_scrollBehavior.wrapAroundScrollOffset; }
    float 	wrapAroundHoldTime()			const	{ return m_scrollBehavior.wrapAroundHoldTime; }
    float  	wrapAroundAlphaChange();

    void 	setEnableAutoForwardScrolling( bool value );
    void 	setEnableAutoBackwardScrolling( bool value );

private:
	struct delta_t
	{
		delta_t() : d( 0.0f ), t( 0.0f ) { }
		delta_t( float const d_, float const t_ ) : d( d_ ), t( t_ ) { }

		float	d;	// distance moved
		float	t;	// time stamp
	};

    eScrollType 							m_currentScrollType; // Is it horizontal scrolling or vertical scrolling
    OvrScrollBehavior						m_scrollBehavior;
    float 									m_maxPosition;

    bool 									m_touchIsDown;
    V3Vectf 								m_lastTouchPosistion;
	// Keeps track of last few(=5) touch movement information
    CircularArrayT< delta_t, 5 > 			m_deltas;
	// Current State - smooth scrolling, wrap around and bounce back
    eScrollState 							m_currentScrollState;

    float 									m_position;
    float 									m_velocity;
	// Delta time is accumulated and scrolling physics will be done in iterations of 0.016f seconds, to stop scrolling at right spot
    float 									m_accumulatedDeltaTimeInSeconds;

    bool									m_wrapAroundEnabled;
    bool 									m_isWrapAroundTimeInitiated;
    float 									m_remainingTimeForWrapAround;

	// Controller Input
    bool									m_autoScrollInFwdDir;
    bool									m_autoScrollBkwdDir;
    bool									m_isHorizontalScrolling;

    float 									m_prevAutoScrollVelocity;

    bool									m_restrictedScrolling;
    eScrollDirectionLockType				m_touchDirectionLocked;
    eScrollDirectionLockType				m_controllerDirectionLocked;

	// Returns the velocity that stops scrolling in proper place, over an item rather than in between items.
    float 									getModifiedVelocity( const float inVelocity );
    void									performWrapAroundOnScrollFinish();
    bool									needsJumpDuringWrapAround();
};

NV_NAMESPACE_END

#endif // OVR_ScrollManager_h
