/************************************************************************************

Filename    :   VRMenuObjectLocal.cpp
Content     :   Menuing system for VR apps.
Created     :   May 23, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#include "VRMenuObjectLocal.h"

#include "../api/VGlShader.h"
#include "GlTexture.h"
#include "App.h"			// for loading images from the assets folder
#include "ModelTrace.h"
#include "BitmapFont.h"
#include "VRMenuMgr.h"
#include "VRMenuComponent.h"
#include "ui_default.h"	// embedded default UI texture (loaded as a placeholder when something doesn't load)
#include "VApkFile.h"
#include "VImageManager.h"
#include "VOpenGLTexture.h"
#include "core/VLog.h"

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
bool VRMenuSurfaceTexture::loadTexture( eSurfaceTextureType const type, const VString &imageName, bool const allowDefault )
{
    free();

	vAssert( type >= 0 && type < SURFACE_TEXTURE_MAX );

	m_type = type;

    if (!imageName.isEmpty()) {
        VImageManager* imagemanager = new VImageManager();
        VImage* image = imagemanager->loadImage(imageName);
        m_width = image->getDimension().Width;
        m_height = image->getDimension().Height;
        m_handle = VOpenGLTexture(image, imageName, TextureFlags_o(_NO_DEFAULT )).getTextureName();

        delete imagemanager;


	}

	if ( m_handle == 0 && allowDefault )
	{
        m_handle = LoadTextureFromBuffer( imageName.toUtf8().data(), uiDefaultTgaData, uiDefaultTgaSize,
							TextureFlags_t(), m_width, m_height );
		vWarn("VRMenuSurfaceTexture::CreateFromImage: failed to load image '" << imageName << "' - default loaded instead!");
	}
    m_ownsTexture = true;
	return m_handle != 0;
}

//==============================
// VRMenuSurfaceTexture::LoadTexture
void VRMenuSurfaceTexture::loadTexture( eSurfaceTextureType const type, const GLuint texId, const int width, const int height )
{
	free();

	vAssert( type >= 0 && type < SURFACE_TEXTURE_MAX );

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
static void PrintBounds( const char * name, char const * prefix, VBoxf const & bounds )
{
	vInfo("'" << name << "' " << prefix << ": min( " <<
	        bounds.GetMins().x << ", " << bounds.GetMins().y << ", " << bounds.GetMins().z << " ) - max( " <<
	        bounds.GetMaxs().x << ", " << bounds.GetMaxs().y << ", " << bounds.GetMaxs().z) << " )";
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
void VRMenuSurface::createImageGeometry( int const textureWidth, int const textureHeight, const V2Vectf &dims, const V4Vectf &border, ContentFlags_t const contents )
{
	//vAssert( Geo.vertexBuffer == 0 && Geo.indexBuffer == 0 && Geo.vertexArrayObject == 0 );

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
	attribs.uvCoordinate0.resize( vertexCount );
	attribs.uvCoordinate1.resize( vertexCount );
	attribs.color.resize( vertexCount );

    V4Vectf color( 1.0f, 1.0f, 1.0f, 1.0f );

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
			attribs.uvCoordinate0[index].x = vertUVX[ x ];
			attribs.uvCoordinate0[index].y = uvY;
			attribs.uvCoordinate1[index] = attribs.uvCoordinate0[index];
			attribs.color[index] = color;
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

    m_tris.init( attribs.position, indices, contents );

	if ( m_geo.vertexBuffer == 0 && m_geo.indexBuffer == 0 && m_geo.vertexArrayObject == 0 )
	{
		m_geo.createGlGeometry( attribs, indices );
	}
	else
	{
		m_geo.updateGlGeometry( attribs );
	}
}

//==============================
// VRMenuSurface::Render
void VRMenuSurface::render( OvrVRMenuMgr const & menuMgr, VR4Matrixf const & mvp, SubmittedMenuObject const & sub ) const
{
	if ( m_geo.vertexCount == 0 )
	{
		return;	// surface wasn't initialized with any geometry -- this can happen if diffuse and additive are both invalid
	}

    //vInfo("Render Surface '" << SurfaceName << "', skip = '" << skipAdditivePass ? "true" : "false" << "'");


    VEglDriver::logErrorsEnum( "VRMenuSurface::Render - pre" );

	VGlShader const * program = NULL;

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
			vAssert( diffuseIndex >= 0);	// surface setup should have detected this!
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
			vAssert( diffuseIndex >= 0);	// surface setup should have detected this!
			int diffuse2Index = indexForTextureType( SURFACE_TEXTURE_DIFFUSE, 2 );
			vAssert( diffuse2Index >= 0);	// surface setup should have detected this!
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
			vAssert( additiveIndex >= 0);	// surface setup should have detected this!
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
			vAssert( diffuseIndex >= 0);	// surface setup should have detected this!
			int additiveIndex = indexForTextureType( SURFACE_TEXTURE_ADDITIVE, 1 );
			vAssert( additiveIndex >= 0);	// surface setup should have detected this!
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
			vAssert( diffuseIndex >= 0);	// surface setup should have detected this!
			int rampIndex = indexForTextureType( SURFACE_TEXTURE_COLOR_RAMP, 1 );
			vAssert( rampIndex >= 0);	// surface setup should have detected this!
			// bind both textures
			glActiveTexture( GL_TEXTURE0 );
			glBindTexture( GL_TEXTURE_2D, m_textures[diffuseIndex].handle() );
			glActiveTexture( GL_TEXTURE1 );
			glBindTexture( GL_TEXTURE_2D, m_textures[rampIndex].handle() );
			// do not do any filtering on the "palette" texture
            glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			break;
		}
		case PROGRAM_DIFFUSE_COLOR_RAMP_TARGET:	// has diffuse, color ramp, and a separate color ramp target
		{
			//vInfo("Surface '" << SurfaceName << "' - PROGRAM_COLOR_RAMP_TARGET");
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
			int diffuseIndex = indexForTextureType( SURFACE_TEXTURE_DIFFUSE, 1 );
			vAssert( diffuseIndex >= 0);	// surface setup should have detected this!
			int rampIndex = indexForTextureType( SURFACE_TEXTURE_COLOR_RAMP, 1 );
			vAssert( rampIndex >= 0);	// surface setup should have detected this!
			int targetIndex = indexForTextureType( SURFACE_TEXTURE_COLOR_RAMP_TARGET, 1 );
			vAssert( targetIndex >= 0);	// surface setup should have detected this!
			// bind both textures
			glActiveTexture( GL_TEXTURE0 );
			glBindTexture( GL_TEXTURE_2D, m_textures[diffuseIndex].handle() );
			glActiveTexture( GL_TEXTURE1 );
			glBindTexture( GL_TEXTURE_2D, m_textures[targetIndex].handle() );
			glActiveTexture( GL_TEXTURE2 );
			glBindTexture( GL_TEXTURE_2D, m_textures[rampIndex].handle() );
			// do not do any filtering on the "palette" texture
            glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
			break;
		}
		case PROGRAM_MAX:
		{
			vWarn("Unsupported texture map combination.");
			return;
		}
		default:
		{
			vAssert( !"Unhandled ProgramType");
			return;
		}
	}

	vAssert( program != NULL);

	glUseProgram( program->program );

    glUniformMatrix4fv( program->uniformModelViewProMatrix, 1, GL_FALSE, mvp.M[0] );

	glUniform4fv( program->uniformColor, 1, &sub.color.x );
	glUniform3fv( program->uniformFadeDirection, 1, &sub.fadeDirection.x );
	glUniform2fv( program->uniformColorTableOffset, 1, &sub.colorTableOffset.x );

	// render
    VEglDriver::glBindVertexArrayOES( m_geo.vertexArrayObject );
	glDrawElements( GL_TRIANGLES, m_geo.indexCount, GL_UNSIGNED_SHORT, NULL );
    VEglDriver::glBindVertexArrayOES( 0 );

    VEglDriver::logErrorsEnum( "VRMenuSurface::Render - post" );
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
            m_textures[i].loadTexture(parms.TextureTypes[i], parms.ImageNames[i], true);
		}
		else if ( ( parms.ImageTexId[i] != 0 ) &&
            ( parms.TextureTypes[i] >= 0 && parms.TextureTypes[i] < SURFACE_TEXTURE_MAX ) )
	    {
    		isValid = true;
            m_textures[i].loadTexture(parms.TextureTypes[i], parms.ImageTexId[i], parms.ImageWidth[i], parms.ImageHeight[i]);
		}
	}
	if ( !isValid )
	{
		//vInfo("VRMenuSurfaceParms '" << parms.SurfaceName << "' - no valid images - skipping");
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
		//vInfo("VRMenuSurface::CreateFromImageParms - no suitable image for surface creation");
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
		vWarn("Invalid material combination -- either add a shader to support it or fix it.");
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
bool VRMenuSurface::intersectRay( V3Vectf const & start, V3Vectf const & dir, VPosf const & pose,
                                  V3Vectf const & scale, ContentFlags_t const testContents,
								  OvrCollisionResult & result ) const
{
    return m_tris.intersectRay( start, dir, pose, scale, testContents, result );
}

//==============================
// VRMenuSurface::IntersectRay
bool VRMenuSurface::intersectRay( V3Vectf const & localStart, V3Vectf const & localDir,
                                  V3Vectf const & scale, ContentFlags_t const testContents,
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
        vAssert( textureIndex >= 0 && textureIndex < VRMENUSURFACE_IMAGE_MAX);
        return;
    }
    m_textures[textureIndex].loadTexture( type, texId, width, height );
}

//==============================
// VRMenuSurface::GetAnchorOffsets
V2Vectf VRMenuSurface::anchorOffsets() const {
    return V2Vectf( ( ( 1.0f - m_anchors.x ) - 0.5f ) * m_dims.x * VRMenuObject::DEFAULT_TEXEL_SCALE, // inverted so that 0.0 is left-aligned
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
    m_hilightPose( VQuatf(), V3Vectf( 0.0f, 0.0f, 0.0f ) ),
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
    for ( int i = 0; i < m_components.length(); ++i )
    {
        delete m_components[i];
        m_components[i] = NULL;
    }
    m_components.clear();
	m_handle.reset();
	m_parentHandle.reset();
	m_type = VRMENU_MAX;
}

//==================================
// VRMenuObjectLocal::Init
void VRMenuObjectLocal::init( VRMenuObjectParms const & parms )
{
	for ( int i = 0; i < parms.SurfaceParms.length(); ++i )
	{
		int idx = m_surfaces.allocBack();
        m_surfaces[idx].createFromSurfaceParms( parms.SurfaceParms[i] );
	}

	// bounds are nothing submitted for rendering
    m_cullBounds = VBoxf( 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f );
	m_fontParms = parms.FontParms;
    for ( int i = 0; i < parms.Components.length(); ++i )
    {
	    addComponent( parms.Components[i] );
    }
}

//==================================
// VRMenuObjectLocal::FreeChildren
void VRMenuObjectLocal::freeChildren( OvrVRMenuMgr & menuMgr )
{
	for ( int i = 0; i < m_children.length(); ++i )
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
	for ( int i = 0; i < m_children.length(); ++i )
	{
		if ( m_children[i] == handle )
		{
			return true;
		}
	}

	for ( int i = 0; i < m_children.length(); ++i )
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
	m_children.append( handle );

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
	for ( int i = 0; i < m_children.length(); ++i )
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
	for ( int i = 0; i < m_children.length(); ++i)
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
void VRMenuObjectLocal::frame( OvrVRMenuMgr & menuMgr, VR4Matrixf const & viewMatrix )
{
	for ( int i = 0; i < m_children.length(); ++i )
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
bool VRMenuObjectLocal::intersectRayBounds( V3Vectf const & start, V3Vectf const & dir,
        V3Vectf const & mins, V3Vectf const & maxs, ContentFlags_t const testContents, float & t0, float & t1 ) const
{
	if ( !( testContents & contents() ) )
	{
		return false;
	}

    if ( VBoxf( mins, maxs ).Contains( start, 0.1f ) )
    {
        return true;
    }
	Intersect_RayBounds( start, dir, mins, maxs, t0, t1 );
	return t0 >= 0.0f && t1 >= 0.0f && t1 >= t0;
}

//==============================
// VRMenuObjectLocal::IntersectRay
bool VRMenuObjectLocal::intersectRay( V3Vectf const & localStart, V3Vectf const & localDir, V3Vectf const & parentScale, VBoxf const & bounds,
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
    V3Vectf const scale = localScale() * parentScale;

	// test vs. collision primitive
	if ( m_collisionPrimitive != NULL )
	{
		m_collisionPrimitive->intersectRay( localStart, localDir, scale, testContents, result );
	}

	// test vs. surfaces
	if (  type() != VRMENU_CONTAINER )
	{
		int numSurfaces = 0;
		for ( int i = 0; i < m_surfaces.length(); ++i )
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
        VPosf const & parentPose, V3Vectf const & parentScale, V3Vectf const & rayStart, V3Vectf const & rayDir,
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
    V3Vectf const & localScale = this->localScale();
    V3Vectf scale = parentScale.EntrywiseMultiply( localScale );
    VPosf modelPose;
	modelPose.Position = parentPose.Position + ( parentPose.Orientation * parentScale.EntrywiseMultiply( m_localPose.Position ) );
	modelPose.Orientation = m_localPose.Orientation * parentPose.Orientation;
    V3Vectf localStart = modelPose.Orientation.Inverted().Rotate( rayStart - modelPose.Position );
    V3Vectf localDir = modelPose.Orientation.Inverted().Rotate( rayDir );
    if ( m_children.length() > 0 )
    {
        if ( m_cullBounds.IsInverted() )
        {
            vVerbose("CullBounds are inverted!!");
            return false;
        }
	    float cullT0;
	    float cullT1;
		// any contents will hit cull bounds
        ContentFlags_t allContents;
        allContents.setAll();
	    bool hitCullBounds = intersectRayBounds( localStart, localDir, m_cullBounds.GetMins(), m_cullBounds.GetMaxs(),
									allContents, cullT0, cullT1 );

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
            VBoxf localBounds = getTextLocalBounds( font ) * parentScale;
            float t0;
	        float t1;
	        bool hit = intersectRayBounds( localStart, localDir, localBounds.GetMins(), localBounds.GetMaxs(), testContents, t0, t1 );
            if ( hit )
            {
                result.HitHandle = m_handle;
                result.t = t1;
                result.uv = V2Vectf( 0.0f );	// unknown
            }
        }
        else
        {
	        float selfT0;
	        float selfT1;
			OvrCollisionResult cresult;
            VBoxf const & localBounds = getTextLocalBounds( font ) * parentScale;
            vAssert( !localBounds.IsInverted() );

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
                VBoxf bounds = setTextLocalBounds( font ) * parentScale;
                bool textHit = intersectRayBounds( localStart, localDir, bounds.GetMins(), bounds.GetMaxs(), testContents, textT0, textT1 );
                if ( textHit && textT1 < result.t )
                {
                    result.HitHandle = m_handle;
                    result.t = textT1;
                    result.uv = V2Vectf( 0.0f );	// unknown
                }
            }
        }
    }

	// test against children
	for ( int i = 0; i < m_children.length(); ++i )
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
	return result.HitHandle.isValid();
}

//==============================
// VRMenuObjectLocal::HitTest
menuHandle_t VRMenuObjectLocal::hitTest( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font, VPosf const & worldPose,
        V3Vectf const & rayStart, V3Vectf const & rayDir, ContentFlags_t const testContents, HitTestResult & result ) const
{
     hitTest_r( app, menuMgr, font, worldPose, V3Vectf( 1.0f ), rayStart, rayDir, testContents, result );

	return result.HitHandle;
}

//==============================
// VRMenuObjectLocal::RenderSurface
void VRMenuObjectLocal::renderSurface( OvrVRMenuMgr const & menuMgr, VR4Matrixf const & mvp, SubmittedMenuObject const & sub ) const
{
    m_surfaces[sub.surfaceIndex].render( menuMgr, mvp, sub );
}

//==============================
// VRMenuObjectLocal::GetLocalBounds
VBoxf VRMenuObjectLocal::getTextLocalBounds( BitmapFont const & font ) const
{
    VBoxf bounds;
	bounds.Clear();
    V3Vectf const localScale = this->localScale();
	for ( int i = 0; i < m_surfaces.length(); i++ )
	{
        VBoxf const & surfaceBounds = m_surfaces[i].localBounds() * localScale;
        bounds = VBoxf::Union( bounds, surfaceBounds );
	}

	if ( m_collisionPrimitive != NULL )
	{
        bounds = VBoxf::Union( bounds, m_collisionPrimitive->bounds() );
	}

    // transform surface bounds by whatever the hilight pose is
    if ( !bounds.IsInverted() )
    {
        bounds = VBoxf::Transform( m_hilightPose, bounds );
    }

	// also union the text bounds, as long as we're not a container (containers don't render anything)
	if ( !m_text.isEmpty() > 0 && type() != VRMENU_CONTAINER )
	{
        bounds = VBoxf::Union( bounds, setTextLocalBounds( font ) );
	}

    // if no valid surface bounds, then the local bounds is the local translation
    if ( bounds.IsInverted() )
    {
        bounds.AddPoint( m_localPose.Position );
        bounds = VBoxf::Transform( m_hilightPose, bounds );
    }

	// after everything is calculated, expand (or contract) the bounds some custom amount
    bounds = VBoxf::Expand( bounds, m_minsBoundsExpand, m_maxsBoundsExpand );

    return bounds;
}

//==============================
// VRMenuObjectLocal::GetTextLocalBounds
VBoxf VRMenuObjectLocal::setTextLocalBounds( BitmapFont const & font ) const
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

            font.CalcTextMetrics(m_text, len, m_textMetrics.w, m_textMetrics.h,
					m_textMetrics.ascent, m_textMetrics.descent, m_textMetrics.fontHeight, lineWidths, MAX_LINES, numLines );
		}
    }

	// NOTE: despite being 3 scalars, text scaling only uses the x component since
	// DrawText3D doesn't take separate x and y scales right now.
    V3Vectf const localScale = this->localScale();
    V3Vectf const textLocalScale = this->textLocalScale();
	float const scale = localScale.x * textLocalScale.x * m_fontParms.Scale;
	// this seems overly complex because font characters are rendered so that their origin
	// is on their baseline and not on one of the corners of the glyph. Because of this
	// we must treat the initial ascent (amount the font goes above the first baseline) and
	// final descent (amount the font goes below the final baseline) independently from the
	// lines in between when centering.
    VBoxf textBounds( V3Vectf( 0.0f, ( m_textMetrics.h - m_textMetrics.ascent ) * -1.0f, 0.0f ) * scale,
                         V3Vectf( m_textMetrics.w, m_textMetrics.ascent, 0.0f ) * scale );

    V3Vectf trans = V3Vectf::ZERO;
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

    VBoxf textLocalBounds = VBoxf::Transform( textLocalPose(), textBounds );
	// transform by hilightpose here since surfaces are transformed by it before unioning the bounds
    textLocalBounds = VBoxf::Transform( m_hilightPose, textLocalBounds );

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
		vAssert( componentIndex < 0);
		return;
	}
	m_components.append( component );
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
	for ( int i = 0; i < m_components.length(); ++i )
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
	VArray< VRMenuComponent* > comps = componentList( );
	for ( int c = 0; c < comps.length(); ++c )
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
			vAssert( comp );
		}
	}

	return NULL;
}

//==============================
// VRMenuObjectLocal::GetComponentByName
VRMenuComponent * VRMenuObjectLocal::getComponentByName_Impl( const char * typeName ) const
{
	VArray< VRMenuComponent* > comps = componentList();
	for ( int c = 0; c < comps.length(); ++c )
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
			vAssert( comp );
		}
	}

	return NULL;
}

//==============================
// VRMenuObjectLocal::GetColorTableOffset
V2Vectf const &	VRMenuObjectLocal::colorTableOffset() const
{
	return m_colorTableOffset;
}

//==============================
// VRMenuObjectLocal::SetColorTableOffset
void VRMenuObjectLocal::setColorTableOffset( V2Vectf const & ofs )
{
	m_colorTableOffset = ofs;
}

//==============================
// VRMenuObjectLocal::GetColor
V4Vectf const & VRMenuObjectLocal::color() const
{
	return m_color;
}

//==============================
// VRMenuObjectLocal::SetColor
void VRMenuObjectLocal::setColor( V4Vectf const & c )
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
				if ( handle.isValid() )
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
V3Vectf VRMenuObjectLocal::localScale() const
{
    return V3Vectf( m_localScale.x * m_hilightScale, m_localScale.y * m_hilightScale, m_localScale.z * m_hilightScale );
}

//==============================
// VRMenuObjectLocal::GetTextLocalScale
V3Vectf VRMenuObjectLocal::textLocalScale() const
{
    return V3Vectf( m_textLocalScale.x * m_hilightScale, m_textLocalScale.y * m_hilightScale, m_textLocalScale.z * m_hilightScale );
}

//==============================
// VRMenuObjectLocal::SetSurfaceTexture
void  VRMenuObjectLocal::setSurfaceTexture( int const surfaceIndex, int const textureIndex,
        eSurfaceTextureType const type, GLuint const texId, int const width, int const height )
{
    if ( surfaceIndex < 0 || surfaceIndex >= m_surfaces.length() )
    {
        vAssert( surfaceIndex >= 0 && surfaceIndex < m_surfaces.length());
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
	if ( surfaceIndex < 0 || surfaceIndex >= m_surfaces.length() )
	{
		vAssert( surfaceIndex >= 0 && surfaceIndex < m_surfaces.length());
		return;
	}
    m_surfaces[ surfaceIndex ].loadTexture( textureIndex, type, texId, width, height );
    m_surfaces[ surfaceIndex ].setOwnership( textureIndex, true );
}

//==============================
// VRMenuObjectLocal::RegenerateSurfaceGeometry
void VRMenuObjectLocal::regenerateSurfaceGeometry( int const surfaceIndex, const bool freeSurfaceGeometry )
{
	if ( surfaceIndex < 0 || surfaceIndex >= m_surfaces.length() )
	{
		vAssert( surfaceIndex >= 0 && surfaceIndex < m_surfaces.length());
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
V2Vectf const & VRMenuObjectLocal::getSurfaceDims( int const surfaceIndex ) const
{
	if ( surfaceIndex < 0 || surfaceIndex >= m_surfaces.length() )
	{
		vAssert( surfaceIndex >= 0 && surfaceIndex < m_surfaces.length());
        return V2Vectf::ZERO;
	}

    return m_surfaces[ surfaceIndex ].dims();
}

//==============================
// VRMenuObjectLocal::SetSurfaceDims
void VRMenuObjectLocal::setSurfaceDims( int const surfaceIndex, V2Vectf const &dims )
{
	if ( surfaceIndex < 0 || surfaceIndex >= m_surfaces.length() )
	{
		vAssert( surfaceIndex >= 0 && surfaceIndex < m_surfaces.length());
		return;
	}

    m_surfaces[ surfaceIndex ].setDims( dims );
}

//==============================
// VRMenuObjectLocal::GetSurfaceBorder
V4Vectf const & VRMenuObjectLocal::getSurfaceBorder( int const surfaceIndex )
{
	if ( surfaceIndex < 0 || surfaceIndex >= m_surfaces.length() )
	{
		vAssert( surfaceIndex >= 0 && surfaceIndex < m_surfaces.length());
        return V4Vectf::ZERO;
	}

    return m_surfaces[ surfaceIndex ].border();
}

//==============================
// VRMenuObjectLocal::SetSurfaceBorder
void VRMenuObjectLocal::setSurfaceBorder( int const surfaceIndex, V4Vectf const & border )
{
	if ( surfaceIndex < 0 || surfaceIndex >= m_surfaces.length() )
	{
		vAssert( surfaceIndex >= 0 && surfaceIndex < m_surfaces.length());
		return;
	}

    m_surfaces[ surfaceIndex ].setBorder( border );
}


//==============================
// VRMenuObjectLocal::SetLocalBoundsExpand
void VRMenuObjectLocal::setLocalBoundsExpand( V3Vectf const mins, V3Vectf const & maxs )
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
	for ( int i = 0; i < m_surfaces.length(); ++i )
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
void VRMenuObjectLocal::setSurfaceColor( int const surfaceIndex, V4Vectf const & color )
{
	VRMenuSurface & surf = m_surfaces[surfaceIndex];
	surf.setColor( color );
}

//==============================
// VRMenuObjectLocal::GetSurfaceColor
V4Vectf const & VRMenuObjectLocal::getSurfaceColor( int const surfaceIndex ) const
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
	return m_surfaces.length();
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
void VRMenuObjectLocal::setTextWordWrapped(const VString &text, BitmapFont const & font, float const widthInMeters )
{
	setText( text );
	font.WordWrapText( m_text, widthInMeters, m_fontParms.Scale );
	m_wrapWidth = widthInMeters;
}

} // namespace NervGear
