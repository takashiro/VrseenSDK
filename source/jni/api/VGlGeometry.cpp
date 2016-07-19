#include "VGlGeometry.h"

#include <math.h>
#include <string.h>
#include <assert.h>

#include "VEglDriver.h"
#include "VAlgorithm.h"
#include "VLensDistortion.h"
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
void VGlGeometry::createGlGeometry( const VertexAttribs & attribs, const VArray< ushort > & indices )
{

    vertexCount = attribs.position.length();
    indexCount = indices.length();

    glGenBuffers( 1, &vertexBuffer );
    glGenBuffers( 1, &indexBuffer );
    VEglDriver::glGenVertexArraysOES( 1, &vertexArrayObject );
    VEglDriver::glBindVertexArrayOES( vertexArrayObject );
    glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );

    VArray< uint8_t > packed;
    PackVertexAttribute( packed, attribs.position,		VERTEX_POSITION,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.normal,		VERTEX_NORMAL,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.tangent,		VERTEX_TANGENT,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.binormal,		VERTEX_BINORMAL,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.color,			VERTEX_COLOR,			GL_FLOAT,	4 );
    PackVertexAttribute( packed, attribs.uvCoordinate0,			VERTEX_UVC0,				GL_FLOAT,	2 );
    PackVertexAttribute( packed, attribs.uvCoordinate1,			VERTEX_UVC1,				GL_FLOAT,	2 );
    PackVertexAttribute( packed, attribs.motionIndices,	JOINT_INDICES,	GL_INT,		4 );
    PackVertexAttribute( packed, attribs.motionWeight,	JOINT_WEIGHTS,	GL_FLOAT,	4 );

    glBufferData( GL_ARRAY_BUFFER, packed.size() * sizeof( packed[0] ), packed.data(), GL_STATIC_DRAW );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices.length() * sizeof( indices[0] ), indices.data(), GL_STATIC_DRAW );

    VEglDriver::glBindVertexArrayOES( 0 );

    glDisableVertexAttribArray( VERTEX_POSITION );
    glDisableVertexAttribArray( VERTEX_NORMAL );
    glDisableVertexAttribArray( VERTEX_TANGENT );
    glDisableVertexAttribArray( VERTEX_BINORMAL );
    glDisableVertexAttribArray( VERTEX_COLOR );
    glDisableVertexAttribArray( VERTEX_UVC0 );
    glDisableVertexAttribArray( VERTEX_UVC1 );
    glDisableVertexAttribArray( JOINT_INDICES );
    glDisableVertexAttribArray( JOINT_WEIGHTS );
}
/*
void VGlGeometry::createGlGeometry( const VertexAttribs & attribs, const VArray< uint > & indices )
{
    VGlOperation glOperation;
    vertexCount = attribs.position.length();
    indexCount = indices.length();

    glGenBuffers( 1, &vertexBuffer );
    glGenBuffers( 1, &indexBuffer );
    glOperation.glGenVertexArraysOES( 1, &vertexArrayObject );
    glOperation.glBindVertexArrayOES( vertexArrayObject );
    glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );

    VArray< uint8_t > packed;
    PackVertexAttribute( packed, attribs.position,		LOCATION_POSITION,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.normal,		LOCATION_NORMAL,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.tangent,		LOCATION_TANGENT,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.binormal,		LOCATION_BINORMAL,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.color,			LOCATION_COLOR,			GL_FLOAT,	4 );
    PackVertexAttribute( packed, attribs.uv0,			LOCATION_UV0,				GL_FLOAT,	2 );
    PackVertexAttribute( packed, attribs.uv1,			LOCATION_UV1,				GL_FLOAT,	2 );
    PackVertexAttribute( packed, attribs.jointIndices,	LOCATION_JOINT_INDICES,	GL_INT,		4 );
    PackVertexAttribute( packed, attribs.jointWeights,	LOCATION_JOINT_WEIGHTS,	GL_FLOAT,	4 );

    glBufferData( GL_ARRAY_BUFFER, packed.size() * sizeof( packed[0] ), packed.data(), GL_STATIC_DRAW );

    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, indexBuffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, indices.length() * sizeof( indices[0] ), indices.data(), GL_STATIC_DRAW );

    glOperation.glBindVertexArrayOES( 0 );

    glDisableVertexAttribArray( LOCATION_POSITION );
    glDisableVertexAttribArray( LOCATION_NORMAL );
    glDisableVertexAttribArray( LOCATION_TANGENT );
    glDisableVertexAttribArray( LOCATION_BINORMAL );
    glDisableVertexAttribArray( LOCATION_COLOR );
    glDisableVertexAttribArray( LOCATION_UV0 );
    glDisableVertexAttribArray( LOCATION_UV1 );
    glDisableVertexAttribArray( LOCATION_JOINT_INDICES );
    glDisableVertexAttribArray( LOCATION_JOINT_WEIGHTS );
}

*/


