#include "VGlGeometry.h"

#include <math.h>
#include <string.h>
#include <assert.h>

#include "../core/Alg.h"
#include "../core/Android/GlUtils.h"
#include "../core/Android/LogUtils.h"
#include "Distortion.h"

#include "VGlShader.h"

/*
 * These are all built inside VertexArrayObjects, so no GL state other
 * than the VAO binding should be disturbed.
 *
 */

NV_NAMESPACE_BEGIN

template< typename _attrib_type_ >
void PackVertexAttribute( VArray< uint8_t > & packed, const VArray< _attrib_type_ > & attrib,
                          const int glLocation, const int glType, const int glComponents )
{
    if ( attrib.size() > 0 )
    {
        const size_t offset = packed.size();
        const size_t size = attrib.size() * sizeof( attrib[0] );

        packed.resize( offset + size );
        memcpy( &packed[offset], attrib.data(), size );

        glEnableVertexAttribArray( glLocation );
        glVertexAttribPointer( glLocation, glComponents, glType, false, sizeof( attrib[0] ), (void *)( offset ) );
    }
    else
    {
        glDisableVertexAttribArray( glLocation );
    }
}

void VGlGeometry::Create( const VertexAttribs & attribs, const VArray< TriangleIndex > & indices )
{
    vertexCount = attribs.position.length();
    indexCount = indices.length();

    glGenBuffers( 1, &vertexBuffer );
    glGenBuffers( 1, &indexBuffer );
    glGenVertexArraysOES_( 1, &vertexArrayObject );
    glBindVertexArrayOES_( vertexArrayObject );
    glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );

    VArray< uint8_t > packed;
    PackVertexAttribute( packed, attribs.position,		SHADER_ATTRIBUTE_LOCATION_POSITION,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.normal,		SHADER_ATTRIBUTE_LOCATION_NORMAL,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.tangent,		SHADER_ATTRIBUTE_LOCATION_TANGENT,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.binormal,		SHADER_ATTRIBUTE_LOCATION_BINORMAL,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.color,			SHADER_ATTRIBUTE_LOCATION_COLOR,			GL_FLOAT,	4 );
    PackVertexAttribute( packed, attribs.uv0,			SHADER_ATTRIBUTE_LOCATION_UV0,				GL_FLOAT,	2 );
    PackVertexAttribute( packed, attribs.uv1,			SHADER_ATTRIBUTE_LOCATION_UV1,				GL_FLOAT,	2 );
    PackVertexAttribute( packed, attribs.jointIndices,	SHADER_ATTRIBUTE_LOCATION_JOINT_INDICES,	GL_INT,		4 );
    PackVertexAttribute( packed, attribs.jointWeights,	SHADER_ATTRIBUTE_LOCATION_JOINT_WEIGHTS,	GL_FLOAT,	4 );

    glBufferData( GL_ARRAY_BUFFER, packed.size() * sizeof( packed[0] ), packed.data(), GL_STATIC_DRAW );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices.length() * sizeof( indices[0] ), indices.data(), GL_STATIC_DRAW );

    glBindVertexArrayOES_( 0 );

    glDisableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_POSITION );
    glDisableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_NORMAL );
    glDisableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_TANGENT );
    glDisableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_BINORMAL );
    glDisableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_COLOR );
    glDisableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_UV0 );
    glDisableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_UV1 );
    glDisableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_JOINT_INDICES );
    glDisableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_JOINT_WEIGHTS );
}

void VGlGeometry::Update( const VertexAttribs & attribs )
{
    vertexCount = attribs.position.length();

    glBindVertexArrayOES_( vertexArrayObject );

    glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );

    VArray< uint8_t > packed;
    PackVertexAttribute( packed, attribs.position,		SHADER_ATTRIBUTE_LOCATION_POSITION,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.normal,		SHADER_ATTRIBUTE_LOCATION_NORMAL,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.tangent,		SHADER_ATTRIBUTE_LOCATION_TANGENT,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.binormal,		SHADER_ATTRIBUTE_LOCATION_BINORMAL,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.color,			SHADER_ATTRIBUTE_LOCATION_COLOR,			GL_FLOAT,	4 );
    PackVertexAttribute( packed, attribs.uv0,			SHADER_ATTRIBUTE_LOCATION_UV0,				GL_FLOAT,	2 );
    PackVertexAttribute( packed, attribs.uv1,			SHADER_ATTRIBUTE_LOCATION_UV1,				GL_FLOAT,	2 );
    PackVertexAttribute( packed, attribs.jointIndices,	SHADER_ATTRIBUTE_LOCATION_JOINT_INDICES,	GL_INT,		4 );
    PackVertexAttribute( packed, attribs.jointWeights,	SHADER_ATTRIBUTE_LOCATION_JOINT_WEIGHTS,	GL_FLOAT,	4 );

    glBufferData( GL_ARRAY_BUFFER, packed.size() * sizeof( packed[0] ), packed.data(), GL_STATIC_DRAW );
}

void VGlGeometry::Draw() const
{
    glBindVertexArrayOES_( vertexArrayObject );
    glDrawElements( GL_TRIANGLES, indexCount, ( sizeof( TriangleIndex ) == 2 ) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, NULL );
}

