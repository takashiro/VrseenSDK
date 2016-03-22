/************************************************************************************

Filename    :   Fader.cpp
Content     :   Utility classes for animation based on alpha values
Created     :   July 25, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#include "Fader.h"

#include "VMath.h"
#include "VBasicmath.h"
#include "Android/LogUtils.h"

namespace NervGear {

//======================================================================================
// Fader
//======================================================================================

//==============================
// Fader::Fader
Fader::Fader( float const startAlpha ) :
    m_fadeState( FADE_NONE ),
    m_prePauseState( FADE_NONE ),
	m_startAlpha( startAlpha ),
	m_fadeAlpha( startAlpha )
{
}

//==============================
// Fader::Update
void Fader::update( float const fadeRate, double const deltaSeconds )
{
    if ( m_fadeState > FADE_PAUSED && deltaSeconds > 0.0f )
    {
        float const fadeDelta = ( fadeRate * deltaSeconds ) * ( m_fadeState == FADE_IN ? 1.0f : -1.0f );
        m_fadeAlpha += fadeDelta;
        OVR_ASSERT( fabs( fadeDelta ) > VConstantsf::SmallestNonDenormal );
        if ( fabs( fadeDelta ) < VConstantsf::SmallestNonDenormal )
		{
            LOG( "Fader::Update fabs( fadeDelta ) < VConstantsf::SmallestNonDenormal !!!!" );
		}
        if ( m_fadeAlpha < VConstantsf::SmallestNonDenormal )
        {
            m_fadeAlpha = 0.0f;
            m_fadeState = FADE_NONE;
            //LOG( "FadeState = FADE_NONE" );
        }
        else if ( m_fadeAlpha >= 1.0f - VConstantsf::SmallestNonDenormal )
        {
            m_fadeAlpha = 1.0f;
            m_fadeState = FADE_NONE;
            //LOG( "FadeState = FADE_NONE" );
        }
        //LOG( "fadeState = %s, fadeDelta = %.4f, fadeAlpha = %.4f", GetFadeStateName( FadeState ), fadeDelta, FadeAlpha );
    }
}

//==============================
// Fader::StartFadeIn
void Fader::startFadeIn()
{
    //LOG( "StartFadeIn" );
    m_fadeState = FADE_IN;
}

//==============================
// Fader::StartFadeOut
void Fader::startFadeOut()
{
    //LOG( "StartFadeOut" );
    m_fadeState = FADE_OUT;
}

//==============================
// Fader::PauseFade
void Fader::pauseFade()
{
    //LOG( "PauseFade" );
    m_prePauseState = m_fadeState;
    m_fadeState = FADE_PAUSED;
}

//==============================
// Fader::UnPause
void Fader::unpause()
{
    m_fadeState = m_prePauseState;
}

//==============================
// Fader::GetFadeStateName
char const * Fader::getFadeStateName( eFadeState const state ) const
{
    char const * fadeStateNames[FADE_MAX] = { "FADE_NONE", "FADE_PAUSED", "FADE_IN", "FADE_OUT" };
    return fadeStateNames[state];
}

//==============================
// Fader::Reset
void Fader::reset()
{
	m_fadeAlpha = m_startAlpha;
}

//==============================
// Fader::Reset
void Fader::forceFinish()
{
	m_fadeState = FADE_NONE;
	m_fadeAlpha = m_fadeState == FADE_IN ? 1.0f : 0.0f;
}

//======================================================================================
// SineFader
//======================================================================================

//==============================
// SineFader::SineFader
SineFader::SineFader( float const startAlpha ) :
    Fader( startAlpha )
{
}

//==============================
// SineFader::GetFinalAlpha
float SineFader::finalAlpha() const
{
    // NOTE: pausing will still re-calculate the
    if ( fadeState() == FADE_NONE )
    {
        return fadeAlpha();   // already clamped
    }
    // map to sine wave
    float radians = ( 1.0f - fadeAlpha() ) * VConstantsf::Pi;  // range 0 to pi
    return ( cos( radians ) + 1.0f ) * 0.5f; // range 0 to 1
}

} // namespace NervGear
