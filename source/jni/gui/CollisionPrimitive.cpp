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

NV_NAMESPACE_BEGIN

//==============================
// OvrCollisionPrimitive::IntersectRayBounds
bool OvrCollisionPrimitive::intersectRayBounds( V3Vectf const & start, V3Vectf const & dir,
        V3Vectf const & scale, ContentFlags_t const testContents, float & t0, float & t1 ) const
{
	if ( !( testContents & contents() ) )
	{
		return false;
	}

	return intersectRayBounds( start, dir, scale, t0, t1 );
}

//==============================
// OvrCollisionPrimitive::IntersectRayBounds
bool OvrCollisionPrimitive::intersectRayBounds( V3Vectf const & start, V3Vectf const & dir,
        V3Vectf const & scale, float & t0, float & t1 ) const
{
    VBoxf scaledBounds = m_bounds * scale;
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
OvrTriCollisionPrimitive::OvrTriCollisionPrimitive( VArray< V3Vectf > const & vertices,
        VArray< ushort > const & indices, ContentFlags_t const contents ) :
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
void OvrTriCollisionPrimitive::init( VArray< V3Vectf > const & vertices, VArray< ushort > const & indices,
		ContentFlags_t const contents )
{
	m_vertices = vertices;
	m_indices = indices;
	setContents( contents );

	// calculate the bounds
    VBoxf b;
	b.Clear();
	for ( int i = 0; i < vertices.length(); ++i )
	{
		b.AddPoint( vertices[i] );
	}

	setBounds( b );
}

//==============================
// OvrTriCollisionPrimitive::IntersectRay
bool OvrTriCollisionPrimitive::intersectRay( V3Vectf const & start, V3Vectf const & dir, VPosf const & pose,
        V3Vectf const & scale, ContentFlags_t const testContents, OvrCollisionResult & result ) const
{
	if ( !( testContents & contents() ) )
	{
		result = OvrCollisionResult();
		return false;
	}

    // transform the ray into local space
    V3Vectf localStart = ( start - pose.Position ) * pose.Orientation.Inverted();
    V3Vectf localDir = dir * pose.Orientation.Inverted();

    return intersectRay( localStart, localDir, scale, testContents, result );
}

//==============================
// OvrTriCollisionPrimitive::IntersectRay
// the ray should already be in local space
bool OvrTriCollisionPrimitive::intersectRay( V3Vectf const & localStart, V3Vectf const & localDir,
        V3Vectf const & scale, ContentFlags_t const testContents, OvrCollisionResult & result ) const
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
    for ( int i = 0; i < m_indices.length(); i += 3 )
    {
        float t_;
        float u_;
        float v_;
        V3Vectf verts[3];
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
void OvrTriCollisionPrimitive::debugRender( OvrDebugLines & debugLines, VPosf & pose ) const
{
    debugLines.AddBounds( pose, bounds(), V4Vectf( 1.0f, 0.5f, 0.0f, 1.0f ) );

    V4Vectf color( 1.0f, 0.0f, 1.0f, 1.0f );
	for ( int i = 0; i < m_indices.length(); i += 3 )
	{
		int i1 = m_indices[i + 0];
		int i2 = m_indices[i + 1];
		int i3 = m_indices[i + 2];
        V3Vectf v1 = pose.Position + ( pose.Orientation * m_vertices[i1] );
        V3Vectf v2 = pose.Position + ( pose.Orientation * m_vertices[i2] );
        V3Vectf v3 = pose.Position + ( pose.Orientation * m_vertices[i3] );
		debugLines.AddLine( v1, v2, color, color, 0, true );
		debugLines.AddLine( v2, v3, color, color, 0, true );
		debugLines.AddLine( v3, v1, color, color, 0, true );
	}
}

NV_NAMESPACE_END