void VGlGeometry::Free()
{
    glDeleteVertexArraysOES_( 1, &vertexArrayObject );
    glDeleteBuffers( 1, &indexBuffer );
    glDeleteBuffers( 1, &vertexBuffer );

    indexBuffer = 0;
    vertexBuffer = 0;
    vertexArrayObject = 0;
    vertexCount = 0;
    indexCount = 0;
}

VGlGeometry VGlGeometryFactory::CreateFadedScreenMask( const float xFraction, const float yFraction )
{
    const float posx[] = { -1.001f, -1.0f + xFraction * 0.25f, -1.0f + xFraction, 1.0f - xFraction, 1.0f - xFraction * 0.25f, 1.001f };
    const float posy[] = { -1.001f, -1.0f + yFraction * 0.25f, -1.0f + yFraction, 1.0f - yFraction, 1.0f - yFraction * 0.25f, 1.001f };

    const int vertexCount = 6 * 6;

    VertexAttribs attribs;
    attribs.position.resize( vertexCount );
    attribs.uv0.resize( vertexCount );
    attribs.color.resize( vertexCount );

    for ( int y = 0; y < 6; y++ )
    {
        for ( int x = 0; x < 6; x++ )
        {
            const int index = y * 6 + x;
            attribs.position[index].x = posx[x];
            attribs.position[index].y = posy[y];
            attribs.position[index].z = 0.0f;
            attribs.uv0[index].x = 0.0f;
            attribs.uv0[index].y = 0.0f;
            // the outer edges will have 0 color
            const float c = ( y <= 1 || y >= 4 || x <= 1 || x >= 4 ) ? 0.0f : 1.0f;
            for ( int i = 0; i < 3; i++ )
            {
                attribs.color[index][i] = c;
            }
            attribs.color[index][3] = 1.0f;	// solid alpha
        }
    }

    VArray< NervGear::TriangleIndex > indices;
    indices.resize( 25 * 6 );

    // Should we flip the triangulation on the corners?
    int index = 0;
    for ( int x = 0; x < 5; x++ )
    {
        for ( int y = 0; y < 5; y++ )
        {
            indices[index + 0] = y * 6 + x;
            indices[index + 1] = y * 6 + x + 1;
            indices[index + 2] = (y + 1) * 6 + x;
            indices[index + 3] = (y + 1) * 6 + x;
            indices[index + 4] = y * 6 + x + 1;
            indices[index + 5] = (y + 1) * 6 + x + 1;
            index += 6;
        }
    }

    return VGlGeometry( attribs, indices );
}

VGlGeometry VGlGeometryFactory::CreateTesselatedQuad( const int horizontal, const int vertical,
                                const bool twoSided )
{
    const int vertexCount = ( horizontal + 1 ) * ( vertical + 1 );

    VertexAttribs attribs;
    attribs.position.resize( vertexCount );
    attribs.uv0.resize( vertexCount );
    attribs.color.resize( vertexCount );

    for ( int y = 0; y <= vertical; y++ )
    {
        const float yf = (float) y / (float) vertical;
        for ( int x = 0; x <= horizontal; x++ )
        {
            const float xf = (float) x / (float) horizontal;
            const int index = y * ( horizontal + 1 ) + x;
            attribs.position[index].x = -1 + xf * 2;
            attribs.position[index].z = 0;
            attribs.position[index].y = -1 + yf * 2;
            attribs.uv0[index].x = xf;
            attribs.uv0[index].y = 1.0 - yf;
            for ( int i = 0; i < 4; i++ )
            {
                attribs.color[index][i] = 1.0f;
            }
            // fade to transparent on the outside
            if ( x == 0 || x == horizontal || y == 0 || y == vertical )
            {
                attribs.color[index][3] = 0.0f;
            }
        }
    }

    VArray< TriangleIndex > indices;
    indices.resize( horizontal * vertical * 6 * ( twoSided ? 2 : 1 ) );

    // If this is to be used to draw a linear format texture, like
    // a surface texture, it is better for cache performance that
    // the triangles be drawn to follow the side to side linear order.
    int index = 0;
    for ( int y = 0; y < vertical; y++ )
    {
        for ( int x = 0; x < horizontal; x++ )
        {
            indices[index + 0] = y * (horizontal + 1) + x;
            indices[index + 1] = y * (horizontal + 1) + x + 1;
            indices[index + 2] = (y + 1) * (horizontal + 1) + x;
            indices[index + 3] = (y + 1) * (horizontal + 1) + x;
            indices[index + 4] = y * (horizontal + 1) + x + 1;
            indices[index + 5] = (y + 1) * (horizontal + 1) + x + 1;
            index += 6;
        }
    }
    if ( twoSided )
    {
        for ( int y = 0; y < vertical; y++ )
        {
            for ( int x = 0; x < horizontal; x++ )
            {
                indices[index + 5] = y * (horizontal + 1) + x;
                indices[index + 4] = y * (horizontal + 1) + x + 1;
                indices[index + 3] = (y + 1) * (horizontal + 1) + x;
                indices[index + 2] = (y + 1) * (horizontal + 1) + x;
                indices[index + 1] = y * (horizontal + 1) + x + 1;
                indices[index + 0] = (y + 1) * (horizontal + 1) + x + 1;
                index += 6;
            }
        }
    }
    return VGlGeometry( attribs, indices );
}

