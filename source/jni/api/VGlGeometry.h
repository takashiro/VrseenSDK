#pragma once

#include "../vglobal.h"

#include "../core/VArray.h"
#include "../core/VBasicmath.h"
#include "../core/MemBuffer.h"

NV_NAMESPACE_BEGIN

struct VertexAttribs
{
    VArray< V3Vectf > position;
    VArray< V3Vectf > normal;
    VArray< V3Vectf > tangent;
    VArray< V3Vectf > binormal;
    VArray< V4Vectf > color;
    VArray< V2Vectf > uv0;
    VArray< V2Vectf > uv1;
    VArray< V4Vecti > jointIndices;
    VArray< V4Vectf > jointWeights;
};

typedef unsigned short TriangleIndex;

static const int MAX_GEOMETRY_VERTICES	= 1 << ( sizeof( TriangleIndex ) * 8 );

static const int MAX_GEOMETRY_INDICES	= 1024 * 1024 * 3;

class VGlGeometry
{
public:
    VGlGeometry() :
            vertexBuffer( 0 ),
            indexBuffer( 0 ),
            vertexArrayObject( 0 ),
            vertexCount( 0 ),
            indexCount( 0 ) {}

    VGlGeometry( const VertexAttribs & attribs, const VArray< TriangleIndex > & indices ) :
            vertexBuffer( 0 ),
            indexBuffer( 0 ),
            vertexArrayObject( 0 ),
            vertexCount( 0 ),
            indexCount( 0 ) { Create( attribs, indices ); }

    void	Create( const VertexAttribs & attribs, const VArray< TriangleIndex > & indices );

    void	Update( const VertexAttribs & attribs );

    void	Draw() const;

    void	Free();

public:
    unsigned 	vertexBuffer;
    unsigned 	indexBuffer;
    unsigned 	vertexArrayObject;
    int			vertexCount;
    int 		indexCount;
};

class VGlGeometryFactory
{
public:

    static VGlGeometry CreateTesselatedQuad( const int horizontal, const int vertical, bool twoSided = false );

    static VGlGeometry CreateFadedScreenMask( const float xFraction, const float yFraction );

    static VGlGeometry CreateVignette( const float xFraction, const float yFraction );

    static VGlGeometry CreateTesselatedCylinder( const float radius, const float height,
                                      const int horizontal, const int vertical, const float uScale, const float vScale );

    static VGlGeometry CreateDome( const float latRads, const float uScale = 1.0f, const float vScale = 1.0f );

    static VGlGeometry CreateGlobe( const float uScale = 1.0f, const float vScale = 1.0f );

    static VGlGeometry CreateSpherePatch( const float fov );

    static VGlGeometry CreateCalibrationLines( const int extraLines, const bool fullGrid );

    static VGlGeometry CreateUnitCubeLines();

    static VGlGeometry CreateCalibrationLines2( const int extraLines, const bool fullGrid );

    static VGlGeometry CreateQuad();
};


NV_NAMESPACE_END

