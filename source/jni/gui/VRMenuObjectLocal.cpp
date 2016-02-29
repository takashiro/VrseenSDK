/************************************************************************************

Filename    :   VRMenuObjectLocal.cpp
Content     :   Menuing system for VR apps.
Created     :   May 23, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#include "VRMenuObjectLocal.h"

#include "GlTexture.h"
#include "App.h"			// for loading images from the assets folder
#include "ModelTrace.h"
#include "BitmapFont.h"
#include "VRMenuMgr.h"
#include "VRMenuComponent.h"
#include "ui_default.h"	// embedded default UI texture (loaded as a placeholder when something doesn't load)
#include "PackageFiles.h"


namespace NervGear {

float const	VRMenuObject::TEXELS_PER_METER		= 500.0f;
float const	VRMenuObject::DEFAULT_TEXEL_SCALE	= 1.0f / TEXELS_PER_METER;

const float VRMenuSurface::Z_BOUNDS = 0.05f;

// too bad this doesn't work
//#pragma GCC diagnostic ignored "-Werror"

//======================================================================================
// VRMenuSurfaceTexture

//==============================
// VRMenuSurfaceTexture::VRMenuSurfaceTexture::
VRMenuSurfaceTexture::	VRMenuSurfaceTexture() :
		m_handle( 0 ),
		m_width( 0 ),
		m_height( 0 ),
		m_type( SURFACE_TEXTURE_MAX ),
        m_ownsTexture( false )
{
}

//==============================
// VRMenuSurfaceTexture::LoadTexture
bool VRMenuSurfaceTexture::loadTexture( eSurfaceTextureType const type, char const * imageName, bool const allowDefault )
{
    free();

	OVR_ASSERT( type >= 0 && type < SURFACE_TEXTURE_MAX );

	m_type = type;

	if ( imageName != NULL && imageName[0] != '\0' )
	{
		void * 	buffer;
		int		bufferLength;
		ovr_ReadFileFromApplicationPackage( imageName, bufferLength, buffer );
		if ( !buffer )
		{
			m_handle = 0;
		}
		else
		{
			m_handle = LoadTextureFromBuffer( imageName, MemBuffer( buffer, bufferLength ),
					TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), m_width, m_height );
            ::free( buffer );
		}
	}

	if ( m_handle == 0 && allowDefault )
	{
		m_handle = LoadTextureFromBuffer( imageName, MemBuffer( uiDefaultTgaData, uiDefaultTgaSize ), 
							TextureFlags_t(), m_width, m_height );
		WARN( "VRMenuSurfaceTexture::CreateFromImage: failed to load image '%s' - default loaded instead!", imageName );
	}
    m_ownsTexture = true;
	return m_handle != 0;
}

//==============================
// VRMenuSurfaceTexture::LoadTexture
void VRMenuSurfaceTexture::loadTexture( eSurfaceTextureType const type, const GLuint texId, const int width, const int height )
{
	free();

	OVR_ASSERT( type >= 0 && type < SURFACE_TEXTURE_MAX );

	m_type = type;
    m_ownsTexture = false;
	m_handle = texId;
	m_width = width;
	m_height = height;
}

//==============================
// VRMenuSurfaceTexture::Free
void VRMenuSurfaceTexture::free()
{
	if ( m_handle != 0 )
	{
        if ( m_ownsTexture )
        {
		    glDeleteTextures( 1, &m_handle );
        }
		m_handle = 0;
		m_width = 0;
		m_height = 0;
		m_type = SURFACE_TEXTURE_MAX;
        m_ownsTexture = false;
	}
}

//======================================================================================
// VRMenuSurfaceTris


//======================================================================================
// VRMenuSurface

#if 0
static void PrintBounds( const char * name, char const * prefix, Bounds3f const & bounds )
{
	LOG( "'%s' %s: min( %.2f, %.2f, %.2f ) - max( %.2f, %.2f, %.2f )", 
		name, prefix,
		bounds.GetMins().x, bounds.GetMins().y, bounds.GetMins().z,
		bounds.GetMaxs().x, bounds.GetMaxs().y, bounds.GetMaxs().z );
}
#endif

//==============================
// VRMenuSurface::VRMenuSurface
VRMenuSurface::VRMenuSurface() :
	m_color( 1.0f ),
	m_border( 0.0f, 0.0f, 0.0f, 0.0f ),
	m_contents( CONTENT_SOLID ),
	m_visible( true ),
	m_programType( PROGRAM_MAX )
{
}

//==============================
// VRMenuSurface::~VRMenuSurface
VRMenuSurface::~VRMenuSurface()
{
	free();
}

//==============================
// VRMenuSurface::CreateImageGeometry
//
// This creates a quad for mapping the texture.
void VRMenuSurface::createImageGeometry( int const textureWidth, int const textureHeight, const Vector2f &dims, const Vector4f &border, ContentFlags_t const contents )
{
	//OVR_ASSERT( Geo.vertexBuffer == 0 && Geo.indexBuffer == 0 && Geo.vertexArrayObject == 0 );

	int vertsX = 0;
	int vertsY = 0;
	float vertUVX[ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float vertUVY[ 4 ] = { 1.0f, 0.0f, 0.0f, 0.0f };
	float vertPosX[ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float vertPosY[ 4 ] = { 0.0f, 0.0f, 0.0f, 0.0f };

	// x components
	vertPosX[ vertsX ] = 0.0f;
	vertUVX[ vertsX++ ] = 0.0f;

	if ( border[ BORDER_LEFT ] > 0.0f )
	{
		vertPosX[ vertsX ] 	= border[ BORDER_LEFT ] / dims.x;
		vertUVX[ vertsX++ ] = border[ BORDER_LEFT ] / (float)textureWidth;
	}

	if ( border[ BORDER_RIGHT ] > 0.0f )
	{
		vertPosX[ vertsX ]  = 1.0f - border[ BORDER_RIGHT ] / dims.x;
		vertUVX[ vertsX++ ] = 1.0f - border[ BORDER_RIGHT ] / (float)textureWidth;
	}

	vertPosX[ vertsX ] = 1.0f;
	vertUVX[ vertsX++ ] = 1.0f;

	// y components
	vertPosY[ vertsY ] = 0.0f;
	vertUVY[ vertsY++ ] = 0.0f;

	if ( border[ BORDER_BOTTOM ] > 0.0f )
	{
		vertPosY[ vertsY ] 	= border[ BORDER_BOTTOM ] / dims.y;
		vertUVY[ vertsY++ ] = border[ BORDER_BOTTOM ] / (float)textureHeight;
	}

	if ( border[ BORDER_TOP ] > 0.0f )
	{
		vertPosY[ vertsY ]  = 1.0f - border[ BORDER_TOP ] / dims.y;
		vertUVY[ vertsY++ ] = 1.0f - border[ BORDER_TOP ] / (float)textureHeight;
	}

	vertPosY[ vertsY ] = 1.0f;
	vertUVY[ vertsY++ ] = 1.0f;

	// create the vertices
	const int vertexCount = vertsX * vertsY;
	const int horizontal = vertsX - 1;
	const int vertical = vertsY - 1;

	VertexAttribs attribs;
	attribs.position.resize( vertexCount );
	attribs.uv0.resize( vertexCount );
	attribs.uv1.resize( vertexCount );
	attribs.color.resize( vertexCount );

	Vector4f color( 1.0f, 1.0f, 1.0f, 1.0f );

	for ( int y = 0; y <= vertical; y++ )
	{
		const float yPos = ( -1 + vertPosY[ y ] * 2 ) * ( dims.y * VRMenuObject::DEFAULT_TEXEL_SCALE * 0.5f );
		const float uvY = 1.0f - vertUVY[ y ];

		for ( int x = 0; x <= horizontal; x++ )
		{
			const int index = y * ( horizontal + 1 ) + x;
			attribs.position[index].x = ( -1 + vertPosX[ x ] * 2 ) * ( dims.x * VRMenuObject::DEFAULT_TEXEL_SCALE * 0.5f );
			attribs.position[index].z = 0;
			attribs.position[index].y = yPos;
			attribs.uv0[index].x = vertUVX[ x ];
			attribs.uv0[index].y = uvY;
			attribs.uv1[index] = attribs.uv0[index];
			attribs.color[index] = color;
		}
	}

	Array< TriangleIndex > indices;
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

    m_tris.init( attribs.position, indices, contents );

	if ( m_geo.vertexBuffer == 0 && m_geo.indexBuffer == 0 && m_geo.vertexArrayObject == 0 )
	{
		m_geo.Create( attribs, indices );
	}
	else
	{
		m_geo.Update( attribs );
	}
}

//==============================
// VRMenuSurface::Render
void VRMenuSurface::render( OvrVRMenuMgr const & menuMgr, Matrix4f const & mvp, SubmittedMenuObject const & sub ) const
{
	if ( m_geo.vertexCount == 0 )
	{
		return;	// surface wasn't initialized with any geometry -- this can happen if diffuse and additive are both invalid
	}

    //LOG( "Render Surface '%s', skip = '%s'", SurfaceName.toCString(), skipAdditivePass ? "true" : "false" );

	GL_CheckErrors( "VRMenuSurface::Render - pre" );

	GlProgram const * program = NULL;

	glEnable( GL_BLEND );

	eGUIProgramType pt = m_programType;

	if ( sub.skipAdditivePass )
	{
		if ( pt == PROGRAM_DIFFUSE_PLUS_ADDITIVE || pt == PROGRAM_DIFFUSE_COMPOSITE )
		{
			pt = PROGRAM_DIFFUSE_ONLY;	// this is used to not render the gazeover hilights
		}
	}

    program = menuMgr.getGUIGlProgram( pt );

	switch( pt )
	{
		case PROGRAM_DIFFUSE_ONLY:
		{
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			int diffuseIndex = indexForTextureType( SURFACE_TEXTURE_DIFFUSE, 1 );
			DROID_ASSERT( diffuseIndex >= 0, "VrMenu" );	// surface setup should have detected this!
			// bind the texture
			glActiveTexture( GL_TEXTURE0 );
			glBindTexture( GL_TEXTURE_2D, m_textures[diffuseIndex].handle() );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			break;
		}
		case PROGRAM_DIFFUSE_COMPOSITE:
		{
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			int diffuseIndex = indexForTextureType( SURFACE_TEXTURE_DIFFUSE, 1 );
			DROID_ASSERT( diffuseIndex >= 0, "VrMenu" );	// surface setup should have detected this!
			int diffuse2Index = indexForTextureType( SURFACE_TEXTURE_DIFFUSE, 2 );
			DROID_ASSERT( diffuse2Index >= 0, "VrMenu" );	// surface setup should have detected this!
			// bind both textures
			glActiveTexture( GL_TEXTURE0 );
			glBindTexture( GL_TEXTURE_2D, m_textures[diffuseIndex].handle() );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			glActiveTexture( GL_TEXTURE1 );
			glBindTexture( GL_TEXTURE_2D, m_textures[diffuse2Index].handle() );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			break;
		}
		case PROGRAM_ADDITIVE_ONLY:
		{
			glBlendFunc( GL_SRC_ALPHA, GL_ONE );
			int additiveIndex = indexForTextureType( SURFACE_TEXTURE_ADDITIVE, 1 );
			DROID_ASSERT( additiveIndex >= 0, "VrMenu" );	// surface setup should have detected this!
			// bind the texture
			glActiveTexture( GL_TEXTURE0 );
			glBindTexture( GL_TEXTURE_2D, m_textures[additiveIndex].handle() );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			break;
		}
		case PROGRAM_DIFFUSE_PLUS_ADDITIVE:		// has a diffuse and an additive
		{
			//glBlendFunc( GL_ONE, GL_ONE );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			int diffuseIndex = indexForTextureType( SURFACE_TEXTURE_DIFFUSE, 1 );
			DROID_ASSERT( diffuseIndex >= 0, "VrMenu" );	// surface setup should have detected this!
			int additiveIndex = indexForTextureType( SURFACE_TEXTURE_ADDITIVE, 1 );
			DROID_ASSERT( additiveIndex >= 0, "VrMenu" );	// surface setup should have detected this!
			// bind both textures
			glActiveTexture( GL_TEXTURE0 );
			glBindTexture( GL_TEXTURE_2D, m_textures[diffuseIndex].handle() );
			glActiveTexture( GL_TEXTURE1 );
			glBindTexture( GL_TEXTURE_2D, m_textures[additiveIndex].handle() );
			break;
		}
		case PROGRAM_DIFFUSE_COLOR_RAMP:			// has a diffuse and color ramp, and color ramp target is the diffuse
		{
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			int diffuseIndex = indexForTextureType( SURFACE_TEXTURE_DIFFUSE, 1 );
			DROID_ASSERT( diffuseIndex >= 0, "VrMenu" );	// surface setup should have detected this!
			int rampIndex = indexForTextureType( SURFACE_TEXTURE_COLOR_RAMP, 1 );
			DROID_ASSERT( rampIndex >= 0, "VrMenu" );	// surface setup should have detected this!
			// bind both textures
			glActiveTexture( GL_TEXTURE0 );
			glBindTexture( GL_TEXTURE_2D, m_textures[diffuseIndex].handle() );
			glActiveTexture( GL_TEXTURE1 );
			glBindTexture( GL_TEXTURE_2D, m_textures[rampIndex].handle() );
			// do not do any filtering on the "palette" texture
			if ( EXT_texture_filter_anisotropic )
			{
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f );
			}
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			break;
		}
		case PROGRAM_DIFFUSE_COLOR_RAMP_TARGET:	// has diffuse, color ramp, and a separate color ramp target
		{
			//LOG( "Surface '%s' - PROGRAM_COLOR_RAMP_TARGET", SurfaceName );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			int diffuseIndex = indexForTextureType( SURFACE_TEXTURE_DIFFUSE, 1 );
			DROID_ASSERT( diffuseIndex >= 0, "VrMenu" );	// surface setup should have detected this!
			int rampIndex = indexForTextureType( SURFACE_TEXTURE_COLOR_RAMP, 1 );
			DROID_ASSERT( rampIndex >= 0, "VrMenu" );	// surface setup should have detected this!
			int targetIndex = indexForTextureType( SURFACE_TEXTURE_COLOR_RAMP_TARGET, 1 );
			DROID_ASSERT( targetIndex >= 0, "VrMenu" );	// surface setup should have detected this!
			// bind both textures
			glActiveTexture( GL_TEXTURE0 );
			glBindTexture( GL_TEXTURE_2D, m_textures[diffuseIndex].handle() );
			glActiveTexture( GL_TEXTURE1 );
			glBindTexture( GL_TEXTURE_2D, m_textures[targetIndex].handle() );
			glActiveTexture( GL_TEXTURE2 );
			glBindTexture( GL_TEXTURE_2D, m_textures[rampIndex].handle() );
			// do not do any filtering on the "palette" texture
			if ( EXT_texture_filter_anisotropic )
			{
				glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f );
			}
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			break;
		}
		case PROGRAM_MAX:
		{
			WARN( "Unsupported texture map combination." );
			return;
		}
		default:
		{
			DROID_ASSERT( !"Unhandled ProgramType", "Uhandled ProgramType" );
			return;
		}
	}

	DROID_ASSERT( program != NULL, "VrMenu" );

	glUseProgram( program->program );

	glUniformMatrix4fv( program->uMvp, 1, GL_FALSE, mvp.M[0] );

	glUniform4fv( program->uColor, 1, &sub.color.x );
	glUniform3fv( program->uFadeDirection, 1, &sub.fadeDirection.x );
	glUniform2fv( program->uColorTableOffset, 1, &sub.colorTableOffset.x );

	// render
	glBindVertexArrayOES_( m_geo.vertexArrayObject );
	glDrawElements( GL_TRIANGLES, m_geo.indexCount, GL_UNSIGNED_SHORT, NULL );
	glBindVertexArrayOES_( 0 );

	GL_CheckErrors( "VRMenuSurface::Render - post" );
}

//==============================
// VRMenuSurface::CreateFromSurfaceParms
void VRMenuSurface::createFromSurfaceParms( VRMenuSurfaceParms const & parms )
{
	free();

	m_furfaceName = parms.SurfaceName;

	// verify the input parms have a valid image name and texture type
	bool isValid = false;
	for ( int i = 0; i < VRMENUSURFACE_IMAGE_MAX; ++i )	
	{
		if ( !parms.ImageNames[i].isEmpty() && 
            ( parms.TextureTypes[i] >= 0 && parms.TextureTypes[i] < SURFACE_TEXTURE_MAX ) )
	    {
    		isValid = true;
            m_textures[i].loadTexture( parms.TextureTypes[i], parms.ImageNames[i], true );
		}
		else if ( ( parms.ImageTexId[i] != 0 ) &&
            ( parms.TextureTypes[i] >= 0 && parms.TextureTypes[i] < SURFACE_TEXTURE_MAX ) )
	    {
    		isValid = true;
            m_textures[i].loadTexture( parms.TextureTypes[i], parms.ImageTexId[i], parms.ImageWidth[i], parms.ImageHeight[i] );
		}
	}
	if ( !isValid )
	{
		//LOG( "VRMenuSurfaceParms '%s' - no valid images - skipping", parms.SurfaceName.toCString() );
		return;
	}

	// make sure we have a surface for sizing the geometry
	int surfaceIdx = -1;
	for ( int i = 0; i < VRMENUSURFACE_IMAGE_MAX; ++i )
	{
		if ( m_textures[i].handle() != 0 )
		{
			surfaceIdx = i;
			break;
		}
	}
	if ( surfaceIdx < 0 )
	{
		//LOG( "VRMenuSurface::CreateFromImageParms - no suitable image for surface creation" );
		return;
	}

	m_textureDims.x = m_textures[surfaceIdx].width();
	m_textureDims.y = m_textures[surfaceIdx].height();

	if ( ( parms.Dims.x == 0 ) || ( parms.Dims.y == 0 ) )
	{
		m_dims = m_textureDims;
	}
	else
	{
		m_dims = parms.Dims;
	}

	m_border = parms.Border;
	m_anchors = parms.Anchors;
	m_contents = parms.Contents;

	createImageGeometry( m_textureDims.x, m_textureDims.y, m_dims, m_border, m_contents );

	// now, based on the combination of surfaces, determine the render prog to use
	if ( hasTexturesOfType( SURFACE_TEXTURE_DIFFUSE, 1 ) &&
		hasTexturesOfType( SURFACE_TEXTURE_COLOR_RAMP, 1 ) &&
		hasTexturesOfType( SURFACE_TEXTURE_COLOR_RAMP_TARGET, 1 ) )
	{
		m_programType = PROGRAM_DIFFUSE_COLOR_RAMP_TARGET;
	}
	else if ( hasTexturesOfType( SURFACE_TEXTURE_DIFFUSE, 1 ) &&
		hasTexturesOfType( SURFACE_TEXTURE_MAX, 2 ) )
	{
		m_programType = PROGRAM_DIFFUSE_ONLY;
	}
	else if ( hasTexturesOfType( SURFACE_TEXTURE_ADDITIVE, 1 ) &&
		hasTexturesOfType( SURFACE_TEXTURE_MAX, 2 ) )
	{
		m_programType = PROGRAM_ADDITIVE_ONLY;
	}
	else if ( hasTexturesOfType( SURFACE_TEXTURE_DIFFUSE, 2 ) &&
		hasTexturesOfType( SURFACE_TEXTURE_MAX, 1 ) )
	{
		m_programType = PROGRAM_DIFFUSE_COMPOSITE;
	}
	else if ( hasTexturesOfType( SURFACE_TEXTURE_DIFFUSE, 1 ) &&
		hasTexturesOfType( SURFACE_TEXTURE_COLOR_RAMP, 1 ) &&
		hasTexturesOfType( SURFACE_TEXTURE_MAX, 1 ) )
	{
		m_programType = PROGRAM_DIFFUSE_COLOR_RAMP;
	}
	else if ( hasTexturesOfType( SURFACE_TEXTURE_DIFFUSE, 1 ) &&
		hasTexturesOfType( SURFACE_TEXTURE_ADDITIVE, 1 ) &&
		hasTexturesOfType( SURFACE_TEXTURE_MAX, 1 ) )
	{
		m_programType = PROGRAM_DIFFUSE_PLUS_ADDITIVE;
	}
	else
	{
		WARN( "Invalid material combination -- either add a shader to support it or fix it." );
		m_programType = PROGRAM_MAX;
	}
}

//==============================
// VRMenuSurface::RegenerateSurfaceGeometry
void VRMenuSurface::regenerateSurfaceGeometry()
{
	createImageGeometry( m_textureDims.x, m_textureDims.y, m_dims, m_border, m_contents );
}

//==============================
// VRMenuSurface::
bool VRMenuSurface::hasTexturesOfType( eSurfaceTextureType const t, int const requiredCount ) const
{
	int count = 0;
	for ( int i = 0; i < VRMENUSURFACE_IMAGE_MAX; ++i )
	{
		if ( m_textures[i].type() == t ) {
			count++;
		}
	}
	return ( requiredCount == count );	// must be the exact same number
}

int VRMenuSurface::indexForTextureType( eSurfaceTextureType const t, int const occurenceCount ) const
{
	int count = 0;
	for ( int i = 0; i < VRMENUSURFACE_IMAGE_MAX; ++i )
	{
		if ( m_textures[i].type() == t ) {
			count++;
			if ( count == occurenceCount )
			{
				return i;
			}
		}
	}
	return -1;
}

//==============================
// VRMenuSurface::Free
void VRMenuSurface::free()
{
	for ( int i = 0; i < VRMENUSURFACE_IMAGE_MAX; ++i )
	{
		m_textures[i].free();
	}
}

//==============================
// VRMenuSurface::IntersectRay
bool VRMenuSurface::intersectRay( Vector3f const & start, Vector3f const & dir, Posef const & pose,
                                  Vector3f const & scale, ContentFlags_t const testContents,
								  OvrCollisionResult & result ) const
{
    return m_tris.intersectRay( start, dir, pose, scale, testContents, result );
}

//==============================
// VRMenuSurface::IntersectRay
bool VRMenuSurface::intersectRay( Vector3f const & localStart, Vector3f const & localDir, 
                                  Vector3f const & scale, ContentFlags_t const testContents,
								  OvrCollisionResult & result ) const
{
    return m_tris.intersectRay( localStart, localDir, scale, testContents, result );
}

//==============================
// VRMenuSurface::LoadTexture
void VRMenuSurface::loadTexture( int const textureIndex, eSurfaceTextureType const type, 
        const GLuint texId, const int width, const int height )
{
    if ( textureIndex < 0 || textureIndex >= VRMENUSURFACE_IMAGE_MAX )
    {
        DROID_ASSERT( textureIndex >= 0 && textureIndex < VRMENUSURFACE_IMAGE_MAX, "VrMenu" );
        return;
    }
    m_textures[textureIndex].loadTexture( type, texId, width, height );
}

//==============================
// VRMenuSurface::GetAnchorOffsets
Vector2f VRMenuSurface::anchorOffsets() const { 
	return Vector2f( ( ( 1.0f - m_anchors.x ) - 0.5f ) * m_dims.x * VRMenuObject::DEFAULT_TEXEL_SCALE, // inverted so that 0.0 is left-aligned
					 ( m_anchors.y - 0.5f ) * m_dims.y * VRMenuObject::DEFAULT_TEXEL_SCALE ); 
}

void VRMenuSurface::setOwnership( int const index, bool const isOwner )
{
	m_textures[ index ].setOwnership( isOwner );
}

//======================================================================================
// VRMenuObjectLocal

//==================================
// VRMenuObjectLocal::VRMenuObjectLocal
VRMenuObjectLocal::VRMenuObjectLocal( VRMenuObjectParms const & parms, 
		menuHandle_t const handle ) :
	m_type( parms.Type ),
	m_handle( handle ),
	m_id( parms.Id ),
	m_flags( parms.Flags ),
	m_localPose( parms.LocalPose ),
	m_localScale( parms.LocalScale ),
    m_hilightPose( Quatf(), Vector3f( 0.0f, 0.0f, 0.0f ) ),
    m_hilightScale( 1.0f ),
    m_textLocalPose( parms.TextLocalPose ),
    m_textLocalScale( parms.TextLocalScale ),
	m_text( parms.Text ),
	m_collisionPrimitive( NULL ),
	m_contents( CONTENT_SOLID ),
	m_color( parms.Color ),
    m_textColor( parms.TextColor ),
	m_colorTableOffset( 0.0f ),
	m_fontParms( parms.FontParms ),
	m_hilighted( false ),
	m_selected( false ),
    m_textDirty( true ),
	m_minsBoundsExpand( 0.0f ),
	m_maxsBoundsExpand( 0.0f ),
	m_textMetrics(),
	m_wrapWidth( 0.0f )
{
	m_cullBounds.Clear();
}

//==================================
// VRMenuObjectLocal::~VRMenuObjectLocal
VRMenuObjectLocal::~VRMenuObjectLocal()
{
	if ( m_collisionPrimitive != NULL )
	{
		delete m_collisionPrimitive;
		m_collisionPrimitive = NULL;
	}

    // all components must be dynamically allocated
    for ( int i = 0; i < m_components.sizeInt(); ++i )
    {
        delete m_components[i];
        m_components[i] = NULL;
    }
    m_components.clear();
	m_handle.Release();
	m_parentHandle.Release();
	m_type = VRMENU_MAX;
}

//==================================
// VRMenuObjectLocal::Init
void VRMenuObjectLocal::init( VRMenuObjectParms const & parms )
{
	for ( int i = 0; i < parms.SurfaceParms.sizeInt(); ++i )
	{
		int idx = m_surfaces.allocBack();
        m_surfaces[idx].createFromSurfaceParms( parms.SurfaceParms[i] );
	}

	// bounds are nothing submitted for rendering
	m_cullBounds = Bounds3f( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f );
	m_fontParms = parms.FontParms;
    for ( int i = 0; i < parms.Components.sizeInt(); ++i )
    {
	    addComponent( parms.Components[i] );
    }
}

//==================================
// VRMenuObjectLocal::FreeChildren
void VRMenuObjectLocal::freeChildren( OvrVRMenuMgr & menuMgr )
{
	for ( int i = 0; i < m_children.sizeInt(); ++i ) 
	{
		menuMgr.freeObject( m_children[i] );
	}
	m_children.resize( 0 );
    // NOTE! bounds will be incorrect now until submitted for rendering
}

//==================================
// VRMenuObjectLocal::IsDescendant
bool VRMenuObjectLocal::isDescendant( OvrVRMenuMgr & menuMgr, menuHandle_t const handle ) const
{
	for ( int i = 0; i < m_children.sizeInt(); ++i )
	{
		if ( m_children[i] == handle )
		{
			return true;
		}
	}

	for ( int i = 0; i < m_children.sizeInt(); ++i )
	{
		VRMenuObject * child = menuMgr.toObject( m_children[i] );
		if ( child != NULL )
		{
			bool r = child->isDescendant( menuMgr, handle );
			if ( r )
			{
				return true;
			}
		}
	}

	return false;
}

//==============================
// VRMenuObjectLocal::AddChild
void VRMenuObjectLocal::addChild( OvrVRMenuMgr & menuMgr, menuHandle_t const handle )
{
	m_children.pushBack( handle );

	VRMenuObject * child = menuMgr.toObject( handle );
	if ( child != NULL )
	{
		child->setParentHandle( this->m_handle );
	}
    // NOTE: bounds will be incorrect until submitted for rendering
}

//==============================
// VRMenuObjectLocal::RemoveChild
void VRMenuObjectLocal::removeChild( OvrVRMenuMgr & menuMgr, menuHandle_t const handle )
{
	for ( int i = 0; i < m_children.sizeInt(); ++i )
	{
		if ( m_children[i] == handle )
		{
			m_children.removeAtUnordered( i );
			return;
		}
	}
}

//==============================
// VRMenuObjectLocal::FreeChild
void VRMenuObjectLocal::freeChild( OvrVRMenuMgr & menuMgr, menuHandle_t const handle )
{
	for ( int i = 0; i < m_children.sizeInt(); ++i) 
	{
		menuHandle_t childHandle = m_children[i];
		if ( childHandle == handle )
		{
			m_children.removeAtUnordered( i );
			menuMgr.freeObject( childHandle );
			return;
		}
	}
}

//==============================
// VRMenuObjectLocal::Frame
void VRMenuObjectLocal::frame( OvrVRMenuMgr & menuMgr, Matrix4f const & viewMatrix )
{
	for ( int i = 0; i < m_children.sizeInt(); ++i )
	{
		VRMenuObject * child = menuMgr.toObject( m_children[i] );
		if ( child != NULL )
		{
			child->frame( menuMgr, viewMatrix );
		}
	}
}

//==============================
// IntersectRayBounds
// Reports true if the hit was at or beyond start in the ray direction, 
// or if the start point was inside of the bounds.
bool VRMenuObjectLocal::intersectRayBounds( Vector3f const & start, Vector3f const & dir, 
        Vector3f const & mins, Vector3f const & maxs, ContentFlags_t const testContents, float & t0, float & t1 ) const
{
	if ( !( testContents & contents() ) )
	{
		return false;
	}

    if ( Bounds3f( mins, maxs ).Contains( start, 0.1f ) )
    {
        return true;
    }
	Intersect_RayBounds( start, dir, mins, maxs, t0, t1 );
	return t0 >= 0.0f && t1 >= 0.0f && t1 >= t0;
}

//==============================
// VRMenuObjectLocal::IntersectRay
bool VRMenuObjectLocal::intersectRay( Vector3f const & localStart, Vector3f const & localDir, Vector3f const & parentScale, Bounds3f const & bounds,
        float & bounds_t0, float & bounds_t1, ContentFlags_t const testContents, OvrCollisionResult & result ) const
{
	result = OvrCollisionResult();

    // bounds are already computed with scale applied
    if ( !intersectRayBounds( localStart, localDir, bounds.GetMins(), bounds.GetMaxs(), testContents, bounds_t0, bounds_t1 ) )
    {
        bounds_t0 = FLT_MAX;
        bounds_t1 = FLT_MAX;
        return false;
    }

    // if marked to check only the bounds, then we've hit the object
	if ( m_flags & VRMENUOBJECT_HIT_ONLY_BOUNDS )
	{
		result.t = bounds_t0;
		return true;
	}

    // vertices have not had the scale applied yet
    Vector3f const scale = localScale() * parentScale;

	// test vs. collision primitive
	if ( m_collisionPrimitive != NULL )
	{
		m_collisionPrimitive->intersectRay( localStart, localDir, scale, testContents, result );
	}
	
	// test vs. surfaces
	if (  type() != VRMENU_CONTAINER )
	{
		int numSurfaces = 0;
		for ( int i = 0; i < m_surfaces.sizeInt(); ++i )
		{
            if ( m_surfaces[i].isRenderable() )
			{
				numSurfaces++;

				OvrCollisionResult localResult;
                if ( m_surfaces[i].intersectRay( localStart, localDir, scale, testContents, localResult ) )
				{
					if ( localResult.t < result.t )
					{
						result = localResult;
					}
				}
			}
		}
	}

    return result.triIndex >= 0;
}

//==============================
// VRMenuObjectLocal::HitTest_r
bool VRMenuObjectLocal:: hitTest_r( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font, 
		Posef const & parentPose, Vector3f const & parentScale, Vector3f const & rayStart, Vector3f const & rayDir,
		ContentFlags_t const testContents, HitTestResult & result ) const
{
	if ( m_flags & VRMENUOBJECT_DONT_RENDER )
	{
		return false;
	}

	if ( m_flags & VRMENUOBJECT_DONT_HIT_ALL )
	{
		return false;
	}

	// transform ray into local space
    Vector3f const & localScale = this->localScale();
	Vector3f scale = parentScale.EntrywiseMultiply( localScale );
	Posef modelPose;
	modelPose.Position = parentPose.Position + ( parentPose.Orientation * parentScale.EntrywiseMultiply( m_localPose.Position ) );
	modelPose.Orientation = m_localPose.Orientation * parentPose.Orientation;
	Vector3f localStart = modelPose.Orientation.Inverted().Rotate( rayStart - modelPose.Position );
	Vector3f localDir = modelPose.Orientation.Inverted().Rotate( rayDir );
/*
    DROIDLOG( "Spam", "Hit test vs '%s', start: (%.2f, %.2f, %.2f ) cull bounds( %.2f, %.2f, %.2f ) -> ( %.2f, %.2f, %.2f )", GetText().toCString(),
            localStart.x, localStart.y, localStart.z,
            CullBounds.b[0].x, CullBounds.b[0].y, CullBounds.b[0].z,
            CullBounds.b[1].x, CullBounds.b[1].y, CullBounds.b[1].z );
*/
    // test against cull bounds if we have children  ... otherwise cullBounds == localBounds
    if ( m_children.sizeInt() > 0 )  
    {
        if ( m_cullBounds.IsInverted() )
        {
            DROIDLOG( "Spam", "CullBounds are inverted!!" );
            return false;
        }
	    float cullT0;
	    float cullT1;
		// any contents will hit cull bounds
		ContentFlags_t allContents( ALL_BITS );
	    bool hitCullBounds = intersectRayBounds( localStart, localDir, m_cullBounds.GetMins(), m_cullBounds.GetMaxs(), 
									allContents, cullT0, cullT1 );

//        DROIDLOG( "Spam", "Cull hit = %s, t0 = %.2f t1 = %.2f", hitCullBounds ? "true" : "false", cullT0, cullT1 );

	    if ( !hitCullBounds )
	    {
            return false;
        }
    }

	// test against self first, if not a container
    if ( contents() & testContents )
    {
        if ( m_flags & VRMENUOBJECT_BOUND_ALL )
        {
            // local bounds are the union of surface bounds and text bounds
            Bounds3f localBounds = getLocalBounds( font ) * parentScale;
            float t0;
	        float t1;
	        bool hit = intersectRayBounds( localStart, localDir, localBounds.GetMins(), localBounds.GetMaxs(), testContents, t0, t1 );
            if ( hit )
            {
                result.HitHandle = m_handle;
                result.t = t1;
				result.uv = Vector2f( 0.0f );	// unknown
            }
        }
        else
        {
	        float selfT0;
	        float selfT1;
			OvrCollisionResult cresult;
	        Bounds3f const & localBounds = getLocalBounds( font ) * parentScale;
            OVR_ASSERT( !localBounds.IsInverted() );

	        bool hit = intersectRay( localStart, localDir, parentScale, localBounds, selfT0, selfT1, testContents, cresult );
            if ( hit )
    	    {
				//app->ShowInfoText( 0.0f, "tri: %i", (int)cresult.TriIndex );
				result = cresult;
		        result.HitHandle = m_handle;
	        }

            // also check vs. the text bounds if there is any text
            if ( !m_text.isEmpty() && type() != VRMENU_CONTAINER && ( m_flags & VRMENUOBJECT_DONT_HIT_TEXT ) == 0 )
            {
                float textT0;
                float textT1;
                Bounds3f bounds = setTextLocalBounds( font ) * parentScale;
                bool textHit = intersectRayBounds( localStart, localDir, bounds.GetMins(), bounds.GetMaxs(), testContents, textT0, textT1 );
                if ( textHit && textT1 < result.t )
                {
                    result.HitHandle = m_handle;
                    result.t = textT1;
					result.uv = Vector2f( 0.0f );	// unknown
                }
            }
        }
    }

	// test against children
	for ( int i = 0; i < m_children.sizeInt(); ++i )
	{
		VRMenuObjectLocal * child = static_cast< VRMenuObjectLocal* >( menuMgr.toObject( m_children[i] ) );
		if ( child != NULL )
		{
			HitTestResult childResult;
			bool intersected = child-> hitTest_r( app, menuMgr, font, modelPose, scale, rayStart, rayDir, testContents, childResult );
			if ( intersected && childResult.t < result.t )
			{
				result = childResult;
			}
		}
    }
	return result.HitHandle.IsValid();
}