VGlGeometry VGlGeometryFactory::CreateTesselatedCylinder( const float radius, const float height, const int horizontal, const int vertical, const float uScale, const float vScale )
{
    const int vertexCount = ( horizontal + 1 ) * ( vertical + 1 );

    VertexAttribs attribs;
    attribs.position.resize( vertexCount );
    attribs.uv0.resize( vertexCount );
    attribs.color.resize( vertexCount );

    for ( int y = 0; y <= vertical; ++y )
    {
        const float yf = (float) y / (float) vertical;
        for ( int x = 0; x <= horizontal; ++x )
        {
            const float xf = (float) x / (float) horizontal;
            const int index = y * ( horizontal + 1 ) + x;
            attribs.position[index].x = cosf( M_PI * 2 * xf ) * radius;
            attribs.position[index].y = sinf( M_PI * 2 * xf ) * radius;
            attribs.position[index].z = -height + yf * 2 * height;
            attribs.uv0[index].x = xf * uScale;
            attribs.uv0[index].y = ( 1.0f - yf ) * vScale;
            for ( int i = 0; i < 4; ++i )
            {
                attribs.color[index][i] = 1.0f;
            }
            // fade to transparent on the outside
            if ( y == 0 || y == vertical )
            {
                attribs.color[index][3] = 0.0f;
            }
        }
    }

    VArray< TriangleIndex > indices;
    indices.resize( horizontal * vertical * 6 );

    // If this is to be used to draw a linear format texture, like
    // a surface texture, it is better for cache performance that
    // the triangles be drawn to follow the side to side linear order.
    int index = 0;
    for ( int y = 0; y < vertical; y++ )
    {
        for ( int x = 0; x < horizontal; x++ )
        {
            indices[index + 0] = y * (horizontal + 1) + x;
            indices[index + 1] = y * (horizontal + 1) + x + 1;
            indices[index + 2] = (y + 1) * (horizontal + 1) + x;
            indices[index + 3] = (y + 1) * (horizontal + 1) + x;
            indices[index + 4] = y * (horizontal + 1) + x + 1;
            indices[index + 5] = (y + 1) * (horizontal + 1) + x + 1;
            index += 6;
        }
    }

    return VGlGeometry( attribs, indices );
}

// To guarantee that the edge pixels are completely black, we need to
// have a band of solid 0.  Just interpolating to 0 at the edges will
// leave some pixels with low color values.  This stuck out as surprisingly
// visible smears from the distorted edges of the eye renderings in
// some cases.
VGlGeometry VGlGeometryFactory::CreateVignette( const float xFraction, const float yFraction )
{
    // Leave 25% of the vignette as solid black
    const float posx[] = { -1.001f, -1.0f + xFraction * 0.25f, -1.0f + xFraction, 1.0f - xFraction, 1.0f - xFraction * 0.25f, 1.001f };
    const float posy[] = { -1.001f, -1.0f + yFraction * 0.25f, -1.0f + yFraction, 1.0f - yFraction, 1.0f - yFraction * 0.25f, 1.001f };

    const int vertexCount = 6 * 6;

    VertexAttribs attribs;
    attribs.position.resize( vertexCount );
    attribs.uv0.resize( vertexCount );
    attribs.color.resize( vertexCount );

    for ( int y = 0; y < 6; y++ )
    {
        for ( int x = 0; x < 6; x++ )
        {
            const int index = y * 6 + x;
            attribs.position[index].x = posx[x];
            attribs.position[index].y = posy[y];
            attribs.position[index].z = 0.0f;
            attribs.uv0[index].x = 0.0f;
            attribs.uv0[index].y = 0.0f;
            // the outer edges will have 0 color
            const float c = ( y <= 1 || y >= 4 || x <= 1 || x >= 4 ) ? 0.0f : 1.0f;
            for ( int i = 0; i < 3; i++ )
            {
                attribs.color[index][i] = c;
            }
            attribs.color[index][3] = 1.0f;	// solid alpha
        }
    }

    VArray< TriangleIndex > indices;
    indices.resize( 24 * 6 );

    int index = 0;
    for ( int x = 0; x < 5; x++ )
    {
        for ( int y = 0; y < 5; y++ )
        {
            if ( x == 2 && y == 2 )
            {
                continue;	// the middle is open
            }
            // flip triangulation at corners
            if ( x == y )
            {
                indices[index + 0] = y * 6 + x;
                indices[index + 1] = (y + 1) * 6 + x + 1;
                indices[index + 2] = (y + 1) * 6 + x;
                indices[index + 3] = y * 6 + x;
                indices[index + 4] = y * 6 + x + 1;
                indices[index + 5] = (y + 1) * 6 + x + 1;
            }
            else
            {
                indices[index + 0] = y * 6 + x;
                indices[index + 1] = y * 6 + x + 1;
                indices[index + 2] = (y + 1) * 6 + x;
                indices[index + 3] = (y + 1) * 6 + x;
                indices[index + 4] = y * 6 + x + 1;
                indices[index + 5] = (y + 1) * 6 + x + 1;
            }
            index += 6;
        }
    }

    return VGlGeometry( attribs, indices );
}

