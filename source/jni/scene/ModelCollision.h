#pragma once

#include "vglobal.h"

#include "VMath.h"
#include "VString.h"
#include "VArray.h"

#pragma once

NV_NAMESPACE_BEGIN

class CollisionPolytope
{
public:
	void	Add( const Planef & p ) { Planes.append( p ); }

	// Returns true if the given point is inside this polytope.
	bool	TestPoint( const Vector3f & p ) const;

	// Returns true if the ray hits the polytope.
	// The length of the ray is clipped to the point where the ray enters the polytope.
	// Optionally the polytope boundary plane that is hit is returned.
	bool	TestRay( const Vector3f & start, const Vector3f & dir, float & length, Planef * plane ) const;

	// Pops the given point out of the polytope if inside.
	bool	PopOut( Vector3f & p ) const;

public:
	VString			Name;
	VArray< Planef > Planes;
};

class CollisionModel
{
public:
	// Returns true if the given point is inside solid.
	bool	TestPoint( const Vector3f & p ) const;

	// Returns true if the ray hits solid.
	// The length of the ray is clipped to the point where the ray enters solid.
	// Optionally the solid boundary plane that is hit is returned.
	bool	TestRay( const Vector3f & start, const Vector3f & dir, float & length, Planef * plane ) const;

	// Pops the given point out of any collision geometry the point may be inside of.
	bool	PopOut( Vector3f & p ) const;

public:
	VArray< CollisionPolytope > Polytopes;
};

Vector3f SlideMove(
		const Vector3f & footPos,
		const float eyeHeight,
		const Vector3f & moveDirection,
		const float moveDistance,
		const CollisionModel & collisionModel,
		const CollisionModel & groundCollisionModel
	    );

NV_NAMESPACE_END