//==============================
// VRMenuObjectLocal::HitTest
menuHandle_t VRMenuObjectLocal::hitTest( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font, Posef const & worldPose, 
        Vector3f const & rayStart, Vector3f const & rayDir, ContentFlags_t const testContents, HitTestResult & result ) const
{
	 hitTest_r( app, menuMgr, font, worldPose, Vector3f( 1.0f ), rayStart, rayDir, testContents, result );

	return result.HitHandle;
}

//==============================
// VRMenuObjectLocal::RenderSurface
void VRMenuObjectLocal::renderSurface( OvrVRMenuMgr const & menuMgr, Matrix4f const & mvp, SubmittedMenuObject const & sub ) const
{
    m_surfaces[sub.surfaceIndex].render( menuMgr, mvp, sub );
}

//==============================
// VRMenuObjectLocal::GetLocalBounds
Bounds3f VRMenuObjectLocal::getLocalBounds( BitmapFont const & font ) const { 
	Bounds3f bounds;
	bounds.Clear();
    Vector3f const localScale = this->localScale();
	for ( int i = 0; i < m_surfaces.sizeInt(); i++ )
	{
        Bounds3f const & surfaceBounds = m_surfaces[i].localBounds() * localScale;
		bounds = Bounds3f::Union( bounds, surfaceBounds );
	}

	if ( m_collisionPrimitive != NULL )
	{
		bounds = Bounds3f::Union( bounds, m_collisionPrimitive->bounds() );
	}

    // transform surface bounds by whatever the hilight pose is
    if ( !bounds.IsInverted() )
    {
        bounds = Bounds3f::Transform( m_hilightPose, bounds );
    }

	// also union the text bounds, as long as we're not a container (containers don't render anything)
	if ( !m_text.isEmpty() > 0 && type() != VRMENU_CONTAINER )
	{
		bounds = Bounds3f::Union( bounds, setTextLocalBounds( font ) );
	}

    // if no valid surface bounds, then the local bounds is the local translation
    if ( bounds.IsInverted() )
    {
        bounds.AddPoint( m_localPose.Position );
        bounds = Bounds3f::Transform( m_hilightPose, bounds );
    }

	// after everything is calculated, expand (or contract) the bounds some custom amount
	bounds = Bounds3f::Expand( bounds, m_minsBoundsExpand, m_maxsBoundsExpand );
    
    return bounds;
}

