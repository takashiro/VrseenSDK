/************************************************************************************

Filename    :   ModelTrace.cpp
Content     :   Ray tracer using a KD-Tree.
Created     :   May, 2014
Authors     :   J.M.P. van Waveren

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "ModelTrace.h"

#include <math.h>

#include "VAlgorithm.h"
#include "VArray.h"

namespace NervGear
{

bool Intersect_RayBounds( const V3Vectf & rayStart, const V3Vectf & rayDir,
                            const V3Vectf & mins, const V3Vectf & maxs,
							float & t0, float & t1 )
{
    const float rcpDirX = ( fabsf( rayDir.x ) > VConstants<float>::SmallestNonDenormal ) ? ( 1.0f / rayDir.x ) : VConstants<float>::HugeNumber;
    const float rcpDirY = ( fabsf( rayDir.y ) > VConstants<float>::SmallestNonDenormal ) ? ( 1.0f / rayDir.y ) : VConstants<float>::HugeNumber;
    const float rcpDirZ = ( fabsf( rayDir.z ) > VConstants<float>::SmallestNonDenormal ) ? ( 1.0f / rayDir.z ) : VConstants<float>::HugeNumber;

	const float sX = ( mins.x - rayStart.x ) * rcpDirX;
	const float sY = ( mins.y - rayStart.y ) * rcpDirY;
	const float sZ = ( mins.z - rayStart.z ) * rcpDirZ;

	const float tX = ( maxs.x - rayStart.x ) * rcpDirX;
	const float tY = ( maxs.y - rayStart.y ) * rcpDirY;
	const float tZ = ( maxs.z - rayStart.z ) * rcpDirZ;

	const float minX = VAlgorithm::Min( sX, tX );
	const float minY = VAlgorithm::Min( sY, tY );
	const float minZ = VAlgorithm::Min( sZ, tZ );

	const float maxX = VAlgorithm::Max( sX, tX );
	const float maxY = VAlgorithm::Max( sY, tY );
	const float maxZ = VAlgorithm::Max( sZ, tZ );

	t0 = VAlgorithm::Max( minX, VAlgorithm::Max( minY, minZ ) );
	t1 = VAlgorithm::Min( maxX, VAlgorithm::Min( maxY, maxZ ) );

	return ( t0 <= t1 );
}

bool Intersect_RayTriangle( const V3Vectf & rayStart, const V3Vectf & rayDir,
                            const V3Vectf & v0, const V3Vectf & v1, const V3Vectf & v2,
							float & t0, float & u, float & v )
{
	assert( rayDir.IsNormalized() );

    const V3Vectf edge1 = v1 - v0;
    const V3Vectf edge2 = v2 - v0;

    const V3Vectf tv = rayStart - v0;
    const V3Vectf pv = rayDir.Cross( edge2 );
    const V3Vectf qv = tv.Cross( edge1 );
	const float det = edge1.Dot( pv );

	// If the determinant is negative then the triangle is backfacing.
	if ( det <= 0.0f )
	{
		return false;
	}

	// This code has been modified to only perform a floating-point
	// division if the ray actually hits the triangle. If back facing
	// triangles are not culled then the sign of 's' and 't' need to
	// be flipped. This can be accomplished by multiplying the values
	// with the determinant instead of the reciprocal determinant.

	const float s = tv.Dot( pv );
	const float t = rayDir.Dot( qv );

	if ( s >= 0.0f && s <= det )
	{
		if ( t >= 0.0f && s + t <= det )
		{
			// If the determinant is almost zero then the ray lies in the triangle plane.
			// This comparison is done last because it is usually rare for
			// the ray to lay in the triangle plane.
            if ( fabsf( det ) > VConstants<float>::SmallestNonDenormal )
			{
				const float rcpDet = 1.0f / det;
				t0 = edge2.Dot( qv ) * rcpDet;
				u = s * rcpDet;
				v = t * rcpDet;
				return true;
			}
		}
	}

	return false;
}

/*

	Stackless KD-Tree Traversal for High Performance GPU Ray Tracing
	Stefan Popov, Johannes Günther, Hans-Peter Seidel, Philipp Slusallek
	Eurographics, Volume 26, Number 3, 2007

*/

const int RT_KDTREE_MAX_ITERATIONS	= 128;