VGlGeometry VGlGeometryFactory::CreateDome( const float latRads, const float uScale, const float vScale )
{
    const int horizontal = 64;
    const int vertical = 32;
    const float radius = 100.0f;

    const int vertexCount = ( horizontal + 1 ) * ( vertical + 1 );

    VertexAttribs attribs;
    attribs.position.resize( vertexCount );
    attribs.uv0.resize( vertexCount );
    attribs.color.resize( vertexCount );

    for ( int y = 0; y <= vertical; y++ )
    {
        const float yf = (float) y / (float) vertical;
        const float lat = M_PI - yf * latRads - 0.5f * M_PI;
        const float cosLat = cosf( lat );
        for ( int x = 0; x <= horizontal; x++ )
        {
            const float xf = (float) x / (float) horizontal;
            const float lon = ( 0.5f + xf ) * M_PI * 2;
            const int index = y * ( horizontal + 1 ) + x;

            if ( x == horizontal )
            {
                // Make sure that the wrap seam is EXACTLY the same
                // xyz so there is no chance of pixel cracks.
                attribs.position[index] = attribs.position[y * ( horizontal + 1 ) + 0];
            }
            else
            {
                attribs.position[index].x = radius * cosf( lon ) * cosLat;
                attribs.position[index].z = radius * sinf( lon ) * cosLat;
                attribs.position[index].y = radius * sinf( lat );
            }

            attribs.uv0[index].x = xf * uScale;
            attribs.uv0[index].y = ( 1.0f - yf ) * vScale;
            for ( int i = 0; i < 4; i++ )
            {
                attribs.color[index][i] = 1.0f;
            }
        }
    }

    VArray< TriangleIndex > indices;
    indices.resize( horizontal * vertical * 6 );

    int index = 0;
    for ( int x = 0; x < horizontal; x++ )
    {
        for ( int y = 0; y < vertical; y++ )
        {
            indices[index + 0] = y * (horizontal + 1) + x;
            indices[index + 1] = y * (horizontal + 1) + x + 1;
            indices[index + 2] = (y + 1) * (horizontal + 1) + x;
            indices[index + 3] = (y + 1) * (horizontal + 1) + x;
            indices[index + 4] = y * (horizontal + 1) + x + 1;
            indices[index + 5] = (y + 1) * (horizontal + 1) + x + 1;
            index += 6;
        }
    }

    return VGlGeometry( attribs, indices );
}

VGlGeometry VGlGeometryFactory::CreateGlobe( const float uScale, const float vScale )
{
    // Make four rows at the polar caps in the place of one
    // to diminish the degenerate triangle issue.
    const int poleVertical = 3;
    const int uniformVertical = 64;
    const int horizontal = 128;
    const int vertical = uniformVertical + poleVertical*2;
    const float radius = 100.0f;

    const int vertexCount = ( horizontal + 1 ) * ( vertical + 1 );

    VertexAttribs attribs;
    attribs.position.resize( vertexCount );
    attribs.uv0.resize( vertexCount );
    attribs.color.resize( vertexCount );

    for ( int y = 0; y <= vertical; y++ )
    {
        float yf;
        if ( y <= poleVertical )
        {
            yf = (float)y / (poleVertical+1) / uniformVertical;
        }
        else if ( y >= vertical - poleVertical )
        {
            yf = (float) (uniformVertical - 1 + ( (float)( y - (vertical - poleVertical - 1) ) / ( poleVertical+1) ) ) / uniformVertical;
        }
        else
        {
            yf = (float) ( y - poleVertical ) / uniformVertical;
        }
        const float lat = ( yf - 0.5f ) * M_PI;
        const float cosLat = cosf( lat );
        for ( int x = 0; x <= horizontal; x++ )
        {
            const float xf = (float) x / (float) horizontal;
            const float lon = ( 0.5f + xf ) * M_PI * 2;
            const int index = y * ( horizontal + 1 ) + x;

            if ( x == horizontal )
            {
                // Make sure that the wrap seam is EXACTLY the same
                // xyz so there is no chance of pixel cracks.
                attribs.position[index] = attribs.position[y * ( horizontal + 1 ) + 0];
            }
            else
            {
                attribs.position[index].x = radius * cosf( lon ) * cosLat;
                attribs.position[index].z = radius * sinf( lon ) * cosLat;
                attribs.position[index].y = radius * sinf( lat );
            }

            // With a normal mapping, half the triangles degenerate at the poles,
            // which causes seams between every triangle.  It is better to make them
            // a fan, and only get one seam.
            if ( y == 0 || y == vertical )
            {
                attribs.uv0[index].x = 0.5f;
            }
            else
            {
                attribs.uv0[index].x = xf * uScale;
            }
            attribs.uv0[index].y = ( 1.0 - yf ) * vScale;
            for ( int i = 0; i < 4; i++ )
            {
                attribs.color[index][i] = 1.0f;
            }
        }
    }

    VArray< TriangleIndex > indices;
    indices.resize( horizontal * vertical * 6 );

    int index = 0;
    for ( int x = 0; x < horizontal; x++ )
    {
        for ( int y = 0; y < vertical; y++ )
        {
            indices[index + 0] = y * (horizontal + 1) + x;
            indices[index + 1] = y * (horizontal + 1) + x + 1;
            indices[index + 2] = (y + 1) * (horizontal + 1) + x;
            indices[index + 3] = (y + 1) * (horizontal + 1) + x;
            indices[index + 4] = y * (horizontal + 1) + x + 1;
            indices[index + 5] = (y + 1) * (horizontal + 1) + x + 1;
            index += 6;
        }
    }

    return VGlGeometry( attribs, indices );
}