//==============================
// VRMenuObjectLocal::GetTextLocalBounds
Bounds3f VRMenuObjectLocal::setTextLocalBounds( BitmapFont const & font ) const
{
    if ( m_textDirty )
    {
		m_textDirty = false;

		// also union the text bounds
		if ( m_text.isEmpty() )
		{
			m_textMetrics = textMetrics_t();
		}
		else
		{
			size_t len;
			int const MAX_LINES = 16;
			float lineWidths[MAX_LINES];
			int numLines = 0;

			font.CalcTextMetrics( m_text.toCString(), len, m_textMetrics.w, m_textMetrics.h, 
					m_textMetrics.ascent, m_textMetrics.descent, m_textMetrics.fontHeight, lineWidths, MAX_LINES, numLines );
		}
    }

	// NOTE: despite being 3 scalars, text scaling only uses the x component since
	// DrawText3D doesn't take separate x and y scales right now.
    Vector3f const localScale = this->localScale();
    Vector3f const textLocalScale = this->textLocalScale();
	float const scale = localScale.x * textLocalScale.x * m_fontParms.Scale;
	// this seems overly complex because font characters are rendered so that their origin
	// is on their baseline and not on one of the corners of the glyph. Because of this
	// we must treat the initial ascent (amount the font goes above the first baseline) and
	// final descent (amount the font goes below the final baseline) independently from the
	// lines in between when centering.
	Bounds3f textBounds( Vector3f( 0.0f, ( m_textMetrics.h - m_textMetrics.ascent ) * -1.0f, 0.0f ) * scale,
						 Vector3f( m_textMetrics.w, m_textMetrics.ascent, 0.0f ) * scale );

	Vector3f trans = Vector3f::ZERO;
	switch( m_fontParms.AlignVert )
	{
		case VERTICAL_BASELINE :
			trans.y = 0.0f;
			break;

		case VERTICAL_CENTER :
		{
			trans.y = ( m_textMetrics.h * 0.5f ) - m_textMetrics.ascent;
			break;
		}

		case VERTICAL_CENTER_FIXEDHEIGHT :
		{
			trans.y = ( m_textMetrics.fontHeight * -0.5f );
			break;
		}

		case VERTICAL_TOP :
		{
			trans.y = m_textMetrics.h - m_textMetrics.ascent;
			break;
		}
	}

	switch( m_fontParms.AlignHoriz )
	{
		case HORIZONTAL_LEFT :
			trans.x = 0.0f;
			break;

		case HORIZONTAL_CENTER :
		{
			trans.x = m_textMetrics.w * -0.5f;
			break;
		}
		case HORIZONTAL_RIGHT :
		{
			trans.x = m_textMetrics.w;
			break;
		}
	}

	textBounds.Translate( trans * scale );

	Bounds3f textLocalBounds = Bounds3f::Transform( textLocalPose(), textBounds );
	// transform by hilightpose here since surfaces are transformed by it before unioning the bounds
	textLocalBounds = Bounds3f::Transform( m_hilightPose, textLocalBounds );

	return textLocalBounds;
}