traceResult_t ModelTrace::Trace( const V3Vectf & start, const V3Vectf & end ) const
{
	traceResult_t result;
	result.triangleIndex = -1;
	result.fraction = 1.0f;
    result.uv = V2Vectf( 0.0f );
    result.normal = V3Vectf( 0.0f );

    const V3Vectf rayDelta = end - start;
	const float rayLengthSqr = rayDelta.LengthSq();
    const float rayLengthRcp = VRcpSqrt( rayLengthSqr );
	const float rayLength = rayLengthSqr * rayLengthRcp;
    const V3Vectf rayDir = rayDelta * rayLengthRcp;

    const float rcpRayDirX = ( fabsf( rayDir.x ) > VConstants<float>::SmallestNonDenormal ) ? ( 1.0f / rayDir.x ) : VConstants<float>::HugeNumber;
    const float rcpRayDirY = ( fabsf( rayDir.y ) > VConstants<float>::SmallestNonDenormal ) ? ( 1.0f / rayDir.y ) : VConstants<float>::HugeNumber;
    const float rcpRayDirZ = ( fabsf( rayDir.z ) > VConstants<float>::SmallestNonDenormal ) ? ( 1.0f / rayDir.z ) : VConstants<float>::HugeNumber;

	const float sX = ( header.bounds.GetMins()[0] - start.x ) * rcpRayDirX;
	const float sY = ( header.bounds.GetMins()[1] - start.y ) * rcpRayDirY;
	const float sZ = ( header.bounds.GetMins()[2] - start.z ) * rcpRayDirZ;

	const float tX = ( header.bounds.GetMaxs()[0] - start.x ) * rcpRayDirX;
	const float tY = ( header.bounds.GetMaxs()[1] - start.y ) * rcpRayDirY;
	const float tZ = ( header.bounds.GetMaxs()[2] - start.z ) * rcpRayDirZ;

	const float minX = VAlgorithm::Min( sX, tX );
	const float minY = VAlgorithm::Min( sY, tY );
	const float minZ = VAlgorithm::Min( sZ, tZ );

	const float maxX = VAlgorithm::Max( sX, tX );
	const float maxY = VAlgorithm::Max( sY, tY );
	const float maxZ = VAlgorithm::Max( sZ, tZ );

	const float t0 = VAlgorithm::Max( minX, VAlgorithm::Max( minY, minZ ) );
	const float t1 = VAlgorithm::Min( maxX, VAlgorithm::Min( maxY, maxZ ) );

	if ( t0 >= t1 )
	{
		return result;
	}

	float entryDistance = VAlgorithm::Max( t0, 0.0f );
	float bestDistance = VAlgorithm::Min( t1 + 0.00001f, rayLength );
    V2Vectf uv;
	const kdtree_node_t * currentNode = &nodes[0];

	for ( int i = 0; i < RT_KDTREE_MAX_ITERATIONS; i++ )
	{
        const V3Vectf rayEntryPoint = start + rayDir * entryDistance;

		// Step down the tree until a leaf node is found.
		while ( ( currentNode->data & 1 ) == 0 )
		{
			// Select the child node based on whether the entry point is left or right of the split plane.
			// If the entry point is directly at the split plane then choose the side based on the ray direction.
			const int nodePlane = ( ( currentNode->data >> 1 ) & 3 );
			int child;
			if ( rayEntryPoint[nodePlane] - currentNode->dist < 0.00001f ) child = 0;
			else if ( rayEntryPoint[nodePlane] - currentNode->dist > 0.00001f ) child = 1;
			else child = ( rayDelta[nodePlane] > 0.0f );
			currentNode = &nodes[( currentNode->data >> 3 ) + child];
		}

		// Check for an intersection with a triangle in this leaf.
		const kdtree_leaf_t * currentLeaf = &leafs[( currentNode->data >> 3 )];
		const int * leafTriangles = currentLeaf->triangles;
		int leafTriangleCount = RT_KDTREE_MAX_LEAF_TRIANGLES;
		for ( int j = 0; j < leafTriangleCount; j++ )
		{
			int currentTriangle = leafTriangles[j];
			if ( currentTriangle < 0 )
			{
				if ( currentTriangle == -1 )
				{
					break;
				}

				const int offset = ( currentTriangle & 0x7FFFFFFF );
				leafTriangles = &overflow[offset];
				leafTriangleCount = header.numOverflow - offset;
				j = 0;
				currentTriangle = leafTriangles[0];
			}

			float distance;
			float u;
			float v;

			if ( Intersect_RayTriangle( start, rayDir,
										vertices[indices[currentTriangle * 3 + 0]],
										vertices[indices[currentTriangle * 3 + 1]],
										vertices[indices[currentTriangle * 3 + 2]], distance, u, v ) )
			{
				if ( distance >= 0.0f && distance < bestDistance )
				{
					bestDistance = distance;

					result.triangleIndex = currentTriangle * 3;
					uv.x = u;
					uv.y = v;
				}
			}
		}

		// Calculate the distance along the ray where the next leaf is entered.
		const float sX = ( currentLeaf->bounds.GetMins()[0] - start.x ) * rcpRayDirX;
		const float sY = ( currentLeaf->bounds.GetMins()[1] - start.y ) * rcpRayDirY;
		const float sZ = ( currentLeaf->bounds.GetMins()[2] - start.z ) * rcpRayDirZ;

		const float tX = ( currentLeaf->bounds.GetMaxs()[0] - start.x ) * rcpRayDirX;
		const float tY = ( currentLeaf->bounds.GetMaxs()[1] - start.y ) * rcpRayDirY;
		const float tZ = ( currentLeaf->bounds.GetMaxs()[2] - start.z ) * rcpRayDirZ;

		const float maxX = VAlgorithm::Max( sX, tX );
		const float maxY = VAlgorithm::Max( sY, tY );
		const float maxZ = VAlgorithm::Max( sZ, tZ );

		entryDistance = VAlgorithm::Min( maxX, VAlgorithm::Min( maxY, maxZ ) );
		if ( entryDistance >= bestDistance )
		{
			break;
		}

		// Calculate the exit plane.
		const int exitX = ( 0 << 1 ) | ( ( sX < tX ) ? 1 : 0 );
		const int exitY = ( 1 << 1 ) | ( ( sY < tY ) ? 1 : 0 );
		const int exitZ = ( 2 << 1 ) | ( ( sZ < tZ ) ? 1 : 0 );
		const int exitPlane = ( maxX < maxY ) ? ( maxX < maxZ ? exitX : exitZ ) : ( maxY < maxZ ? exitY : exitZ );

		// Use a rope to enter the adjacent leaf.
		const int exitNodeIndex = currentLeaf->ropes[exitPlane];
		if ( exitNodeIndex == -1 )
		{
			break;
		}

		currentNode = &nodes[exitNodeIndex];
	}

	if ( result.triangleIndex != -1 )
	{
		result.fraction = bestDistance * rayLengthRcp;
		result.uv = uvs[indices[result.triangleIndex + 0]] * ( 1.0f - uv.x - uv.y ) +
					uvs[indices[result.triangleIndex + 1]] * uv.x +
					uvs[indices[result.triangleIndex + 2]] * uv.y;
        const V3Vectf d1 = vertices[indices[result.triangleIndex + 1]] - vertices[indices[result.triangleIndex + 0]];
        const V3Vectf d2 = vertices[indices[result.triangleIndex + 2]] - vertices[indices[result.triangleIndex + 0]];
		result.normal = d1.Cross( d2 ).Normalized();
	}

	return result;
}