VGlGeometry VGlGeometryFactory::CreateSpherePatch( const float fov )
{
    const int horizontal = 64;
    const int vertical = 64;
    const float radius = 100.0f;

    const int vertexCount = (horizontal + 1) * (vertical + 1);

    VertexAttribs attribs;
    attribs.position.resize( vertexCount );
    attribs.uv0.resize( vertexCount );
    attribs.color.resize( vertexCount );

    for ( int y = 0; y <= vertical; y++ )
    {
        const float yf = (float) y / (float) vertical;
        const float lat = (yf - 0.5) * fov;
        const float cosLat = cosf(lat);
        for ( int x = 0; x <= horizontal; x++ )
        {
            const float xf = (float) x / (float) horizontal;
            const float lon = ( xf - 0.5f ) * fov;
            const int index = y * ( horizontal + 1 ) + x;

            attribs.position[index].x = radius * cosf( lon ) * cosLat;
            attribs.position[index].z = radius * sinf( lon ) * cosLat;
            attribs.position[index].y = radius * sinf( lat );

            // center in the middle of the screen for roll rotation
            attribs.uv0[index].x = xf - 0.5f;
            attribs.uv0[index].y = ( 1.0f - yf ) - 0.5f;

            for ( int i = 0 ; i < 4 ; i++ )
            {
                attribs.color[index][i] = 1.0f;
            }
        }
    }

    VArray< TriangleIndex > indices;
    indices.resize( horizontal * vertical * 6 );

    int index = 0;
    for ( int x = 0; x < horizontal; x++ )
    {
        for ( int y = 0; y < vertical; y++ )
        {
            indices[index + 0] = y * (horizontal + 1) + x;
            indices[index + 1] = y * (horizontal + 1) + x + 1;
            indices[index + 2] = (y + 1) * (horizontal + 1) + x;
            indices[index + 3] = (y + 1) * (horizontal + 1) + x;
            indices[index + 4] = y * (horizontal + 1) + x + 1;
            indices[index + 5] = (y + 1) * (horizontal + 1) + x + 1;
            index += 6;
        }
    }

    return VGlGeometry( attribs, indices );
}

VGlGeometry VGlGeometryFactory::CreateCalibrationLines( const int extraLines, const bool fullGrid )
{
    // lines per axis
    const int lineCount = 1 + extraLines * 2;
    const int vertexCount = lineCount * 2 * 2;

    VertexAttribs attribs;
    attribs.position.resize( vertexCount );
    attribs.uv0.resize( vertexCount );
    attribs.color.resize( vertexCount );

    for ( int y = 0; y < lineCount; y++ )
    {
        const float yf = ( lineCount == 1 ) ? 0.5f : (float) y / (float) ( lineCount - 1 );
        for ( int x = 0; x <= 1; x++ )
        {
            // along x
            const int v1 = 2 * ( y * 2 + x ) + 0;
            attribs.position[v1].x = -1 + x * 2;
            attribs.position[v1].z = -1.001f;	// keep the -1 and 1 just off the projection edges
            attribs.position[v1].y = -1 + yf * 2;
            attribs.uv0[v1].x = x;
            attribs.uv0[v1].y = 1.0f - yf;
            for ( int i = 0; i < 4; i++ )
            {
                attribs.color[v1][i] = 1.0f;
            }

            // swap y and x to go along y
            const int v2 = 2 * ( y * 2 + x ) + 1;
            attribs.position[v2].y = -1 + x * 2;
            attribs.position[v2].z = -1.001f;	// keep the -1 and 1 just off the projection edges
            attribs.position[v2].x = -1 + yf * 2;
            attribs.uv0[v2].x = x;
            attribs.uv0[v2].y = 1.0f - yf;
            for ( int i = 0; i < 4; i++ )
            {
                attribs.color[v2][i] = 1.0f;
            }

            if ( !fullGrid && y != extraLines )
            {	// make a short hash instead of a full line
                attribs.position[v1].x *= 0.02f;
                attribs.position[v2].y *= 0.02f;
            }
        }
    }

    VArray< TriangleIndex > indices;
    indices.resize( lineCount * 4 );

    int index = 0;
    for ( int x = 0; x < lineCount; x++ )
    {
        const int start = x * 4;

        indices[index + 0] = start;
        indices[index + 1] = start + 2;

        indices[index + 2] = start + 1;
        indices[index + 3] = start + 3;

        index += 4;
    }
    return VGlGeometry( attribs, indices );
}

