/************************************************************************************

Filename    :   WarpGeometry.cpp
Content     :   Geometry used by the time warp.
Created     :   March 3, 2015
Authors     :   J.M.P. van Waveren

Copyright   :   Copyright 2015 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "WarpGeometry.h"
#include "VGlShader.h"

namespace NervGear {

void CreateQuadWarpGeometry( WarpGeometry * geometry )
{
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

	geometry->vertexCount = 4;
	geometry->indexCount = 6;

	glGenVertexArraysOES_( 1, &geometry->vertexArrayObject );
	glBindVertexArrayOES_( geometry->vertexArrayObject );

	glGenBuffers( 1, &geometry->vertexBuffer );
	glBindBuffer( GL_ARRAY_BUFFER, geometry->vertexBuffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof( vertices ), &vertices, GL_STATIC_DRAW );

	glGenBuffers( 1, &geometry->indexBuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, geometry->indexBuffer );
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
}

void DestroyWarpGeometry( WarpGeometry * geometry )
{
	glDeleteVertexArraysOES_( 1, &geometry->vertexArrayObject );
	glDeleteBuffers( 1, &geometry->indexBuffer );
	glDeleteBuffers( 1, &geometry->vertexBuffer );

	geometry->vertexArrayObject = 0;
	geometry->indexBuffer = 0;
	geometry->vertexBuffer = 0;
	geometry->vertexCount = 0;
	geometry->indexCount = 0;
}

}	// namespace NervGear