traceResult_t ModelTrace::Trace_Exhaustive( const V3Vectf & start, const V3Vectf & end ) const
{
	traceResult_t result;
	result.triangleIndex = -1;
	result.fraction = 1.0f;
    result.uv = V2Vectf( 0.0f );
    result.normal = V3Vectf( 0.0f );

    const V3Vectf rayDelta = end - start;
	const float rayLengthSqr = rayDelta.LengthSq();
    const float rayLengthRcp = VRcpSqrt( rayLengthSqr );
	const float rayLength = rayLengthSqr * rayLengthRcp;
    const V3Vectf rayStart = start;
    const V3Vectf rayDir = rayDelta * rayLengthRcp;

	float bestDistance = rayLength;
    V2Vectf uv;

	for ( int i = 0; i < header.numIndices; i += 3 )
	{
		float distance;
		float u;
		float v;

		if ( Intersect_RayTriangle( rayStart, rayDir,
									vertices[indices[i + 0]],
									vertices[indices[i + 1]],
									vertices[indices[i + 2]], distance, u, v ) )
		{
			if ( distance >= 0.0f && distance < bestDistance )
			{
				bestDistance = distance;

				result.triangleIndex = i;
				uv.x = u;
				uv.y = v;
			}
		}
	}

	if ( result.triangleIndex != -1 )
	{
		result.fraction = bestDistance * rayLengthRcp;
		result.uv = uvs[indices[result.triangleIndex + 0]] * ( 1.0f - uv.x - uv.y ) +
					uvs[indices[result.triangleIndex + 1]] * uv.x +
					uvs[indices[result.triangleIndex + 2]] * uv.y;
        const V3Vectf d1 = vertices[indices[result.triangleIndex + 1]] - vertices[indices[result.triangleIndex + 0]];
        const V3Vectf d2 = vertices[indices[result.triangleIndex + 2]] - vertices[indices[result.triangleIndex + 0]];
		result.normal = d1.Cross( d2 ).Normalized();
	}

	return result;
}

}
