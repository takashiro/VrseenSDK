#pragma once

#include "vglobal.h"

#include "VBasicmath.h"
#include "VString.h"
#include "VArray.h"

#pragma once

NV_NAMESPACE_BEGIN

class CollisionPolytope
{
public:
    void	Add( const VPlanef & p ) { Planes.append( p ); }

	// Returns true if the given point is inside this polytope.
    bool	TestPoint( const V3Vectf & p ) const;

	// Returns true if the ray hits the polytope.
	// The length of the ray is clipped to the point where the ray enters the polytope.
	// Optionally the polytope boundary plane that is hit is returned.
    bool	TestRay( const V3Vectf & start, const V3Vectf & dir, float & length, VPlanef * plane ) const;

	// Pops the given point out of the polytope if inside.
    bool	PopOut( V3Vectf & p ) const;

public:
	VString			Name;
    VArray< VPlanef > Planes;
};

class CollisionModel
{
public:
	// Returns true if the given point is inside solid.
    bool	TestPoint( const V3Vectf & p ) const;

	// Returns true if the ray hits solid.
	// The length of the ray is clipped to the point where the ray enters solid.
	// Optionally the solid boundary plane that is hit is returned.
    bool	TestRay( const V3Vectf & start, const V3Vectf & dir, float & length, VPlanef * plane ) const;

	// Pops the given point out of any collision geometry the point may be inside of.
    bool	PopOut( V3Vectf & p ) const;

public:
	VArray< CollisionPolytope > Polytopes;
};

V3Vectf SlideMove(
        const V3Vectf & footPos,
		const float eyeHeight,
        const V3Vectf & moveDirection,
		const float moveDistance,
		const CollisionModel & collisionModel,
		const CollisionModel & groundCollisionModel
	    );

NV_NAMESPACE_END