void VGlGeometry::updateGlGeometry( const VertexAttribs & attribs )
{

    vertexCount = attribs.position.length();

    VEglDriver::glBindVertexArrayOES( vertexArrayObject );

    glBindBuffer( GL_ARRAY_BUFFER, vertexBuffer );

    VArray< uint8_t > packed;

    PackVertexAttribute( packed, attribs.position,		VERTEX_POSITION,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.normal,		VERTEX_NORMAL,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.tangent,		VERTEX_TANGENT,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.binormal,		VERTEX_BINORMAL,			GL_FLOAT,	3 );
    PackVertexAttribute( packed, attribs.color,			VERTEX_COLOR,			GL_FLOAT,	4 );
    PackVertexAttribute( packed, attribs.uvCoordinate0,			VERTEX_UVC0,				GL_FLOAT,	2 );
    PackVertexAttribute( packed, attribs.uvCoordinate1,			VERTEX_UVC1,				GL_FLOAT,	2 );
    PackVertexAttribute( packed, attribs.motionIndices,	JOINT_INDICES,	GL_INT,		4 );
    PackVertexAttribute( packed, attribs.motionWeight,	JOINT_WEIGHTS,	GL_FLOAT,	4 );
    glBufferData( GL_ARRAY_BUFFER, packed.size() * sizeof( packed[0] ), packed.data(), GL_STATIC_DRAW );
}

void VGlGeometry::drawElements() const
{
    VEglDriver::glBindVertexArrayOES( vertexArrayObject );
    glDrawElements( GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT , NULL );
}

void VGlGeometry::destroy()
{

    if(vertexArrayObject != 0)
    VEglDriver::glDeleteVertexArraysOES( 1, &vertexArrayObject );
     if(indexBuffer != 0)
    glDeleteBuffers( 1, &indexBuffer );
      if(vertexBuffer != 0)
    glDeleteBuffers( 1, &vertexBuffer );

    indexBuffer = 0;
    vertexBuffer = 0;
    vertexArrayObject = 0;
    vertexCount = 0;
    indexCount = 0;
}











void VGlGeometry::createScreenQuad( const float xx, const float yy )
 {
     const float posx[] = { -1.001f, -1.0f + xx * 0.25f, -1.0f + xx, 1.0f - xx, 1.0f - xx * 0.25f, 1.001f };
     const float posy[] = { -1.001f, -1.0f + yy * 0.25f, -1.0f + yy, 1.0f - yy, 1.0f - yy * 0.25f, 1.001f };

     const int vertexCount = 6 * 6;

     VertexAttribs attribs;
     attribs.position.resize( vertexCount );
     attribs.uvCoordinate0.resize( vertexCount );
     attribs.color.resize( vertexCount );

     for ( int y = 0; y < 6; y++ )
     {
         for ( int x = 0; x < 6; x++ )
         {
             const int index = y * 6 + x;
             attribs.position[index].x = posx[x];
             attribs.position[index].y = posy[y];
             attribs.position[index].z = 0.0f;
             attribs.uvCoordinate0[index].x = 0.0f;
             attribs.uvCoordinate0[index].y = 0.0f;
             // the outer edges will have 0 color
             const float c = ( y <= 1 || y >= 4 || x <= 1 || x >= 4 ) ? 0.0f : 1.0f;
             attribs.color[index].x = c;
             attribs.color[index].y = c;
             attribs.color[index].z = c;
             attribs.color[index].w = 1.0f;	// solid alpha
         }
     }

     VArray< ushort > indices;
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

     createGlGeometry( attribs, indices );


 }


void VGlGeometry::createPlaneQuadGrid( const int horizontal, const int vertical )
{
    const int vertexCount = ( horizontal + 1 ) * ( vertical + 1 );

    VertexAttribs attribs;
    attribs.position.resize( vertexCount );
    attribs.uvCoordinate0.resize( vertexCount );
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
            attribs.uvCoordinate0[index].x = xf;
            attribs.uvCoordinate0[index].y = 1.0 - yf;
            attribs.color[index].x = 1.0f;
            attribs.color[index].y = 1.0f;
            attribs.color[index].z = 1.0f;
            attribs.color[index].w = 1.0f;
            // fade to transparent on the outside
            if ( x == 0 || x == horizontal || y == 0 || y == vertical )
            {
                attribs.color[index].w = 0.0f;
            }
        }
    }

    VArray< ushort > indices;
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
 createGlGeometry( attribs, indices );
}


