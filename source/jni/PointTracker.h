#pragma once

#include "vglobal.h"

NV_NAMESPACE_BEGIN

class OvrPointTracker
{
public:
	static const int DEFAULT_FRAME_RATE = 60;

	OvrPointTracker( float const rate = 0.1f ) :
		LastFrameTime( 0.0 ),
		Rate( 0.1f ),
		CurPosition( 0.0f ),
		FirstFrame( true )
	{
	}

    void		Update( double const curFrameTime, V3Vectf const & newPos )
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

	void				Reset() { FirstFrame = true; }
	void				SetRate( float const r ) { Rate = r; }

    V3Vectf const &	GetCurPosition() const { return CurPosition; }

private:
	double		LastFrameTime;
	float		Rate;
    V3Vectf	CurPosition;
	bool		FirstFrame;
};


NV_NAMESPACE_END

