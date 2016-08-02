#pragma once
#include "VString.h"
#include "VVect3.h"
#include "VVect4.h"
#include "VFrame.h"
NV_NAMESPACE_BEGIN
class VPointTracker
{
public:
    static const int DEFAULT_FRAME_RATE = 60;

    VPointTracker(float const rate = 0.1f);

    void Update(double const curFrameTime, VVect3f const &newPos);

    void Reset();

    void SetRate(float const r);

    VVect3f const &GetCurPosition() const;

private:
    double LastFrameTime;
    float Rate;
    VVect3f CurPosition;
    bool FirstFrame;
};

class VText
{
public:
    VText();
    VString infoText;           // informative text to show in front of the view
    VVect4f infoTextColor;      // color of info text
    VVect3f infoTextOffset;     // offset from center of screen in view space
    long long infoTextEndFrame;   // time to stop showing text
    VPointTracker infoTextPointTracker;   // smoothly tracks to text ideal location

    VFrame vrFrame;

    void show(const VString &text, float duration);
    void show(float const duration, VVect3f const & offset, VVect4f const &color, const char *fmt, ...);

};
NV_NAMESPACE_END
