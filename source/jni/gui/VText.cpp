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
    Rate(rate),
    CurPosition( 0.0f ),
    FirstFrame( true )
{
}

void    VPointTracker::Update( double const curFrameTime, VVect3f const & newPos )
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
        VVect3f delta = ( newPos - CurPosition ) * rate;
        if ( delta.length() < 0.001f )
        {
            // don't allow a denormal to propagate from multiplications of very small numbers
            delta = VVect3f( 0.0f );
        }
        CurPosition += delta;
    }
    FirstFrame = false;
}

void    VPointTracker::Reset() { FirstFrame = true; }

void    VPointTracker::SetRate( float const r ) { Rate = r; }

VVect3f const & VPointTracker::GetCurPosition() const { return CurPosition; }

void VText::show(const VString &text, float duration)
{
    infoText = text;
    infoTextColor = VVect4f(1.0f);
    infoTextOffset = VVect3f(0.0f, 0.0f, 1.5f);
    infoTextPointTracker.Reset();
    infoTextEndFrame = vrFrame.id + (long long)(duration * 60.0f) + 1;
}

void VText::show( float const duration, VVect3f const & offset, VVect4f const & color, const char * fmt, ... )
{
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    infoText = buffer;
    infoTextColor = color;
    if (offset != infoTextOffset || infoTextEndFrame < vrFrame.id)
    {
        infoTextPointTracker.Reset();
    }
    infoTextOffset = offset;
    infoTextEndFrame = vrFrame.id + (long long)(duration * 60.0f) + 1;
}
VText::VText():
        infoTextColor(1.0f),
        infoTextOffset(0.0f),
        infoTextEndFrame(-1)
{

}

NV_NAMESPACE_END



