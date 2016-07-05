#pragma once

#include "vglobal.h"

#include "VString.h"	// String
#include "../api/VGlShader.h"			// VGlShader
#include "ModelRender.h"		// ModelState
#include "ModelCollision.h"

NV_NAMESPACE_BEGIN

struct MaterialParms
{
	MaterialParms() :
		UseSrgbTextureFormats( false ),
		EnableDiffuseAniso( false ),
		EnableEmissiveLodClamp( true ),
		Transparent( false ),
		PolygonOffset( false ) { }

	bool	UseSrgbTextureFormats;		// use sRGB textures
	bool	EnableDiffuseAniso;			// enable anisotropic filtering on the diffuse texture
	bool	EnableEmissiveLodClamp;	// enable LOD clamp on the emissive texture to avoid light bleeding
	bool	Transparent;				// surfaces with this material flag need to render in a transparent pass
	bool	PolygonOffset;				// render with polygon offset enabled
};

struct ModelTexture
{
	VString		name;
	VTexture	texid;
};

enum ModelJointAnimation
{
	MODEL_JOINT_ANIMATION_NONE,
	MODEL_JOINT_ANIMATION_ROTATE,
	MODEL_JOINT_ANIMATION_SWAY,
	MODEL_JOINT_ANIMATION_BOB
};

struct ModelJoint
{
	int					index;
	VString				name;
    VR4Matrixf			transform;
	ModelJointAnimation	animation;
    V3Vectf			parameters;
	float				timeOffset;
	float				timeScale;
};

struct ModelTag
{
	VString		name;
    VR4Matrixf	matrix;
    V4Vecti	jointIndices;
    V4Vectf	jointWeights;
};

// A ModelFile is the in-memory representation of a digested model file.
// It should be imutable in normal circumstances, but it is ok to load
// and modify a model for a particular task, such as changing materials.
class ModelFile
{
public:
								ModelFile();
								ModelFile( const char * name ) : FileName( name ) {}
								~ModelFile();	// Frees all textures and geometry

	SurfaceDef *				FindNamedSurface( const char * name ) const;
	const ModelTexture *		FindNamedTexture( const char * name ) const;
	const ModelJoint *			FindNamedJoint( const char * name ) const;
	const ModelTag *			FindNamedTag(const VString &name ) const;

	int							GetJointCount() const { return Joints.length(); }
	const ModelJoint *			GetJoint( const int index ) const { return &Joints[index]; }
    VBoxf					GetBounds() const;

public:
	VString						FileName;
	bool						UsingSrgbTextures;

	// Textures will need to be freed if the model is unloaded,
	// and applications may include additional textures not
	// referenced directly by the scene geometry, such as
	// extra lighting passes.
	VArray< ModelTexture >		Textures;

	VArray< ModelJoint >			Joints;

	VArray< ModelTag >			Tags;

	// This is used by the rendering code
	ModelDef					Def;

	// This is used by the movement code
	CollisionModel				Collisions;
	CollisionModel				GroundCollisions;

	// This is typically used for gaze selection.
//	ModelTrace					TraceModel;
};

struct ModelGlPrograms
{
	ModelGlPrograms() :
		ProgVertexColor( NULL ),
		ProgSingleTexture( NULL ),
		ProgLightMapped( NULL ),
		ProgReflectionMapped( NULL ),
		ProgSkinnedVertexColor( NULL ),
		ProgSkinnedSingleTexture( NULL ),
		ProgSkinnedLightMapped( NULL ),
		ProgSkinnedReflectionMapped( NULL ) {}
	ModelGlPrograms( const VGlShader * singleTexture ) :
		ProgVertexColor( singleTexture ),
		ProgSingleTexture( singleTexture ),
		ProgLightMapped( singleTexture ),
		ProgReflectionMapped( singleTexture ),
		ProgSkinnedVertexColor( singleTexture ),
		ProgSkinnedSingleTexture( singleTexture ),
		ProgSkinnedLightMapped( singleTexture ),
		ProgSkinnedReflectionMapped( singleTexture ) {}
	ModelGlPrograms( const VGlShader * singleTexture, const VGlShader * dualTexture ) :
		ProgVertexColor( singleTexture ),
		ProgSingleTexture( singleTexture ),
		ProgLightMapped( dualTexture ),
		ProgReflectionMapped( dualTexture ),
		ProgSkinnedVertexColor( singleTexture ),
		ProgSkinnedSingleTexture( singleTexture ),
		ProgSkinnedLightMapped( dualTexture ),
		ProgSkinnedReflectionMapped( dualTexture ) {}

	const VGlShader * ProgVertexColor;
	const VGlShader * ProgSingleTexture;
	const VGlShader * ProgLightMapped;
	const VGlShader * ProgReflectionMapped;
	const VGlShader * ProgSkinnedVertexColor;
	const VGlShader * ProgSkinnedSingleTexture;
	const VGlShader * ProgSkinnedLightMapped;
	const VGlShader * ProgSkinnedReflectionMapped;
};

NV_NAMESPACE_END


