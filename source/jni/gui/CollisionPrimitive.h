/************************************************************************************

Filename    :   CollisionPrimitive.h
Content     :   Generic collision class supporting ray / triangle intersection.
Created     :   September 10, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#if !defined( OVR_CollisionPrimitive_h )
#define OVR_CollisionPrimitive_h

#include "VFlags.h"
#include "../api/VGlGeometry.h" // For TriangleIndex
//#include "VArray.h"
NV_NAMESPACE_BEGIN

class OvrDebugLines;

enum eContentFlags
{
	CONTENT_SOLID,
	CONTENT_ALL = 0x7fffffff
};

typedef VFlags<eContentFlags> ContentFlags_t;

//==============================================================
// OvrCollisionResult
// Structure that holds the result of a collision
class OvrCollisionResult
{
public:
	OvrCollisionResult() :
		t( FLT_MAX ),
		uv( 0.0f ),
        triIndex( -1 )
	{
	}

	float			t;			// fraction along line where the intersection occurred
    V2Vectf		uv;			// texture coordinate of intersection
    int64_t			triIndex;	// index of triangle hit (local to collider)
};

//==============================================================
// OvrCollisionPrimitive
// Base class for a collision primitive.
class OvrCollisionPrimitive
{
public:
						OvrCollisionPrimitive() { }
                        OvrCollisionPrimitive( ContentFlags_t const contents ) : m_contents( contents ) { }
	virtual				~OvrCollisionPrimitive();

    virtual  bool		intersectRay( V3Vectf const & start, V3Vectf const & dir, VPosf const & pose,
                                V3Vectf const & scale, ContentFlags_t const testContents,
								OvrCollisionResult & result ) const = 0;
	// the ray should already be in local space
    virtual bool		intersectRay( V3Vectf const & localStart, V3Vectf const & localDir,
                                V3Vectf const & scale, ContentFlags_t const testContents,
								OvrCollisionResult & result ) const = 0;

	// test for ray intersection against only the AAB
    bool				intersectRayBounds( V3Vectf const & start, V3Vectf const & dir,
                                V3Vectf const & scale, ContentFlags_t const testContents,
								float & t0, float & t1 ) const;

    virtual void		debugRender( OvrDebugLines & debugLines, VPosf & pose ) const = 0;

    ContentFlags_t		contents() const { return m_contents; }
    void				setContents( ContentFlags_t const contents ) { m_contents = contents; }

    VBoxf const &	bounds() const { return m_bounds; }
    void				setBounds( VBoxf const & bounds ) { m_bounds = bounds; }

protected:
    bool				intersectRayBounds( V3Vectf const & start, V3Vectf const & dir,
                                V3Vectf const & scale, float & t0, float & t1 ) const;

private:
    ContentFlags_t		m_contents;	// flags dictating what can hit this collider
    VBoxf			m_bounds;		// Axial-aligned bounds of the primitive

};

//==============================================================
// OvrTriCollisionPrimitive
// Collider that handles collision vs. polygons and stores those polygons itself.
class OvrTriCollisionPrimitive : public OvrCollisionPrimitive
{
public:
	OvrTriCollisionPrimitive();
    OvrTriCollisionPrimitive( VArray< V3Vectf > const & vertices, VArray< ushort > const & indices,
			ContentFlags_t const contents );

	virtual	~OvrTriCollisionPrimitive();

    void				init( VArray< V3Vectf > const & vertices, VArray< ushort > const & indices,
								ContentFlags_t const contents );

    virtual  bool		intersectRay( V3Vectf const & start, V3Vectf const & dir, VPosf const & pose,
                                V3Vectf const & scale, ContentFlags_t const testContents,
								OvrCollisionResult & result ) const;


	// the ray should already be in local space
    virtual bool		intersectRay( V3Vectf const & localStart, V3Vectf const & localDir,
                                V3Vectf const & scale, ContentFlags_t const testContents,
								OvrCollisionResult & result ) const;

    virtual void		debugRender( OvrDebugLines & debugLines, VPosf & pose ) const;

private:
    VArray< V3Vectf >		m_vertices;	// vertices for all triangles
    VArray< ushort >	m_indices;	// indices indicating which vertices make up each triangle
};

NV_NAMESPACE_END

#endif // OVR_CollisionPrimitive_h
