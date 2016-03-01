#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

//==============================================================
// Fader
// Fades a value between 0 and 1
class Fader {
public:
    enum eFadeState
    {
        FADE_NONE,      // only the state when the alpha is either 1 or 0
        FADE_PAUSED,    // value may be in the middle
        FADE_IN,
        FADE_OUT,
        FADE_MAX
    };

    Fader( float const startAlpha );

    void    update( float const fadeRate, double const deltaSeconds );

    float       fadeAlpha() const { return m_fadeAlpha; }
    eFadeState  fadeState() const { return m_fadeState; }

    void startFadeIn();
    void startFadeOut();
    void pauseFade();
    void unpause();
    void reset();
    void forceFinish();
    void setFadeAlpha( float const fa ) { m_fadeAlpha = fa; }

    char const * getFadeStateName( eFadeState const state ) const;

private:
    eFadeState  m_fadeState;
    eFadeState  m_prePauseState;
    const float m_startAlpha;
    float       m_fadeAlpha;
};

//==============================================================
// SineFader
// Maps fade alpha to a sine curve
class SineFader : public Fader
{
public:
    SineFader( float const startAlpha );

    float   finalAlpha() const;
};

NV_NAMESPACE_END

