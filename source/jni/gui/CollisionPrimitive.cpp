/************************************************************************************

Filename    :   CollisionPrimitive.cpp
Content     :   Generic collision class supporting ray / triangle intersection.
Created     :   September 10, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#include "CollisionPrimitive.h"

#include "ModelTrace.h"
#include "DebugLines.h"

namespace NervGear {

//==============================
// OvrCollisionPrimitive::IntersectRayBounds
bool OvrCollisionPrimitive::intersectRayBounds( Vector3f const & start, Vector3f const & dir, 
		Vector3f const & scale, ContentFlags_t const testContents, float & t0, float & t1 ) const
{
	if ( !( testContents & contents() ) )
	{
		return false;
	}

	return intersectRayBounds( start, dir, scale, t0, t1 );
}

//==============================
// OvrCollisionPrimitive::IntersectRayBounds
bool OvrCollisionPrimitive::intersectRayBounds( Vector3f const & start, Vector3f const & dir, 
		Vector3f const & scale, float & t0, float & t1 ) const
{
	Bounds3f scaledBounds = m_bounds * scale;
    if (scaledBounds.Contains( start, 0.1f ) )
    {
        return true;
    }

	Intersect_RayBounds( start, dir, scaledBounds.GetMins(), scaledBounds.GetMaxs(), t0, t1 );

	return t0 >= 0.0f && t1 >= 0.0f && t1 >= t0;	
}

//==============================================================================================
// OvrCollisionPrimitive

//==============================
// OvrCollisionPrimitive::OvrCollisionPrimitive
OvrCollisionPrimitive::~OvrCollisionPrimitive()
{
}

//==============================================================================================
// OvrTriCollisionPrimitive

//==============================
// OvrTriCollisionPrimitive::OvrTriCollisionPrimitive
OvrTriCollisionPrimitive::OvrTriCollisionPrimitive() 
{ 
}

//==============================
// OvrTriCollisionPrimitive::OvrTriCollisionPrimitive
OvrTriCollisionPrimitive::OvrTriCollisionPrimitive( Array< Vector3f > const & vertices, 
		Array< TriangleIndex > const & indices, ContentFlags_t const contents ) :
	OvrCollisionPrimitive( contents )
{
	init( vertices, indices, contents );
}

//==============================
// OvrTriCollisionPrimitive::~OvrTriCollisionPrimitive
OvrTriCollisionPrimitive::~OvrTriCollisionPrimitive()
{
}

//==============================
// OvrTriCollisionPrimitive::Init
void OvrTriCollisionPrimitive::init( Array< Vector3f > const & vertices, Array< TriangleIndex > const & indices, 
		ContentFlags_t const contents )
{
	m_vertices = vertices;
	m_indices = indices;
	setContents( contents );

	// calculate the bounds
	Bounds3f b;
	b.Clear();
	for ( int i = 0; i < vertices.sizeInt(); ++i )
	{
		b.AddPoint( vertices[i] );
	}

	setBounds( b );
}

//==============================
// OvrTriCollisionPrimitive::IntersectRay
bool OvrTriCollisionPrimitive::intersectRay( Vector3f const & start, Vector3f const & dir, Posef const & pose,
		Vector3f const & scale, ContentFlags_t const testContents, OvrCollisionResult & result ) const
{
	if ( !( testContents & contents() ) )
	{
		result = OvrCollisionResult();
		return false;
	}

    // transform the ray into local space
    Vector3f localStart = ( start - pose.Position ) * pose.Orientation.Inverted();
    Vector3f localDir = dir * pose.Orientation.Inverted();

    return intersectRay( localStart, localDir, scale, testContents, result );
}

//==============================
// OvrTriCollisionPrimitive::IntersectRay
// the ray should already be in local space
bool OvrTriCollisionPrimitive::intersectRay( Vector3f const & localStart, Vector3f const & localDir,
		Vector3f const & scale, ContentFlags_t const testContents, OvrCollisionResult & result ) const
{
	if ( !( testContents & contents() ) )
	{
		result = OvrCollisionResult();
		return false;
	}

	// test vs bounds
	float t0;
	float t1;
	if ( !intersectRayBounds( localStart, localDir, scale, t0, t1 ) )
	{
		return false;
	}

    result.triIndex = -1;
    for ( int i = 0; i < m_indices.sizeInt(); i += 3 )
    {
        float t_;
        float u_;
        float v_;
        Vector3f verts[3];
        verts[0] = m_vertices[m_indices[i]] * scale;
        verts[1] = m_vertices[m_indices[i + 1]] * scale;
        verts[2] = m_vertices[m_indices[i + 2]] * scale;
        if ( Intersect_RayTriangle( localStart, localDir, verts[0], verts[1], verts[2], t_, u_, v_ ) )
        {
            if ( t_ < result.t )
            {
                result.t = t_;
                result.uv.x = u_;
                result.uv.y = v_;
                result.triIndex = i / 3;
            }
        }
    }
    return result.triIndex >= 0;
}

//==============================
// OvrTriCollisionPrimitive::DebugRender
void OvrTriCollisionPrimitive::debugRender( OvrDebugLines & debugLines, Posef & pose ) const
{
	debugLines.AddBounds( pose, bounds(), Vector4f( 1.0f, 0.5f, 0.0f, 1.0f ) );

	Vector4f color( 1.0f, 0.0f, 1.0f, 1.0f );
	for ( int i = 0; i < m_indices.sizeInt(); i += 3 )
	{
		int i1 = m_indices[i + 0];
		int i2 = m_indices[i + 1];
		int i3 = m_indices[i + 2];
		Vector3f v1 = pose.Position + ( pose.Orientation * m_vertices[i1] );
		Vector3f v2 = pose.Position + ( pose.Orientation * m_vertices[i2] );
		Vector3f v3 = pose.Position + ( pose.Orientation * m_vertices[i3] );
		debugLines.AddLine( v1, v2, color, color, 0, true );
		debugLines.AddLine( v2, v3, color, color, 0, true );
		debugLines.AddLine( v3, v1, color, color, 0, true );
	}
}

} // namespace NervGear