void  VGlGeometry::createCylinder( const float radius, const float height,const int horizontal, const int vertical, const float  uScale, const float  vScale )
   {
       const int vertexCount = ( horizontal + 1 ) * ( vertical + 1 );

       VertexAttribs attribs;
       attribs.position.resize( vertexCount );
       attribs.uvCoordinate0.resize( vertexCount );
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
               attribs.uvCoordinate0[index].x = xf * uScale;
               attribs.uvCoordinate0[index].y = ( 1.0f - yf ) * vScale;
               attribs.color[index].x = 1.0f;
               attribs.color[index].y = 1.0f;
               attribs.color[index].z = 1.0f;
               attribs.color[index].w = 1.0f;
               // fade to transparent on the outside
               if ( y == 0 || y == vertical )
               {
                   attribs.color[index].w = 0.0f;
               }
           }
       }

       VArray< ushort > indices;
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

       createGlGeometry( attribs, indices );

   }

void VGlGeometry::createStylePattern( const float xx, const float yy )
{
    const float posx[] = { -1.001f, -1.0f + xx * 0.25f, -1.0f + xx, 1.0f - xx, 1.0f - xx * 0.25f, 1.001f };
    const float posy[] = { -1.001f, -1.0f + yy * 0.25f, -1.0f + yy, 1.0f - yy, 1.0f - yy * 0.25f, 1.001f };

    const int vertexCount = 6 * 6;

    VertexAttribs attribs;
    attribs.position.resize( vertexCount );
    attribs.uvCoordinate0.resize( vertexCount );
    attribs.color.resize( vertexCount );

    for ( int y = 0; y < 6; y++ )
    {
        for ( int x = 0; x < 6; x++ )
        {
            const int index = y * 6 + x;
            attribs.position[index].x = posx[x];
            attribs.position[index].y = posy[y];
            attribs.position[index].z = 0.0f;
            attribs.uvCoordinate0[index].x = 0.0f;
            attribs.uvCoordinate0[index].y = 0.0f;
            // the outer edges will have 0 color
            const float c = ( y <= 1 || y >= 4 || x <= 1 || x >= 4 ) ? 0.0f : 1.0f;
            attribs.color[index].x = c;
            attribs.color[index].y = c;
            attribs.color[index].z = c;
            attribs.color[index].w = 1.0f;	// solid alpha
        }
    }

    VArray< ushort > indices;
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

    createGlGeometry( attribs, indices );

}

void  VGlGeometry::createDome( const float rad, const float uScale, const float vScale)
    {

        const int horizontal = 64;
        const int vertical = 32;
        const float radius = 100.0f;

        const int vertexCount = ( horizontal + 1 ) * ( vertical + 1 );

        VertexAttribs attribs;
        attribs.position.resize( vertexCount );
        attribs.uvCoordinate0.resize( vertexCount );
        attribs.color.resize( vertexCount );

        for ( int y = 0; y <= vertical; y++ )
        {
            const float yf = (float) y / (float) vertical;
            const float lat = M_PI - yf * rad - 0.5f * M_PI;
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

                attribs.uvCoordinate0[index].x = xf * uScale;
                attribs.uvCoordinate0[index].y = ( 1.0f - yf ) * vScale;
                attribs.color[index].x = 1.0f;
                attribs.color[index].y = 1.0f;
                attribs.color[index].z = 1.0f;
                attribs.color[index].w = 1.0f;
            }
        }

        VArray< ushort > indices;
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

       createGlGeometry( attribs, indices );


}


void  VGlGeometry::createSkybox( const float length, const float width, const float height )
{}



void  VGlGeometry::createSphere( const float uScale, const float vScale )

{
    const int poleVertical = 3;
    const int uniformVertical = 64;
    const int horizontal = 128;
    const int vertical = uniformVertical + poleVertical*2;
    const float radius = 100.0f;

    const int vertexCount = ( horizontal + 1 ) * ( vertical + 1 );

    VertexAttribs attribs;
    attribs.position.resize( vertexCount );
    attribs.uvCoordinate0.resize( vertexCount );
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
                attribs.uvCoordinate0[index].x = 0.5f;
            }
            else
            {
                attribs.uvCoordinate0[index].x = xf * uScale;
            }
            attribs.uvCoordinate0[index].y = ( 1.0 - yf ) * vScale;
            attribs.color[index].x = 1.0f;
            attribs.color[index].y = 1.0f;
            attribs.color[index].z = 1.0f;
            attribs.color[index].w = 1.0f;
        }
    }

    VArray< ushort> indices;
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

    createGlGeometry( attribs, indices );


}