//==============================
// VRMenuObjectLocal::AddComponent
void VRMenuObjectLocal::addComponent( VRMenuComponent * component )
{
	if ( component == NULL )
	{
		return;	// this is fine... makes submitting VRMenuComponentParms easier.
	}

	int componentIndex = getComponentIndex( component );
	if ( componentIndex >= 0 )
	{
		// cannot add the same component twice!
		DROID_ASSERT( componentIndex < 0, "VRMenu" );
		return;
	}
	m_components.pushBack( component );
}

//==============================
// VRMenuObjectLocal::RemoveComponent
void VRMenuObjectLocal::removeComponent( VRMenuComponent * component )
{
	int componentIndex = getComponentIndex( component );
	if ( componentIndex < 0 )
	{
		return;
	}
	// maintain order because components of the same handler type may be have intentionally 
	// been added in a specific order
	m_components.removeAt( componentIndex );
}

//==============================
// VRMenuObjectLocal::GetComponentIndex
int VRMenuObjectLocal::getComponentIndex( VRMenuComponent * component ) const
{	
	for ( int i = 0; i < m_components.sizeInt(); ++i )
	{
		if ( m_components[i] == component )
		{
			return i;
		}
	}
	return -1;
}

//==============================
// VRMenuObjectLocal::GetComponentById
VRMenuComponent * VRMenuObjectLocal::getComponentById_Impl( int id ) const
{
	Array< VRMenuComponent* > comps = componentList( );
	for ( int c = 0; c < comps.sizeInt(); ++c )
	{
		if ( VRMenuComponent * comp = comps[ c ] )
		{
			if ( comp->typeId( ) == id )
			{
				return comp;
			}
		}
		else
		{
			OVR_ASSERT( comp );
		}
	}

	return NULL;
}

