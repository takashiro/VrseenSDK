/*
 * VText.h
 *
 *  Created on: 2016年3月30日
 *      Author: yangkai
 */

#pragma once
#include "VString.h"
#include "VVector.h"
#include "Input.h"
NV_NAMESPACE_BEGIN
class VPointTracker
{
public:
    static const int DEFAULT_FRAME_RATE = 60;

    VPointTracker(float const rate = 0.1f );

    void        Update( double const curFrameTime, V3Vectf const & newPos );

    void        Reset();

    void        SetRate( float const r );

    V3Vectf const & GetCurPosition() const;

private:
    double      LastFrameTime;
    float       Rate;
    V3Vectf CurPosition;
    bool        FirstFrame;
};

class VText
{
public:
    VText();
    VString         infoText;           // informative text to show in front of the view
    V4Vectf     infoTextColor;      // color of info text
    V3Vectf     infoTextOffset;     // offset from center of screen in view space
    long long       infoTextEndFrame;   // time to stop showing text
    VPointTracker   infoTextPointTracker;   // smoothly tracks to text ideal location
    VPointTracker   fpsPointTracker;
    VrFrame         vrFrame;

    void show(const VString &text, float duration);
    void show( float const duration, V3Vectf const & offset, V4Vectf const & color, const char * fmt, ... );

};
NV_NAMESPACE_END