VGlGeometry VGlGeometryFactory::CreateUnitCubeLines()
{
    VertexAttribs attribs;
    attribs.position.resize( 8 );

    for ( int i = 0; i < 8; i++) {
        attribs.position[i][0] = i & 1;
        attribs.position[i][1] = ( i & 2 ) >> 1;
        attribs.position[i][2] = ( i & 4 ) >> 2;
    }

    const TriangleIndex staticIndices[24] = { 0,1, 1,3, 3,2, 2,0, 4,5, 5,7, 7,6, 6,4, 0,4, 1,5, 3,7, 2,6 };

    VArray< TriangleIndex > indices;
    indices.resize( 24 );
    memcpy( &indices[0], staticIndices, 24 * sizeof( indices[0] ) );

    return VGlGeometry( attribs, indices );
}

VGlGeometry VGlGeometryFactory::CreateQuad()
{
    VGlGeometry geometry;
    struct vertices_t
    {
        float positions[4][3];
        float uvs[4][2];
        float colors[4][4];
    }
            vertices =
            {
                    { { -1, -1, 0 }, { -1, 1, 0 }, { 1, -1, 0 }, { 1, 1, 0 } },
                    { { 0, 1 }, { 1, 1 }, { 0, 0 }, { 1, 0 } },
                    { { 1, 1, 1, 0 }, { 1, 1, 1, 0 }, { 1, 1, 1, 0 }, { 1, 1, 1, 0 } }
            };
    unsigned short indices[6] = { 0, 1, 2, 2, 1, 3 };

    geometry.vertexCount = 4;
    geometry.indexCount = 6;

    glGenVertexArraysOES_( 1, &geometry.vertexArrayObject );
    glBindVertexArrayOES_( geometry.vertexArrayObject );

    glGenBuffers( 1, &geometry.vertexBuffer );
    glBindBuffer( GL_ARRAY_BUFFER, geometry.vertexBuffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), &vertices, GL_STATIC_DRAW );

    glGenBuffers( 1, &geometry.indexBuffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, geometry.indexBuffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );

    glEnableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_POSITION );
    glVertexAttribPointer( SHADER_ATTRIBUTE_LOCATION_POSITION, 3, GL_FLOAT, false,
                           sizeof( vertices.positions[0] ), (const GLvoid *)offsetof( vertices_t, positions ) );

    glEnableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_UV0 );
    glVertexAttribPointer( SHADER_ATTRIBUTE_LOCATION_UV0, 2, GL_FLOAT, false,
                           sizeof( vertices.uvs[0] ), (const GLvoid *)offsetof( vertices_t, uvs ) );

    glEnableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_COLOR );
    glVertexAttribPointer( SHADER_ATTRIBUTE_LOCATION_COLOR, 4, GL_FLOAT, false,
                           sizeof( vertices.colors[0] ), (const GLvoid *)offsetof( vertices_t, colors ) );

    glBindVertexArrayOES_( 0 );

    return  geometry;
}

// Assume the cursor is a small region around the center of projection,
// extended horizontally to allow it to be at different 3D depths.
// While this will be around the center of each screen half on Note 4,
// smaller phone screens will be offset, and an automated distortion
// calibration might also not be completely symmetric.
//
// There is a hazard here, because the area up to the next vertex
// past this will be drawn, so the density of tesselation effects
// the area that the cursor will show in.
static bool VectorHitsCursor( const V2Vectf & v )
{
    if ( fabs( v.y ) > 0.017f ) // +/- 1 degree vertically
    {
        return false;
    }
    if ( fabs( v.x ) > 0.070f ) // +/- 4 degree horizontally
    {
        return false;
    }
    return true;
}