//==============================
// VRMenuObjectLocal::GetComponentByName
VRMenuComponent * VRMenuObjectLocal::getComponentByName_Impl( const char * typeName ) const
{
	Array< VRMenuComponent* > comps = componentList();
	for ( int c = 0; c < comps.sizeInt(); ++c )
	{
		if ( VRMenuComponent * comp = comps[ c ] )
		{
			if ( comp->typeName( ) == typeName )
			{
				return comp;
			}
		}
		else
		{
			OVR_ASSERT( comp );
		}
	}

	return NULL;
}

//==============================
// VRMenuObjectLocal::GetColorTableOffset
Vector2f const &	VRMenuObjectLocal::colorTableOffset() const
{
	return m_colorTableOffset;
}

//==============================
// VRMenuObjectLocal::SetColorTableOffset
void VRMenuObjectLocal::setColorTableOffset( Vector2f const & ofs )
{
	m_colorTableOffset = ofs;
}

//==============================
// VRMenuObjectLocal::GetColor
Vector4f const & VRMenuObjectLocal::color() const
{
	return m_color;
}

//==============================
// VRMenuObjectLocal::SetColor
void VRMenuObjectLocal::setColor( Vector4f const & c )
{
	m_color = c;
}

void VRMenuObjectLocal::setVisible( bool visible )
{
	if ( visible )
	{
		m_flags &= ~VRMenuObjectFlags_t( VRMENUOBJECT_DONT_RENDER );
	}
	else
	{
		m_flags |= VRMenuObjectFlags_t( VRMENUOBJECT_DONT_RENDER );
	}
}

