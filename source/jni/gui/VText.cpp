/*
 * VText.cpp
 *
 *  Created on: 2016年3月30日
 *      Author: yangkai
 */
#include "VText.h"
NV_NAMESPACE_BEGIN

VPointTracker::VPointTracker( float const rate) :
    LastFrameTime( 0.0 ),
    Rate( 0.1f ),
    CurPosition( 0.0f ),
    FirstFrame( true )
{
}

void    VPointTracker::Update( double const curFrameTime, V3Vectf const & newPos )
{
    double frameDelta = curFrameTime - LastFrameTime;
    LastFrameTime = curFrameTime;
    float const rateScale = static_cast< float >( frameDelta / ( 1.0 / static_cast< double >( DEFAULT_FRAME_RATE ) ) );
    float const rate = Rate * rateScale;
    if ( FirstFrame )
    {
        CurPosition = newPos;
    }
    else
    {
        V3Vectf delta = ( newPos - CurPosition ) * rate;
        if ( delta.Length() < 0.001f )
        {
            // don't allow a denormal to propagate from multiplications of very small numbers
            delta = V3Vectf( 0.0f );
        }
        CurPosition += delta;
    }
    FirstFrame = false;
}

void    VPointTracker::Reset() { FirstFrame = true; }

void    VPointTracker::SetRate( float const r ) { Rate = r; }

V3Vectf const & VPointTracker::GetCurPosition() const { return CurPosition; }

void VText::show(const VString &text, float duration)
{
    infoText = text;
    infoTextColor = V4Vectf(1.0f);
    infoTextOffset = V3Vectf(0.0f, 0.0f, 1.5f);
    infoTextPointTracker.Reset();
    infoTextEndFrame = vrFrame.FrameNumber + (long long)(duration * 60.0f) + 1;
}

void VText::show( float const duration, V3Vectf const & offset, V4Vectf const & color, const char * fmt, ... )
{
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    infoText = buffer;
    infoTextColor = color;
    if (offset != infoTextOffset || infoTextEndFrame < vrFrame.FrameNumber)
    {
        infoTextPointTracker.Reset();
    }
    infoTextOffset = offset;
    infoTextEndFrame = vrFrame.FrameNumber + (long long)(duration * 60.0f) + 1;
}
VText::VText():
        infoTextColor(1.0f),
        infoTextOffset(0.0f),
        infoTextEndFrame(-1)
{

}

NV_NAMESPACE_END