// The cursorOnly flag will cull all triangles that aren't in the central
// gaze cursor region.
VGlGeometry VGlGeometryFactory::LoadMeshFromMemory( const MemBuffer & buf,
                                       const int numSlicesPerEye, const float fovScale, const bool cursorOnly )
{
    VGlGeometry geometry;

    if ( buf.length < 12 )
    {
        LOG( "bad buf.length %i", buf.length );
        return geometry;
    }

    const int magic = ((int *)buf.buffer)[0];
    const int tesselationsX = ((int *)buf.buffer)[1];
    const int tesselationsY = ((int *)buf.buffer)[2];
    const int totalX = (tesselationsX+1)*2;

    const int vertexBytes = 12 + 2 * (tesselationsX+1) * (tesselationsY+1) * 6 * sizeof( float );
    if ( buf.length != vertexBytes )
    {
        LOG( "buf.length %i != %i", buf.length, vertexBytes );
        return geometry;
    }
    if ( magic != DISTORTION_BUFFER_MAGIC || tesselationsX < 1 || tesselationsY < 1 )
    {
        LOG( "Bad distortion header" );
        return geometry;
    }
    const float * bufferVerts = &((float *)buf.buffer)[3];

    // Identify which verts would be inside the cursor plane
    bool * vertInCursor = NULL;
    if ( cursorOnly )
    {
        vertInCursor = new bool[totalX*(tesselationsY+1)];
        for ( int y = 0; y <= tesselationsY; y++ )
        {
            for ( int x = 0; x < totalX; x++ )
            {
                const int vertIndex = (y*totalX+x );
                V2Vectf	v;
                for ( int i = 0 ; i < 2; i++ )
                {
                    v[i] = fovScale * bufferVerts[vertIndex*6+i];
                }
                vertInCursor[ vertIndex ] = VectorHitsCursor( v );
            }
        }
    }

    // build a VertexArrayObject
    glGenVertexArraysOES_( 1, &geometry.vertexArrayObject );
    glBindVertexArrayOES_( geometry.vertexArrayObject );

    const int attribCount = 10;
    const int sliceTess = tesselationsX / numSlicesPerEye;

    const int vertexCount = 2*numSlicesPerEye*(sliceTess+1)*(tesselationsY+1);
    const int floatCount = vertexCount * attribCount;
    float * tessVertices = new float[floatCount];

    const int indexCount = 2*tesselationsX*tesselationsY*6;
    unsigned short * tessIndices = new unsigned short[indexCount];
    const int indexBytes = indexCount * sizeof( *tessIndices );

    int	index = 0;
    int	verts = 0;

    for ( int eye = 0; eye < 2; eye++ )
    {
        for ( int slice = 0; slice < numSlicesPerEye; slice++ )
        {
            const int vertBase = verts;

            for ( int y = 0; y <= tesselationsY; y++ )
            {
                const float	yf = (float)y / (float)tesselationsY;
                for ( int x = 0; x <= sliceTess; x++ )
                {
                    const int sx = slice * sliceTess + x;
                    const float	xf = (float)sx / (float)tesselationsX;
                    float * v = &tessVertices[attribCount * ( vertBase + y * (sliceTess+1) + x ) ];
                    v[0] = -1.0 + eye + xf;
                    v[1] = yf*2.0f - 1.0f;

                    // Copy the offsets from the file
                    for ( int i = 0 ; i < 6; i++ )
                    {
                        v[2+i] = fovScale * bufferVerts
                        [(y*(tesselationsX+1)*2+sx + eye * (tesselationsX+1))*6+i];
                    }

                    v[8] = (float)x / sliceTess;
                    // Enable this to allow fading at the edges.
                    // Samsung recommends not doing this, because it could cause
                    // visible differences in pixel wear on the screen over long
                    // periods of time.
                    if ( 0 && ( y == 0 || y == tesselationsY || sx == 0 || sx == tesselationsX ) )
                    {
                        v[9] = 0.0f;	// fade to black at edge
                    }
                    else
                    {
                        v[9] = 1.0f;
                    }
                }
            }
            verts += (tesselationsY+1)*(sliceTess+1);

            // The order of triangles doesn't matter for tiled rendering,
            // but when we can do direct rendering to the screen, we want the
            // order to follow the raster order to minimize the chance
            // of tear lines.
            //
            // This can be checked by quartering the number of indexes, and
            // making sure that the drawn pixels are the first pixels that
            // the raster will scan.
            for ( int x = 0; x < sliceTess; x++ )
            {
                for ( int y = 0; y < tesselationsY; y++ )
                {
                    if ( vertInCursor )
                    {	// skip this quad if none of the verts are in the cursor region
                        const int xx = x + eye * (tesselationsX+1) + slice * sliceTess;
                        if ( 0 ==
                             vertInCursor[ y * totalX + xx ]
                             + vertInCursor[ y * totalX + xx + 1 ]
                             + vertInCursor[ (y + 1) * totalX + xx ]
                             + vertInCursor[ (y + 1) * totalX + xx + 1 ] )
                        {
                            continue;
                        }
                    }

                    // flip the triangulation in opposite corners
                    if ( (slice*sliceTess+x <  tesselationsX/2) ^ (y < (tesselationsY/2)) )
                    {
                        tessIndices[index+0] = vertBase + y * (sliceTess+1) + x;
                        tessIndices[index+1] = vertBase + y * (sliceTess+1) + x + 1;
                        tessIndices[index+2] = vertBase + (y+1) * (sliceTess+1) + x + 1;

                        tessIndices[index+3] = vertBase + y * (sliceTess+1) + x;
                        tessIndices[index+4] = vertBase + (y+1) * (sliceTess+1) + x + 1;
                        tessIndices[index+5] = vertBase + (y+1) * (sliceTess+1) + x;
                    }
                    else
                    {
                        tessIndices[index+0] = vertBase + y * (sliceTess+1) + x;
                        tessIndices[index+1] = vertBase + y * (sliceTess+1) + x + 1;
                        tessIndices[index+2] = vertBase + (y+1) * (sliceTess+1) + x;

                        tessIndices[index+3] = vertBase + (y+1) * (sliceTess+1) + x;
                        tessIndices[index+4] = vertBase + y * (sliceTess+1) + x + 1;
                        tessIndices[index+5] = vertBase + (y+1) * (sliceTess+1) + x + 1;
                    }
                    index += 6;
                }
            }
        }
    }

    geometry.vertexCount = vertexCount;
    geometry.indexCount = index;

    glGenBuffers( 1, &geometry.vertexBuffer );
    glBindBuffer( GL_ARRAY_BUFFER, geometry.vertexBuffer );
    glBufferData( GL_ARRAY_BUFFER, floatCount * sizeof(*tessVertices), (void *)tessVertices, GL_STATIC_DRAW );
    delete[] tessVertices;

    glGenBuffers( 1, &geometry.indexBuffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, geometry.indexBuffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, indexBytes, (void *)tessIndices, GL_STATIC_DRAW );
    delete[] tessIndices;

    glEnableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_POSITION );
    glVertexAttribPointer( SHADER_ATTRIBUTE_LOCATION_POSITION, 2, GL_FLOAT, false, attribCount * sizeof( float ), (void *)( 0 * sizeof( float ) ) );

    glEnableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_NORMAL );
    glVertexAttribPointer( SHADER_ATTRIBUTE_LOCATION_NORMAL, 2, GL_FLOAT, false, attribCount * sizeof( float ), (void *)( 2 * sizeof( float ) ) );

    glEnableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_UV0 );
    glVertexAttribPointer( SHADER_ATTRIBUTE_LOCATION_UV0, 2, GL_FLOAT, false, attribCount * sizeof( float ), (void *)( 4 * sizeof( float ) ) );

    glEnableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_TANGENT );
    glVertexAttribPointer( SHADER_ATTRIBUTE_LOCATION_TANGENT, 2, GL_FLOAT, false, attribCount * sizeof( float ), (void *)( 6 * sizeof( float ) ) );

    glEnableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_UV1 );
    glVertexAttribPointer( SHADER_ATTRIBUTE_LOCATION_UV1, 2, GL_FLOAT, false, attribCount * sizeof( float ), (void *)( 8 * sizeof( float ) ) );

    glBindVertexArrayOES_( 0 );

    delete[] vertInCursor;

    return geometry;
}