//==============================
// VRMenuObjectLocal::ChildHandleForId
menuHandle_t VRMenuObjectLocal::childHandleForId( OvrVRMenuMgr & menuMgr, VRMenuId_t const id ) const
{
	int n = numChildren();
	for ( int i = 0; i < n; ++i )
	{
		VRMenuObjectLocal const * child = static_cast< VRMenuObjectLocal* >( menuMgr.toObject( getChildHandleForIndex( i ) ) );
		if ( child != NULL )
		{
			if ( child->id() == id )
			{
				return child->handle();
			}
			else
			{
				menuHandle_t handle = child->childHandleForId( menuMgr, id );
				if ( handle.IsValid() )
				{
					return handle;
				}
			}
		}
	}
	return menuHandle_t();
}

//==============================
// VRMenuObjectLocal::GetLocalScale
Vector3f VRMenuObjectLocal::localScale() const 
{ 
    return Vector3f( m_localScale.x * m_hilightScale, m_localScale.y * m_hilightScale, m_localScale.z * m_hilightScale ); 
}

//==============================
// VRMenuObjectLocal::GetTextLocalScale
Vector3f VRMenuObjectLocal::textLocalScale() const 
{
    return Vector3f( m_textLocalScale.x * m_hilightScale, m_textLocalScale.y * m_hilightScale, m_textLocalScale.z * m_hilightScale ); 
}

