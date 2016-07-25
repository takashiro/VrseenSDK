#pragma once

#include "vglobal.h"

#include "VArray.h"
#include "VVect.h"

NV_NAMESPACE_BEGIN

struct VertexAttribs
{
    VArray< VVect3f > position;
    VArray< VVect3f > normal;
    VArray< VVect3f > tangent;
    VArray< VVect3f > binormal;
    VArray< V4Vectf > color;
    VArray< VVect2f > uvCoordinate0;
    VArray< VVect2f > uvCoordinate1;
    VArray< V4Vecti > motionIndices;
    VArray< V4Vectf > motionWeight;
};



class VGlGeometry
{
public:
    VGlGeometry() :
            vertexBuffer( 0 ),
            indexBuffer( 0 ),
            vertexArrayObject( 0 ),
            vertexCount( 0 ),
            indexCount( 0 ) {}


    VGlGeometry( const VertexAttribs & attribs, const VArray< ushort > & indices ) :
            vertexBuffer( 0 ),
            indexBuffer( 0 ),
            vertexArrayObject( 0 ),
            vertexCount( 0 ),
            indexCount( 0 ) { createGlGeometry( attribs, indices ); }



    void createGlGeometry( const VertexAttribs & attribs, const VArray< ushort > & indices );
    void updateGlGeometry( const VertexAttribs & attribs );
    void drawElements() const;
    void destroy();
    void createPlaneQuadGrid( const int horizontal, const int vertical );
    void createScreenQuad( const float xx, const float yy );
    void createCylinder( const float radius, const float height,const int horizontal, const int vertical, const float  uScale = 1.0f, const float  vScale = 1.0f );
    void createStylePattern( const float xx, const float yy );
    void createDome( const float radius, const float uScale = 1.0f, const float vScale = 1.0f );
    void createSkybox( const float length, const float width, const float height );
    void createSphere( const float uScale = 1.0f, const float vScale = 1.0f );
    void createPartSphere( const float fov );
    void createCalibrationGrid( const int lines, const bool full );
    void createUnitCubeGrid();
    void createQuad();

public:
    unsigned 	vertexBuffer;
    unsigned 	indexBuffer;
    unsigned 	vertexArrayObject;
    int			vertexCount;
    int 		indexCount;
};




NV_NAMESPACE_END