VGlGeometry VGlGeometryFactory::CreateCalibrationLines2( const int extraLines, const bool fullGrid )
{
    // lines per axis
    const int lineCount = 1 + extraLines * 2;
    const int vertexCount = lineCount * 2 * 2;

    struct vertex_t
    {
        float x, y, z;
        float s, t;
        float color[4];
    };

    vertex_t * vertices = new vertex_t[vertexCount];

    for ( int y = 0; y < lineCount; y++ )
    {
        const float yf = ( lineCount == 1 ) ? 0.5f : (float) y / (float) ( lineCount - 1 );
        for ( int x = 0; x <= 1; x++ )
        {
            // along x
            const int v1 = 2 * ( y * 2 + x ) + 0;
            vertices[v1].x = -1 + x * 2;
            vertices[v1].z = -1.001f;	// keep the -1 and 1 just off the projection edges
            vertices[v1].y = -1 + yf * 2;
            vertices[v1].s = x;
            vertices[v1].t = 1.0f - yf;
            for ( int i = 0; i < 4; i++ )
            {
                vertices[v1].color[i] = 1.0f;
            }

            // swap y and x to go along y
            const int v2 = 2 * ( y * 2 + x ) + 1;
            vertices[v2].y = -1 + x * 2;
            vertices[v2].z = -1.001f;	// keep the -1 and 1 just off the projection edges
            vertices[v2].x = -1 + yf * 2;
            vertices[v2].s = x;
            vertices[v2].t = 1.0f - yf;
            for ( int i = 0; i < 4; i++ )
            {
                vertices[v2].color[i] = 1.0f;
            }

            if ( !fullGrid && y != extraLines )
            {	// make a short hash instead of a full line
                vertices[v1].x *= 0.02f;
                vertices[v2].y *= 0.02f;
            }
        }
    }

    unsigned short * indices = new unsigned short [lineCount * 4];

    int index = 0;
    for ( int x = 0; x < lineCount; x++ )
    {
        const int start = x * 4;

        indices[index + 0] = start;
        indices[index + 1] = start + 2;

        indices[index + 2] = start + 1;
        indices[index + 3] = start + 3;

        index += 4;
    }

    VGlGeometry geo;

    glGenVertexArraysOES_( 1, &geo.vertexArrayObject );
    glBindVertexArrayOES_( geo.vertexArrayObject );

    glGenBuffers( 1, &geo.vertexBuffer );
    glBindBuffer( GL_ARRAY_BUFFER, geo.vertexBuffer );
    glBufferData( GL_ARRAY_BUFFER, vertexCount * sizeof( vertex_t ), vertices, GL_STATIC_DRAW );

    glGenBuffers( 1, &geo.indexBuffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, geo.indexBuffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, lineCount * 4 * sizeof( unsigned short ), indices, GL_STATIC_DRAW );

    glEnableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_POSITION );
    glVertexAttribPointer( SHADER_ATTRIBUTE_LOCATION_POSITION, 3, GL_FLOAT, false, sizeof( vertex_t ), (void *)&((vertex_t *)0)->x );

    glEnableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_UV0 );
    glVertexAttribPointer( SHADER_ATTRIBUTE_LOCATION_UV0, 2, GL_FLOAT, false, sizeof( vertex_t ), (void *)&((vertex_t *)0)->s );

    glEnableVertexAttribArray( SHADER_ATTRIBUTE_LOCATION_COLOR );
    glVertexAttribPointer( SHADER_ATTRIBUTE_LOCATION_COLOR, 4, GL_FLOAT, false, sizeof( vertex_t ), (void *)&((vertex_t *)0)->color[0] );

    glBindVertexArrayOES_( 0 );

    return geo;
}

NV_NAMESPACE_END