//==============================
// VRMenuObjectLocal::SetSurfaceTexture
void  VRMenuObjectLocal::setSurfaceTexture( int const surfaceIndex, int const textureIndex, 
        eSurfaceTextureType const type, GLuint const texId, int const width, int const height )
{
    if ( surfaceIndex < 0 || surfaceIndex >= m_surfaces.sizeInt() )
    {
        DROID_ASSERT( surfaceIndex >= 0 && surfaceIndex < m_surfaces.sizeInt(), "VrMenu" );
        return;
    }
    m_surfaces[surfaceIndex].loadTexture( textureIndex, type, texId, width, height );
}

//==============================
// VRMenuObjectLocal::SetSurfaceTexture
void  VRMenuObjectLocal::setSurfaceTextureTakeOwnership( int const surfaceIndex, int const textureIndex,
	eSurfaceTextureType const type, GLuint const texId,
	int const width, int const height )
{
	if ( surfaceIndex < 0 || surfaceIndex >= m_surfaces.sizeInt() )
	{
		DROID_ASSERT( surfaceIndex >= 0 && surfaceIndex < m_surfaces.sizeInt(), "VrMenu" );
		return;
	}
    m_surfaces[ surfaceIndex ].loadTexture( textureIndex, type, texId, width, height );
    m_surfaces[ surfaceIndex ].setOwnership( textureIndex, true );
}

