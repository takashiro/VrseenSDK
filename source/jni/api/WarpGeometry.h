#pragma once

#include "vglobal.h"
#include "Android/GlUtils.h"

NV_NAMESPACE_BEGIN

struct WarpGeometry
{
				WarpGeometry() :
					vertexBuffer( 0 ),
					indexBuffer( 0 ),
					vertexArrayObject( 0 ),
					vertexCount( 0 ),
					indexCount( 0 ) {}

	GLuint		vertexBuffer;
	GLuint		indexBuffer;
	GLuint	 	vertexArrayObject;
	int			vertexCount;
	int 		indexCount;
};

void CreateQuadWarpGeometry( WarpGeometry * geometry );
void DestroyWarpGeometry( WarpGeometry * geometry );

NV_NAMESPACE_END


