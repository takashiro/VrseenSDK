/************************************************************************************

Filename    :   ScrollManager.cpp
Content     :
Created     :   December 12, 2014
Authors     :   Jonathan E. Wright, Warsam Osman, Madhu Kalva

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#include "ScrollManager.h"
#include "Alg.h"
#include "api/Vsync.h"
#include "Input.h"
#include "VrCommon.h"

namespace NervGear {

static const float 	EPSILON_VEL = 0.00001f;

static const int HALF_WRAP_JUMP = 4;
static const int HALF_WRAP_JUMP_ALPHA = 2;
static const float MINIMUM_FADE_VALUE = 0.2f;

OvrScrollManager::OvrScrollManager( eScrollType type )
		: m_currentScrollType( type )
		, m_maxPosition( 0.0f )
		, m_touchIsDown( false )
		, m_deltas()
		, m_currentScrollState( SMOOTH_SCROLL )
		, m_position( 0.0f )
		, m_velocity( 0.0f )
		, m_accumulatedDeltaTimeInSeconds( 0.0f )
		, m_wrapAroundEnabled( true )
		, m_isWrapAroundTimeInitiated( false )
		, m_remainingTimeForWrapAround( 0.0f )
		, m_autoScrollInFwdDir( false )
		, m_autoScrollBkwdDir( false )
		, m_prevAutoScrollVelocity( 0.0f )
		, m_restrictedScrolling( false )
		, m_touchDirectionLocked( NO_LOCK )
		, m_controllerDirectionLocked( NO_LOCK )
{
}

void OvrScrollManager::frame( float deltaSeconds, unsigned int controllerState )
{
	// --
	// Controller Input Handling
	bool forwardScrolling = false;
	bool backwardScrolling = false;

	if( m_currentScrollType == HORIZONTAL_SCROLL )
	{
		forwardScrolling = controllerState & ( BUTTON_LSTICK_RIGHT | BUTTON_DPAD_RIGHT );
		backwardScrolling = controllerState & ( BUTTON_LSTICK_LEFT | BUTTON_DPAD_LEFT );
	}
	else if( m_currentScrollType == VERTICAL_SCROLL )
	{
		forwardScrolling = controllerState & ( BUTTON_LSTICK_DOWN | BUTTON_DPAD_DOWN );
		backwardScrolling = controllerState & ( BUTTON_LSTICK_UP | BUTTON_DPAD_UP );
	}

	if ( forwardScrolling || backwardScrolling )
	{
		m_currentScrollState = SMOOTH_SCROLL;
	}
	setEnableAutoForwardScrolling( forwardScrolling );
	setEnableAutoBackwardScrolling( backwardScrolling );

	// --
	// Logic update for the current scroll state
	switch( m_currentScrollState )
	{
		case SMOOTH_SCROLL:
		{
			// Auto scroll during wrap around initiation.
			if( ( !m_restrictedScrolling ) || // Either not restricted scrolling
				( m_restrictedScrolling && ( m_touchDirectionLocked != NO_LOCK ) ) ) // or restricted scrolling with touch direction locked
			{
				if ( m_touchIsDown )
				{
					if ( ( m_position < 0.0f ) || ( m_position > m_maxPosition ) )
					{
						// When scrolled out of bounds and touch is still down,
						// auto-scroll to initiate wrap around easily.
						float totalDistance = 0.0f;
						for ( int i = 0; i < m_deltas.num(); ++i )
						{
							totalDistance += m_deltas[i].d;
						}

						const float EPSILON_VEL = 0.00001f;
						if ( totalDistance < -EPSILON_VEL )
						{
							m_velocity = -1.0f;
						}
						else if ( totalDistance > EPSILON_VEL )
						{
							m_velocity = 1.0f;
						}
						else
						{
							m_velocity = m_prevAutoScrollVelocity;
						}
					}
					else
					{
						m_velocity = 0.0f;
					}

					m_prevAutoScrollVelocity = m_velocity;
				}
			}

			if ( isOutOfWrapBounds() && isScrolling() )
			{
				if( m_isWrapAroundTimeInitiated )
				{
					m_remainingTimeForWrapAround -= deltaSeconds;
				}
				else
				{
					m_isWrapAroundTimeInitiated = true;
					m_remainingTimeForWrapAround = m_scrollBehavior.wrapAroundHoldTime;
				}
			}
			else
			{
				m_isWrapAroundTimeInitiated 	= false;
				m_remainingTimeForWrapAround 	= 0.0f;
				if( isOutOfBounds() )
				{
					if( !isScrolling() )
					{
						m_currentScrollState = BOUNCE_SCROLL;
						const float bounceBackVelocity = 3.0f;
						if( m_position < 0.0f )
						{
							m_velocity = bounceBackVelocity;
						}
						else if( m_position > m_maxPosition )
						{
							m_velocity = -bounceBackVelocity;
						}
						m_velocity = getModifiedVelocity( m_velocity );
					}
				}
			}
		}
		break;

		case WRAP_AROUND_SCROLL:
		{
			if( ( m_velocity > 0.0f && m_position > m_maxPosition ) ||
				( m_velocity < 0.0f && m_position < 0.0f ) ) // Wrapping around is over
			{
				m_velocity = 0.0f;
				m_currentScrollState = SMOOTH_SCROLL;
				m_position = Alg::Clamp( m_position, 0.0f, m_maxPosition );
			}

			else
			{
				if ( needsJumpDuringWrapAround() )
				{
					const int startCutOff 		= HALF_WRAP_JUMP - 1;
					const int endCutOff 		= m_maxPosition - HALF_WRAP_JUMP;
					int direction = m_velocity / fabsf( m_velocity );

					if ( ( m_position >= startCutOff ) &&  ( m_position <= endCutOff ) )
					{
						if ( false ) // snap jump
						{
							if ( m_velocity > 0.0f )
							{
								m_position = endCutOff;
							}
							else if ( m_velocity < 0.0f )
							{
								m_position = startCutOff;
							}
						}
						else
						{
							float requiredJump = endCutOff - startCutOff;
							int SpeedUpRange = requiredJump / 2;
							SpeedUpRange = ( SpeedUpRange > 5 ) ? 5 : SpeedUpRange;
							float speedScale = requiredJump * 0.5f;
							speedScale = (speedScale > 4.0f) ? speedScale : 4.0f;
							if ( m_position < startCutOff + SpeedUpRange )
							{
								speedScale = LinearRangeMapFloat( m_position, startCutOff, startCutOff + SpeedUpRange, 1.0f, speedScale );
							}
							else if ( m_position > endCutOff - SpeedUpRange )
							{
								speedScale = LinearRangeMapFloat( m_position, endCutOff - SpeedUpRange, endCutOff, speedScale, 1.0f );
							}

							m_velocity = m_scrollBehavior.wrapAroundSpeed * direction * speedScale;
						}
					}
					else
					{

						m_velocity = m_scrollBehavior.wrapAroundSpeed * direction;
					}
				}
			}
		}
		break;

		case BOUNCE_SCROLL:
		{
			if( !isOutOfBounds() )
			{
				m_currentScrollState = SMOOTH_SCROLL;
				if( m_velocity > 0.0f ) // Pulled at the beginning
				{
					m_position = 0.0f;
				}
				else if( m_velocity < 0.0f ) // Pulled at the end
				{
					m_position = m_maxPosition;
				}
				m_velocity = 0.0f;
			}
		}
		break;
	}

	// --
	// Scrolling physics update
	m_accumulatedDeltaTimeInSeconds += deltaSeconds;
	OVR_ASSERT( m_accumulatedDeltaTimeInSeconds <= 10.0f );
	while( true )
	{
		if( m_accumulatedDeltaTimeInSeconds < 0.016f )
		{
			break;
		}

		m_accumulatedDeltaTimeInSeconds -= 0.016f;
		if( m_currentScrollState == SMOOTH_SCROLL || m_currentScrollState == BOUNCE_SCROLL ) // While wrap around don't slow down scrolling
		{
			m_velocity -= m_velocity * m_scrollBehavior.frictionPerSec * 0.016f;
			if( fabsf(m_velocity) < EPSILON_VEL )
			{
				m_velocity = 0.0f;
			}
		}

		m_position += m_velocity * 0.016f;
		if( m_currentScrollState == BOUNCE_SCROLL )
		{
			if( m_velocity > 0.0f )
			{
				m_position = Alg::Clamp( m_position, -m_scrollBehavior.padding, m_maxPosition );
			}
			else
			{
				m_position = Alg::Clamp( m_position, 0.0f, m_maxPosition + m_scrollBehavior.padding );
			}
		}
		else
		{
			m_position = Alg::Clamp( m_position, -m_scrollBehavior.padding, m_maxPosition + m_scrollBehavior.padding );
		}
	}
}

void OvrScrollManager::touchDown()
{
	m_touchIsDown = true;
	m_currentScrollState = SMOOTH_SCROLL;
	m_lastTouchPosistion.Set( 0.0f, 0.0f, 0.0f );
	m_velocity = 0.0f;
	m_deltas.clear();

}

void OvrScrollManager::touchUp()
{
	m_touchIsDown = false;

	if ( m_deltas.num() == 0 )
	{
		m_velocity = 0.0f;
	}
	else
	{
		if ( m_wrapAroundEnabled && m_isWrapAroundTimeInitiated )
		{
			performWrapAroundOnScrollFinish();
		}
		else
		{
			// accumulate total distance
			float totalDistance = 0.0f;
			for ( int i = 0; i < m_deltas.num(); ++i )
			{
				totalDistance += m_deltas[i].d;
			}

			// calculating total time spent
			delta_t const & head = m_deltas.head();
			delta_t const & tail = m_deltas.tail();
			float totalTime = head.t - tail.t;

			// calculating velocity based on touches
            float touchVelocity = totalTime > VConstantsf::SmallestNonDenormal ? totalDistance / totalTime : 0.0f;
			m_velocity = getModifiedVelocity( touchVelocity );
		}
	}

	m_deltas.clear();

	return;
}

void OvrScrollManager::touchRelative( Vector3f touchPos )
{
	if ( !m_touchIsDown )
	{
		return;
	}

	// Check if the touch is valid for this( horizontal / vertical ) scrolling type
	bool isValid = false;
	if( fabsf( m_lastTouchPosistion.x - touchPos.x ) > fabsf( m_lastTouchPosistion.y - touchPos.y ) )
	{
		if( m_currentScrollType == HORIZONTAL_SCROLL )
		{
			isValid = true;
		}
	}
	else
	{
		if( m_currentScrollType == VERTICAL_SCROLL )
		{
			isValid = true;
		}
	}

	if( isValid )
	{
		float touchVal;
		float lastTouchVal;
		float curMove;
		if( m_currentScrollType == HORIZONTAL_SCROLL )
		{
			touchVal = touchPos.x;
			lastTouchVal = m_lastTouchPosistion.x;
			curMove = lastTouchVal - touchVal;
		}
		else
		{
			touchVal = touchPos.y;
			lastTouchVal = m_lastTouchPosistion.y;
			curMove = touchVal - lastTouchVal;
		}

		float const DISTANCE_SCALE = 0.0025f;

		m_position += curMove * DISTANCE_SCALE;

		float const DISTANCE_RAMP = 150.0f;
		float const ramp = fabsf( touchVal ) / DISTANCE_RAMP;
		m_deltas.Append( delta_t( curMove * DISTANCE_SCALE * ramp, ovr_GetTimeInSeconds() ) );
	}

	m_lastTouchPosistion = touchPos;
}

bool OvrScrollManager::isOutOfBounds() const
{
	return ( m_position < 0.0f || m_position > m_maxPosition );
}

bool OvrScrollManager::isOutOfWrapBounds() const
{
	return ( m_position < -m_scrollBehavior.wrapAroundScrollOffset || m_position > m_maxPosition + m_scrollBehavior.wrapAroundScrollOffset );
}

void OvrScrollManager::setEnableAutoForwardScrolling( bool value )
{
	if( value )
	{
		float newVelocity = m_scrollBehavior.autoVerticalSpeed;
		m_velocity = m_velocity + ( newVelocity - m_velocity ) * 0.3f;
	}
	else
	{
		if( m_autoScrollInFwdDir )
		{
			// Turning off auto forward scrolling, adjust velocity so it stops at proper position
			m_velocity = getModifiedVelocity( m_velocity );

			if ( m_wrapAroundEnabled )
			{
				performWrapAroundOnScrollFinish();
			}
		}
	}

	m_autoScrollInFwdDir = value;
}

void OvrScrollManager::setEnableAutoBackwardScrolling( bool value )
{
	if( value )
	{
		float newVelocity = -m_scrollBehavior.autoVerticalSpeed;
		m_velocity = m_velocity + ( newVelocity - m_velocity ) * 0.3f;
	}
	else
	{
		if( m_autoScrollBkwdDir )
		{
			// Turning off auto forward scrolling, adjust velocity so it stops at proper position
			m_velocity = getModifiedVelocity( m_velocity );

			if ( m_wrapAroundEnabled )
			{
				performWrapAroundOnScrollFinish();
			}
		}
	}

	m_autoScrollBkwdDir = value;
}

float OvrScrollManager::getModifiedVelocity( const float inVelocity )
{
	// calculating distance its going to travel with touch velocity.
	float distanceItsGonnaTravel = 0.0f;
	float begginingVelocity = inVelocity;
	while( true )
	{
		begginingVelocity -= begginingVelocity * m_scrollBehavior.frictionPerSec * 0.016f;
		distanceItsGonnaTravel += begginingVelocity * 0.016f;

		if( fabsf( begginingVelocity ) < EPSILON_VEL )
		{
			break;
		}
	}

	float currentTargetPosition = m_position + distanceItsGonnaTravel;
	float desiredTargetPosition = nearbyint( currentTargetPosition );

	// Calculating velocity to compute desired target position
	float desiredVelocity = EPSILON_VEL;
	if( m_position >= desiredTargetPosition )
	{
		desiredVelocity = -EPSILON_VEL;
	}
	float currentPosition = m_position;
	float prevPosition;

	while( true )
	{
		prevPosition = currentPosition;
		currentPosition += desiredVelocity * 0.016f;
		desiredVelocity += desiredVelocity * m_scrollBehavior.frictionPerSec * 0.016f;

		if( ( prevPosition <= desiredTargetPosition && currentPosition >= desiredTargetPosition ) ||
			( currentPosition <= desiredTargetPosition && prevPosition >= desiredTargetPosition ) )
		{
			// Found the spot where it should end up.
			// Adjust the position so it can end up at desiredPosition with desiredVelocity
			m_position += ( desiredTargetPosition - prevPosition );
			break;
		}
	}

	return desiredVelocity;
}

void OvrScrollManager::performWrapAroundOnScrollFinish()
{
	if( m_isWrapAroundTimeInitiated &&
		( m_maxPosition > 0.0f ) ) // No wrap around for just 1 item/position
	{
		if( m_remainingTimeForWrapAround < 0.0f )
		{
			int direction = 0;
			if( m_position <= -m_scrollBehavior.padding * 0.5f )
			{
				direction = 1;

			}
			else if( m_position >= m_maxPosition + m_scrollBehavior.padding * 0.5f )
			{
				direction = -1;
			}

			if( direction != 0 )
			{
				// int itemCount = (int)( MaxPosition + 1.0f );
				m_velocity = m_scrollBehavior.wrapAroundSpeed * direction;
				m_currentScrollState = WRAP_AROUND_SCROLL;
				m_isWrapAroundTimeInitiated = false;
				m_remainingTimeForWrapAround = 0.0f;
			}
		}
		else
		{
			m_currentScrollState = BOUNCE_SCROLL;
			const float bounceBackVelocity = 3.0f;
			if( m_position < 0.0f )
			{
				m_velocity = bounceBackVelocity;
			}
			else if( m_position > m_maxPosition )
			{
				m_velocity = -bounceBackVelocity;
			}
			m_velocity = getModifiedVelocity( m_velocity );
		}
	}
}

void OvrScrollManager::setRestrictedScrollingData( bool isRestricted, eScrollDirectionLockType touchLock, eScrollDirectionLockType controllerLock )
{
	m_restrictedScrolling = isRestricted;
	m_touchDirectionLocked = touchLock;
	m_controllerDirectionLocked = controllerLock;
}

bool IsInRage( float val, float startRange, float endRange )
{
	return ( ( val >= startRange ) && ( val <= endRange ) );
}

float OvrScrollManager::wrapAroundAlphaChange()
{
	float finalAlpha = 1.0f;

	if ( ( m_currentScrollState == WRAP_AROUND_SCROLL ) &&
		 ( needsJumpDuringWrapAround() ) )
	{
		float fadeStart = HALF_WRAP_JUMP_ALPHA;
		float fadeEnd 	= m_maxPosition - HALF_WRAP_JUMP_ALPHA;

		if ( IsInRage( m_position, fadeStart, fadeEnd ) )
		{
			float fadeOutStart 		= fadeStart;
			float fadeOutEnd 		= HALF_WRAP_JUMP - 1.0f;
			float fadeBackInStart 	= m_maxPosition - ( HALF_WRAP_JUMP - 1.0f );
			float fadeBackInEnd 	= fadeEnd;

			if ( IsInRage( m_position, fadeOutStart, fadeOutEnd ) )
			{
				finalAlpha = LinearRangeMapFloat( m_position, fadeOutStart, fadeOutEnd, 1.0f, MINIMUM_FADE_VALUE );
			}
			else if ( IsInRage( m_position, fadeBackInStart, fadeBackInEnd ) )
			{
				finalAlpha = LinearRangeMapFloat( m_position, fadeBackInStart, fadeBackInEnd, MINIMUM_FADE_VALUE, 1.0f );
			}
			else
			{
				finalAlpha = MINIMUM_FADE_VALUE;
			}
		}
	}

	return finalAlpha;
}

bool OvrScrollManager::needsJumpDuringWrapAround()
{
	return m_maxPosition > ( 2 * HALF_WRAP_JUMP );
}

}