//==============================
// VRMenuObjectLocal::RegenerateSurfaceGeometry
void VRMenuObjectLocal::regenerateSurfaceGeometry( int const surfaceIndex, const bool freeSurfaceGeometry )
{
	if ( surfaceIndex < 0 || surfaceIndex >= m_surfaces.sizeInt() )
	{
		DROID_ASSERT( surfaceIndex >= 0 && surfaceIndex < m_surfaces.sizeInt(), "VrMenu" );
		return;
	}

	if ( freeSurfaceGeometry )
	{
        m_surfaces[ surfaceIndex ].free();
	}

    m_surfaces[ surfaceIndex ].regenerateSurfaceGeometry();
}

//==============================
// VRMenuObjectLocal::GetSurfaceDims
Vector2f const & VRMenuObjectLocal::getSurfaceDims( int const surfaceIndex ) const
{
	if ( surfaceIndex < 0 || surfaceIndex >= m_surfaces.sizeInt() )
	{
		DROID_ASSERT( surfaceIndex >= 0 && surfaceIndex < m_surfaces.sizeInt(), "VrMenu" );
		return Vector2f::ZERO;
	}

    return m_surfaces[ surfaceIndex ].dims();
}

//==============================
// VRMenuObjectLocal::SetSurfaceDims
void VRMenuObjectLocal::setSurfaceDims( int const surfaceIndex, Vector2f const &dims )
{
	if ( surfaceIndex < 0 || surfaceIndex >= m_surfaces.sizeInt() )
	{
		DROID_ASSERT( surfaceIndex >= 0 && surfaceIndex < m_surfaces.sizeInt(), "VrMenu" );
		return;
	}

    m_surfaces[ surfaceIndex ].setDims( dims );
}

//==============================
// VRMenuObjectLocal::GetSurfaceBorder
Vector4f const & VRMenuObjectLocal::getSurfaceBorder( int const surfaceIndex )
{
	if ( surfaceIndex < 0 || surfaceIndex >= m_surfaces.sizeInt() )
	{
		DROID_ASSERT( surfaceIndex >= 0 && surfaceIndex < m_surfaces.sizeInt(), "VrMenu" );
		return Vector4f::ZERO;
	}

    return m_surfaces[ surfaceIndex ].border();
}

//==============================
// VRMenuObjectLocal::SetSurfaceBorder
void VRMenuObjectLocal::setSurfaceBorder( int const surfaceIndex, Vector4f const & border )
{
	if ( surfaceIndex < 0 || surfaceIndex >= m_surfaces.sizeInt() )
	{
		DROID_ASSERT( surfaceIndex >= 0 && surfaceIndex < m_surfaces.sizeInt(), "VrMenu" );
		return;
	}

    m_surfaces[ surfaceIndex ].setBorder( border );
}


//==============================
// VRMenuObjectLocal::SetLocalBoundsExpand
void VRMenuObjectLocal::setLocalBoundsExpand( Vector3f const mins, Vector3f const & maxs )
{
	m_minsBoundsExpand = mins;
	m_maxsBoundsExpand = maxs;
}

//==============================
// VRMenuObjectLocal::SetCollisionPrimitive
void VRMenuObjectLocal::setCollisionPrimitive( OvrCollisionPrimitive * c )
{
	if ( m_collisionPrimitive != NULL )
	{
		delete m_collisionPrimitive;
	}
	m_collisionPrimitive = c;
}

//==============================
//  VRMenuObjectLocal::FindSurfaceWithTextureType
int VRMenuObjectLocal::findSurfaceWithTextureType( eSurfaceTextureType const type, bool const singular ) const
{
	for ( int i = 0; i < m_surfaces.sizeInt(); ++i )
	{
		VRMenuSurface const & surf = m_surfaces[i];
		int numTextures = 0;
		bool hasType = false;
		// we have to look through the surface images because we don't know how many are valid
		for ( int j = 0; j < VRMENUSURFACE_IMAGE_MAX; j++ )
		{
			VRMenuSurfaceTexture const & texture = surf.getTexture( j );
			if ( texture.type() != SURFACE_TEXTURE_MAX )
			{
				numTextures++;
			}
			if ( texture.type() == type )
			{
				hasType = true;
			}
		}
		if ( hasType )
		{
			if ( !singular || ( singular && numTextures == 1 ) )
			{
				return i;
			}
		}
	}
	return -1;
}

//==============================
// VRMenuObjectLocal::SetSurfaceColor
void VRMenuObjectLocal::setSurfaceColor( int const surfaceIndex, Vector4f const & color )
{
	VRMenuSurface & surf = m_surfaces[surfaceIndex];
	surf.setColor( color );
}

//==============================
// VRMenuObjectLocal::GetSurfaceColor
Vector4f const & VRMenuObjectLocal::getSurfaceColor( int const surfaceIndex ) const
{
	VRMenuSurface const & surf = m_surfaces[surfaceIndex];
	return surf.color();
}

//==============================
// VRMenuObjectLocal::SetSurfaceVisible
void VRMenuObjectLocal::setSurfaceVisible( int const surfaceIndex, bool const v )
{
	VRMenuSurface & surf = m_surfaces[surfaceIndex];
	surf.setVisible( v );
}

//==============================
// VRMenuObjectLocal::GetSurfaceVisible
bool VRMenuObjectLocal::getSurfaceVisible( int const surfaceIndex ) const
{
	VRMenuSurface const & surf = m_surfaces[surfaceIndex];
	return surf.isVisible();
}

//==============================
// VRMenuObjectLocal::NumSurfaces
int VRMenuObjectLocal::numSurfaces() const
{
	return m_surfaces.sizeInt(); 
}

//==============================
// VRMenuObjectLocal::AllocSurface
int VRMenuObjectLocal::allocSurface()
{
	return m_surfaces.allocBack();
}


//==============================
// VRMenuObjectLocal::CreateFromSurfaceParms
void VRMenuObjectLocal::createFromSurfaceParms( int const surfaceIndex, VRMenuSurfaceParms const & parms )
{
	VRMenuSurface & surf = m_surfaces[surfaceIndex];
	surf.createFromSurfaceParms( parms );
}

//==============================
// VRMenuObjectLocal::SetTextWordWrapped
void VRMenuObjectLocal::setTextWordWrapped( char const * text, BitmapFont const & font, float const widthInMeters )
{
	setText( text );
	font.WordWrapText( m_text, widthInMeters, m_fontParms.Scale );
	m_wrapWidth = widthInMeters;
}

} // namespace NervGear