void  VGlGeometry::createPartSphere( const float fov )
{

    const int horizontal = 64;
    const int vertical = 64;
    const float radius = 100.0f;

    const int vertexCount = (horizontal + 1) * (vertical + 1);

    VertexAttribs attribs;
    attribs.position.resize( vertexCount );
    attribs.uvCoordinate0.resize( vertexCount );
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
            attribs.uvCoordinate0[index].x = xf - 0.5f;
            attribs.uvCoordinate0[index].y = ( 1.0f - yf ) - 0.5f;

            attribs.color[index].x = 1.0f;
            attribs.color[index].y = 1.0f;
            attribs.color[index].z = 1.0f;
            attribs.color[index].w = 1.0f;
        }
    }

    VArray< ushort > indices;
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

    createGlGeometry( attribs, indices );

}


void  VGlGeometry::createCalibrationGrid( const int lines, const bool full )
{


    const int lineCount = 1 + lines * 2;
    const int vertexCount = lineCount * 2 * 2;

    VertexAttribs attribs;
    attribs.position.resize( vertexCount );
    attribs.uvCoordinate0.resize( vertexCount );
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
            attribs.uvCoordinate0[v1].x = x;
            attribs.uvCoordinate0[v1].y = 1.0f - yf;

            attribs.color[v1].x = 1.0f;
            attribs.color[v1].y = 1.0f;
            attribs.color[v1].z = 1.0f;
            attribs.color[v1].w = 1.0f;

            // swap y and x to go along y
            const int v2 = 2 * ( y * 2 + x ) + 1;
            attribs.position[v2].y = -1 + x * 2;
            attribs.position[v2].z = -1.001f;	// keep the -1 and 1 just off the projection edges
            attribs.position[v2].x = -1 + yf * 2;
            attribs.uvCoordinate0[v2].x = x;
            attribs.uvCoordinate0[v2].y = 1.0f - yf;

            attribs.color[v2].x = 1.0f;
            attribs.color[v2].y = 1.0f;
            attribs.color[v2].z = 1.0f;
            attribs.color[v2].w = 1.0f;

            if ( !full && y != lines )
            {	// make a short hash instead of a full line
                attribs.position[v1].x *= 0.02f;
                attribs.position[v2].y *= 0.02f;
            }
        }
    }

    VArray< ushort > indices;
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
    createGlGeometry( attribs, indices );
}



void  VGlGeometry::createUnitCubeGrid()

{
    VertexAttribs attribs;
    attribs.position.resize( 8 );

    for ( int i = 0; i < 8; i++) {
        attribs.position[i].x = i & 1;
        attribs.position[i].y = ( i & 2 ) >> 1;
        attribs.position[i].z = ( i & 4 ) >> 2;
    }

    const uint staticIndices[24] = { 0,1, 1,3, 3,2, 2,0, 4,5, 5,7, 7,6, 6,4, 0,4, 1,5, 3,7, 2,6 };

    VArray< ushort > indices;
    indices.resize( 24 );
    memcpy( &indices[0], staticIndices, 24 * sizeof( indices[0] ) );

    createGlGeometry( attribs, indices );

}



void  VGlGeometry::createQuad()
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

    VEglDriver::glGenVertexArraysOES( 1, &geometry.vertexArrayObject );
    VEglDriver::glBindVertexArrayOES( geometry.vertexArrayObject );

    glGenBuffers( 1, &geometry.vertexBuffer );
    glBindBuffer( GL_ARRAY_BUFFER, geometry.vertexBuffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), &vertices, GL_STATIC_DRAW );

    glGenBuffers( 1, &geometry.indexBuffer );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, geometry.indexBuffer );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( indices ), indices, GL_STATIC_DRAW );


    glEnableVertexAttribArray( VERTEX_POSITION );
    glVertexAttribPointer( VERTEX_POSITION, 3, GL_FLOAT, false,
                           sizeof( vertices.positions[0] ), (const GLvoid *)offsetof( vertices_t, positions ) );

    glEnableVertexAttribArray( VERTEX_UVC0 );
    glVertexAttribPointer( VERTEX_UVC0, 2, GL_FLOAT, false,
                           sizeof( vertices.uvs[0] ), (const GLvoid *)offsetof( vertices_t, uvs ) );

    glEnableVertexAttribArray( VERTEX_COLOR );
    glVertexAttribPointer( VERTEX_COLOR, 4, GL_FLOAT, false,

                           sizeof( vertices.colors[0] ), (const GLvoid *)offsetof( vertices_t, colors ) );

    VEglDriver::glBindVertexArrayOES( 0 );



}




NV_NAMESPACE_END
