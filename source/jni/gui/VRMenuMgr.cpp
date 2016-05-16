/************************************************************************************

Filename    :   OvrMenuMgr.cpp
Content     :   Menuing system for VR apps.
Created     :   May 23, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#include "VRMenuMgr.h"
#include "api/VEglDriver.h"
#include "VAlgorithm.h"
#include "../api/VGlShader.h"
#include "GlTexture.h"
#include "../api/VGlGeometry.h"
#include "ModelView.h"
#include "DebugLines.h"
#include "BitmapFont.h"
#include "VRMenuObjectLocal.h"
#include <algorithm>


namespace NervGear {

// diffuse-only programs
char const* GUIDiffuseOnlyVertexShaderSrc =
	"uniform mat4 Mvpm;\n"
	"uniform lowp vec4 UniformColor;\n"
	"uniform vec3 UniformFadeDirection;\n"
	"attribute vec4 Position;\n"
	"attribute vec2 TexCoord;\n"
	"attribute vec4 VertexColor;\n"
	"varying highp vec2 oTexCoord;\n"
	"varying lowp vec4 oColor;\n"
	"void main()\n"
	"{\n"
	"    gl_Position = Mvpm * Position;\n"
	"    oTexCoord = TexCoord;\n"
	"    oColor = UniformColor * VertexColor;\n"
	// Fade out vertices if direction is positive
	"    if ( dot(UniformFadeDirection, UniformFadeDirection) > 0.0 )\n"
	"	 {\n"
	"        if ( dot(UniformFadeDirection, Position.xyz ) > 0.0 ) { oColor[3] = 0.0; }\n"
	"    }\n"
	"}\n";

char const* GUIDiffuseOnlyFragmentShaderSrc =
	"uniform sampler2D Texture0;\n"
	"varying highp vec2 oTexCoord;\n"
	"varying lowp vec4 oColor;\n"
	"void main()\n"
	"{\n"
	"    gl_FragColor = oColor * texture2D( Texture0, oTexCoord );\n"
	"}\n";

// diffuse color ramped programs
static const char * GUIColorRampFragmentSrc =
	"uniform sampler2D Texture0;\n"
	"varying highp vec2 oTexCoord;\n"
	"varying lowp vec4 oColor;\n"
	"uniform sampler2D Texture1;\n"
	"uniform mediump vec2 ColorTableOffset;\n"
	"void main()\n"
	"{\n"
	"    lowp vec4 texel = texture2D( Texture0, oTexCoord );\n"
	"    lowp vec2 colorIndex = vec2( ColorTableOffset.x + texel.x, ColorTableOffset.y );\n"
	"    lowp vec4 remappedColor = texture2D( Texture1, colorIndex.xy );\n"
	"    gl_FragColor = oColor * vec4( remappedColor.xyz, texel.a );\n"
	"}\n";

// diffuse + color ramped + target programs
char const* GUIDiffuseColorRampTargetVertexShaderSrc =
	"uniform mat4 Mvpm;\n"
	"uniform lowp vec4 UniformColor;\n"
	"uniform lowp vec3 UniformFadeDirection;\n"
	"attribute vec4 Position;\n"
	"attribute vec2 TexCoord;\n"
	"attribute vec2 TexCoord1;\n"
	"attribute vec4 VertexColor;\n"
	"varying highp vec2 oTexCoord;\n"
	"varying highp vec2 oTexCoord1;\n"
	"varying lowp vec4 oColor;\n"
	"void main()\n"
	"{\n"
	"    gl_Position = Mvpm * Position;\n"
	"    oTexCoord = TexCoord;\n"
	"    oTexCoord1 = TexCoord1;\n"
	"    oColor = UniformColor * VertexColor;\n"
	// Fade out vertices if direction is positive
	"    if ( dot(UniformFadeDirection, UniformFadeDirection) > 0.0 )\n"
	"	 {\n"
	"       if ( dot(UniformFadeDirection, Position.xyz ) > 0.0 ) { oColor[3] = 0.0; }\n"
	"    }\n"
	"}\n";

static const char * GUIColorRampTargetFragmentSrc =
	"uniform sampler2D Texture0;\n"
	"uniform sampler2D Texture1;\n"	// color ramp target
	"uniform sampler2D Texture2;\n"	// color ramp
	"varying highp vec2 oTexCoord;\n"
	"varying highp vec2 oTexCoord1;\n"
	"varying lowp vec4 oColor;\n"
	"uniform mediump vec2 ColorTableOffset;\n"
	"void main()\n"
	"{\n"
	"    mediump vec4 lookup = texture2D( Texture1, oTexCoord1 );\n"
	"    mediump vec2 colorIndex = vec2( ColorTableOffset.x + lookup.x, ColorTableOffset.y );\n"
	"    mediump vec4 remappedColor = texture2D( Texture2, colorIndex.xy );\n"
//	"    gl_FragColor = lookup;\n"
	"    mediump vec4 texel = texture2D( Texture0, oTexCoord );\n"
	"    mediump vec3 blended = ( texel.xyz * ( 1.0 - lookup.a ) ) + ( remappedColor.xyz * lookup.a );\n"
//	"    mediump vec3 blended = ( texel.xyz * ( 1.0 - lookup.a ) ) + ( lookup.xyz * lookup.a );\n"
	"    gl_FragColor = oColor * vec4( blended.xyz, texel.a );\n"
//	"    gl_FragColor = texel;\n"
	"}\n";

// diffuse + additive programs
char const* GUITwoTextureColorModulatedShaderSrc =
	"uniform mat4 Mvpm;\n"
	"uniform lowp vec4 UniformColor;\n"
	"uniform lowp vec3 UniformFadeDirection;\n"
	"attribute vec4 Position;\n"
	"attribute vec2 TexCoord;\n"
	"attribute vec2 TexCoord1;\n"
	"attribute vec4 VertexColor;\n"
	"attribute vec4 Parms;\n"
	"varying highp vec2 oTexCoord;\n"
	"varying highp vec2 oTexCoord1;\n"
	"varying lowp vec4 oColor;\n"
	"void main()\n"
	"{\n"
	"    gl_Position = Mvpm * Position;\n"
	"    oTexCoord = TexCoord;\n"
	"    oTexCoord1 = TexCoord1;\n"
	"    oColor = UniformColor * VertexColor;\n"
	// Fade out vertices if direction is positive
	"    if ( dot(UniformFadeDirection, UniformFadeDirection) > 0.0 )\n"
	"	 {\n"
	"        if ( dot(UniformFadeDirection, Position.xyz ) > 0.0 ) { oColor[3] = 0.0; }\n"
	"    }\n"
	"}\n";

char const* GUIDiffusePlusAdditiveFragmentShaderSrc =
	"uniform sampler2D Texture0;\n"
	"uniform sampler2D Texture1;\n"
	"varying highp vec2 oTexCoord;\n"
	"varying highp vec2 oTexCoord1;\n"
	"varying lowp vec4 oColor;\n"
	"void main()\n"
	"{\n"
	"    lowp vec4 diffuseTexel = texture2D( Texture0, oTexCoord );\n"
	//"   lowp vec4 additiveTexel = texture2D( Texture1, oTexCoord1 ) * oColor;\n"
	"    lowp vec4 additiveTexel = texture2D( Texture1, oTexCoord1 );\n"
	"    lowp vec4 additiveModulated = vec4( additiveTexel.xyz * additiveTexel.a, 0.0 );\n"
	//"    gl_FragColor = min( diffuseTexel + additiveModulated, 1.0 );\n"
	"    gl_FragColor = min( diffuseTexel + additiveModulated, 1.0 ) * oColor;\n"
	"}\n";

// diffuse + diffuse program
// the alpha for the second diffuse is used to composite the color to the first diffuse and
// the alpha of the first diffuse is used to composite to the fragment.
char const* GUIDiffuseCompositeFragmentShaderSrc =
	"uniform sampler2D Texture0;\n"
	"uniform sampler2D Texture1;\n"
	"varying highp vec2 oTexCoord;\n"
	"varying highp vec2 oTexCoord1;\n"
	"varying lowp vec4 oColor;\n"
	"void main()\n"
	"{\n"
	"    lowp vec4 diffuse1Texel = texture2D( Texture0, oTexCoord );\n"
	"    lowp vec4 diffuse2Texel = texture2D( Texture1, oTexCoord1 );\n"
	"    gl_FragColor = vec4( diffuse1Texel.xyz * ( 1.0 - diffuse2Texel.a ) + diffuse2Texel.xyz * diffuse2Texel.a, diffuse1Texel.a ) * oColor;\n"
	"}\n";


//==================================
// ComposeHandle
menuHandle_t ComposeHandle( int const index, vuint32 const id )
{
	vuint64 handle = ( ( (vuint64)id ) << 32ULL ) | (vuint64)index;
	return menuHandle_t( handle );
}

//==================================
// DecomposeHandle
void DecomposeHandle( menuHandle_t const handle, int & index, vuint32 & id )
{
	index = (int)( handle.value() & 0xFFFFFFFF );
	id = (vuint32)( handle.value() >> 32ULL );
}

//==================================
// HandleComponentsAreValid
static bool HandleComponentsAreValid( int const index, vuint32 const id )
{
    if ( id == 0 )
	{
		return false;
	}
	if ( index < 0 )
	{
		return false;
	}
	return true;
}

//==============================================================
// SurfSort
class SurfSort
{
public:
	int64_t		Key;

	bool operator < ( SurfSort const & other ) const
	{
		return Key - other.Key > 0;	// inverted because we want to render furthest-to-closest
	}
};

//==============================================================
// VRMenuMgrLocal
class VRMenuMgrLocal : public OvrVRMenuMgr
{
public:
	static int const	MAX_SUBMITTED	= 256;

	VRMenuMgrLocal();
	virtual	~VRMenuMgrLocal();

	// Initialize the VRMenu system
	virtual void				init();
	// Shutdown the VRMenu syatem
	virtual void				shutdown();

	// creates a new menu object
	virtual menuHandle_t		createObject( VRMenuObjectParms const & parms );
	// Frees a menu object.  If the object is a child of a parent object, this will
	// also remove the child from the parent.
	virtual void				freeObject( menuHandle_t const handle );
	// Returns true if the handle is valid.
	virtual bool				isValid( menuHandle_t const handle ) const;
	// Return the object for a menu handle or NULL if the object does not exist or the
	// handle is invalid;
	virtual VRMenuObject	*	toObject( menuHandle_t const handle ) const;

	// Called once at the very beginning o f the frame before any submissions.
	virtual void				beginFrame();

	// Submits the specified menu object to be renderered
	virtual void				submitForRendering( OvrDebugLines & debugLines, BitmapFont const & font,
                                        BitmapFontSurface & fontSurface, menuHandle_t const handle,
                                        VPosf const & worldPose, VRMenuRenderFlags_t const & flags );

	// Call once per frame before rendering to sort surfaces.
    virtual void				finish( VR4Matrixf const & viewMatrix );

	// Render's all objects that have been submitted on the current frame.
    virtual void				renderSubmitted( VR4Matrixf const & mvp, VR4Matrixf const & viewMatrix ) const;

    virtual VGlShader const *   getGUIGlProgram( eGUIProgramType const programType ) const;

	virtual void				SetShowDebugBounds( bool const b ) { ShowDebugBounds = b; }
	virtual void				SetShowDebugHierarchy( bool const b ) { ShowDebugHierarchy = b; }
	virtual void				SetShowPoses( bool const b ) { ShowPoses = b; }

	static VRMenuMgrLocal &		ToLocal( OvrVRMenuMgr & menuMgr ) { return *(VRMenuMgrLocal*)&menuMgr; }

private:
	//--------------------------------------------------------------
	// private methods
	//--------------------------------------------------------------
	void						CondenseList();
	void						SubmitForRenderingRecursive( OvrDebugLines & debugLines, BitmapFont const & font,
                                        BitmapFontSurface & fontSurface, VRMenuRenderFlags_t const & flags,
                                        VRMenuObjectLocal const * obj, VPosf const & parentModelPose,
                                        V4Vectf const & parentColor, V3Vectf const & parentScale, VBoxf & cullBounds,
                                        SubmittedMenuObject * submitted, int const maxIndices, int & curIndex,
										int const distanceIndex ) const;

	//--------------------------------------------------------------
	// private members
	//--------------------------------------------------------------
	vuint32						CurrentId;		// ever-incrementing object ID (well... up to 4 billion or so :)
	VArray< VRMenuObject* >		ObjectList;		// list of all menu objects
	VArray< int >				FreeList;		// list of free slots in the array
	bool						Initialized;	// true if Init has been called

	SubmittedMenuObject			Submitted[MAX_SUBMITTED];	// all objects that have been submitted for rendering on the current frame
	VArray< SurfSort >			SortKeys;					// sort key consisting of distance from view and submission index
	int							NumSubmitted;				// number of currently submitted menu objects

	VGlShader		            GUIProgramDiffuseOnly;					// has a diffuse only
	VGlShader		            GUIProgramDiffusePlusAdditive;			// has a diffuse and an additive
	VGlShader					GUIProgramDiffuseComposite;				// has a two diffuse maps
	VGlShader		            GUIProgramDiffuseColorRamp;				// has a diffuse and color ramp, and color ramp target is the diffuse
	VGlShader		            GUIProgramDiffuseColorRampTarget;		// has diffuse, color ramp, and a separate color ramp target
	//VGlShader		            GUIProgramDiffusePlusAdditiveColorRamp;
	//VGlShader		            GUIProgramAdditiveColorRamp;

	static bool			ShowDebugBounds;	// true to show the menu items' debug bounds. This is static so that the console command will turn on bounds for all activities.
	static bool			ShowDebugHierarchy;	// true to show the menu items' hierarchy. This is static so that the console command will turn on bounds for all activities.
	static bool			ShowPoses;
};

bool VRMenuMgrLocal::ShowDebugBounds = false;
bool VRMenuMgrLocal::ShowDebugHierarchy = false;
bool VRMenuMgrLocal::ShowPoses = false;

void DebugMenuBounds( void * appPtr, const char * cmd )
{
	int show = 0;
	sscanf( cmd, "%i", &show );
	vAssert( appPtr != NULL );	// something changed / broke in the OvrConsole code if this is NULL
    VRMenuMgrLocal::ToLocal( ( ( App* )appPtr )->vrMenuMgr() ).SetShowDebugBounds( show != 0 );
}

void DebugMenuHierarchy( void * appPtr, const char * cmd )
{
	int show = 0;
	sscanf( cmd, "%i", &show );
	vAssert( appPtr != NULL );	// something changed / broke in the OvrConsole code if this is NULL
    VRMenuMgrLocal::ToLocal( ( ( App* )appPtr )->vrMenuMgr() ).SetShowDebugHierarchy( show != 0 );
}

void DebugMenuPoses( void * appPtr, const char * cmd )
{
	int show = 0;
	sscanf( cmd, "%i", &show );
	vAssert( appPtr != NULL );	// something changed / broke in the OvrConsole code if this is NULL
    VRMenuMgrLocal::ToLocal( ( ( App* )appPtr )->vrMenuMgr() ).SetShowPoses( show != 0 );
}

//==================================
// VRMenuMgrLocal::VRMenuMgrLocal
VRMenuMgrLocal::VRMenuMgrLocal() :
	CurrentId( 0 ),
	Initialized( false ),
	NumSubmitted( 0 )
{
}

//==================================
// VRMenuMgrLocal::~VRMenuMgrLocal
VRMenuMgrLocal::~VRMenuMgrLocal()
{
}

//==================================
// VRMenuMgrLocal::Init
//
// Initialize the VRMenu system
void VRMenuMgrLocal::init()
{
	vInfo("VRMenuMgrLocal::Init");
	if ( Initialized )
	{
        return;
	}

	// diffuse only
	if ( GUIProgramDiffuseOnly.vertexShader == 0 || GUIProgramDiffuseOnly.fragmentShader == 0 )
	{
		GUIProgramDiffuseOnly .initShader( GUIDiffuseOnlyVertexShaderSrc, GUIDiffuseOnlyFragmentShaderSrc );
	}
	// diffuse + additive
	if ( GUIProgramDiffusePlusAdditive.vertexShader == 0 || GUIProgramDiffusePlusAdditive.fragmentShader == 0 )
	{
		GUIProgramDiffusePlusAdditive .initShader( GUITwoTextureColorModulatedShaderSrc, GUIDiffusePlusAdditiveFragmentShaderSrc );
	}
	// diffuse + diffuse
	if ( GUIProgramDiffuseComposite.vertexShader == 0 || GUIProgramDiffuseComposite.fragmentShader == 0 )
	{
		GUIProgramDiffuseComposite .initShader( GUITwoTextureColorModulatedShaderSrc, GUIDiffuseCompositeFragmentShaderSrc );
	}
	// diffuse color ramped
	if ( GUIProgramDiffuseColorRamp.vertexShader == 0 || GUIProgramDiffuseColorRamp.fragmentShader == 0 )
	{
		GUIProgramDiffuseColorRamp .initShader( GUIDiffuseOnlyVertexShaderSrc, GUIColorRampFragmentSrc );
	}
	// diffuse, color ramp, and a specific target for the color ramp
	if ( GUIProgramDiffuseColorRampTarget.vertexShader == 0 || GUIProgramDiffuseColorRampTarget.fragmentShader == 0 )
	{
		GUIProgramDiffuseColorRampTarget .initShader( GUIDiffuseColorRampTargetVertexShaderSrc, GUIColorRampTargetFragmentSrc );
	}

	Initialized = true;
}

//==================================
// VRMenuMgrLocal::Shutdown
//
// Shutdown the VRMenu syatem
void VRMenuMgrLocal::shutdown()
{
	if ( !Initialized )
	{
        return;
	}

	 GUIProgramDiffuseOnly.destroy();
	 GUIProgramDiffusePlusAdditive.destroy();
	 GUIProgramDiffuseComposite.destroy();
	 GUIProgramDiffuseColorRamp.destroy();
	 GUIProgramDiffuseColorRampTarget.destroy();

    Initialized = false;
}

//==================================
// VRMenuMgrLocal::CreateObject
// creates a new menu object
menuHandle_t VRMenuMgrLocal::createObject( VRMenuObjectParms const & parms )
{
	if ( !Initialized )
	{
		vWarn("VRMenuMgrLocal::CreateObject - manager has not been initialized!");
		return menuHandle_t();
	}

	// validate parameters
	if ( parms.Type >= VRMENU_MAX )
	{
		vWarn("VRMenuMgrLocal::CreateObject - Invalid menu object type:" << parms.Type);
		return menuHandle_t();
	}

	// create the handle first so we can enforce setting it be requiring it to be passed to the constructor
	int index = -1;
	if ( FreeList.length() > 0 )
	{
		index = FreeList.back();
        FreeList.removeLast();
	}
	else
	{
		index = ObjectList.length();
	}

	vuint32 id = ++CurrentId;
	menuHandle_t handle = ComposeHandle( index, id );
	//vInfo("VRMenuMgrLocal::CreateObject - handle is" << handle.Get());

	VRMenuObject * obj = new VRMenuObjectLocal( parms, handle );
	if ( obj == NULL )
	{
		vWarn("VRMenuMgrLocal::CreateObject - failed to allocate menu object!");
		vAssert( obj != NULL );	// this would be bad -- but we're likely just going to explode elsewhere
		return menuHandle_t();
	}

	obj->init( parms );

	if ( index == ObjectList.length() )
	{
		// we have to grow the array
		ObjectList.append( obj );
	}
	else
	{
		// insert in existing slot
		vAssert( ObjectList[index] == NULL );
		ObjectList[index ] = obj;
	}

	return handle;
}

//==================================
// VRMenuMgrLocal::FreeObject
// Frees a menu object.  If the object is a child of a parent object, this will
// also remove the child from the parent.
void VRMenuMgrLocal::freeObject( menuHandle_t const handle )
{
	int index;
	vuint32 id;
	DecomposeHandle( handle, index, id );
	if ( !HandleComponentsAreValid( index, id ) )
	{
		return;
	}
	if ( ObjectList[index] == NULL )
	{
		// already freed
		return;
	}

	VRMenuObject * obj = ObjectList[index];
	// remove this object from its parent's child list
	if ( obj->parentHandle().isValid() )
	{
		VRMenuObject * parentObj = toObject( obj->parentHandle() );
		if ( parentObj != NULL )
		{
			parentObj->removeChild( *this, handle );
		}
	}

    // free all of this object's children
    obj->freeChildren( *this );

	delete obj;

	// empty the slot
	ObjectList[index] = NULL;
	// add the index to the free list
	FreeList.append( index );

	CondenseList();
}

//==================================
// VRMenuMgrLocal::CondenseList
// keeps the free list from growing too large when items are removed
void VRMenuMgrLocal::CondenseList()
{
	// we can only condense the array if we have a significant number of items at the end of the array buffer
	// that are empty (because we cannot move an existing object around without changing its handle, too, which
	// would invalidate any existing references to it).
	// This is the difference between the current size and the array capacity.
	int const MIN_FREE = 64;	// very arbitray number
    if ( ObjectList.capacity() - ObjectList.length() < MIN_FREE )
	{
		return;
	}

	// shrink to current size
	ObjectList.resize( ObjectList.length() );

	// create a new free list of just indices < the new size
	VArray< int > newFreeList;
	for ( int i = 0; i < FreeList.length(); ++i )
	{
		if ( FreeList[i] <= ObjectList.length() )
		{
			newFreeList.append( FreeList[i] );
		}
	}
	FreeList = newFreeList;
}

//==================================
// VRMenuMgrLocal::IsValid
bool VRMenuMgrLocal::isValid( menuHandle_t const handle ) const
{
	int index;
	vuint32 id;
	DecomposeHandle( handle, index, id );
	return HandleComponentsAreValid( index, id );
}

//==================================
// VRMenuMgrLocal::ToObject
// Return the object for a menu handle.
VRMenuObject * VRMenuMgrLocal::toObject( menuHandle_t const handle ) const
{
	int index;
	vuint32 id;
	DecomposeHandle( handle, index, id );
    if ( id == 0 )
	{
		return NULL;
	}
	if ( !HandleComponentsAreValid( index, id ) )
	{
		vWarn("VRMenuMgrLocal::ToObject - invalid handle.");
		return NULL;
	}
	if ( index >= ObjectList.length() )
	{
		vWarn("VRMenuMgrLocal::ToObject - index out of range.");
		return NULL;
	}
	VRMenuObject * object = ObjectList[index];
	if ( object == NULL )
	{
		vWarn("VRMenuMgrLocal::ToObject - slot empty.");
		return NULL;	// this can happen if someone is holding onto the handle of an object that's been freed
	}
	if ( object->handle() != handle )
	{
		// if the handle of the object in the slot does not match, then the object the handle refers to was deleted
		// and a new object is in the slot
		vWarn("VRMenuMgrLocal::ToObject - slot mismatch.");
		return NULL;
	}
	return object;
}

//==============================
// VRMenuMgrLocal::BeginFrame
void VRMenuMgrLocal::beginFrame()
{
	NumSubmitted = 0;
}
//==============================
// VRMenuMgrLocal::SubmitForRenderingRecursive
void VRMenuMgrLocal::SubmitForRenderingRecursive( OvrDebugLines & debugLines, BitmapFont const & font,
        BitmapFontSurface & fontSurface, VRMenuRenderFlags_t const & flags, VRMenuObjectLocal const * obj,
        VPosf const & parentModelPose, V4Vectf const & parentColor, V3Vectf const & parentScale,
        VBoxf & cullBounds, SubmittedMenuObject * submitted, int const maxIndices, int & curIndex,
		int const distanceIndex ) const
{
	if ( curIndex >= maxIndices )
	{
		// If this happens we're probably not correctly clearing the submitted surfaces each frame
		// OR we've got a LOT of surfaces.
		vInfo("maxIndices =" << maxIndices << ", curIndex =" << curIndex);
        vAssert(curIndex < maxIndices);
		return;
	}

	// check if this object is hidden
	if ( obj->flags() & VRMENUOBJECT_DONT_RENDER )
	{
		return;
	}

    VPosf const & localPose = obj->localPose();

    VPosf curModelPose;
	curModelPose.Position = parentModelPose.Position + ( parentModelPose.Orientation * parentScale.EntrywiseMultiply( localPose.Position ) );
	curModelPose.Orientation = parentModelPose.Orientation * localPose.Orientation;

    V4Vectf curColor = parentColor * obj->color();
    V3Vectf const & localScale = obj->localScale();
    V3Vectf scale = parentScale.EntrywiseMultiply( localScale );

	vAssert( obj != NULL );

	int submissionIndex = -1;
	VRMenuObjectFlags_t const oFlags = obj->flags();
	if ( obj->type() != VRMENU_CONTAINER )	// containers never render, but their children may
	{
        VPosf const & hilightPose = obj->hilightPose();
        VPosf itemPose( curModelPose.Orientation * hilightPose.Orientation,
                        curModelPose.Position + ( curModelPose.Orientation * parentScale.EntrywiseMultiply( hilightPose.Position ) ) );
        VR4Matrixf poseMat( itemPose.Orientation );
        V3Vectf itemUp = poseMat.GetYBasis();
        V3Vectf itemNormal = poseMat.GetZBasis();
		curModelPose = itemPose;	// so children like the slider bar caret use our hilight offset and don't end up clipping behind us!
		VRMenuRenderFlags_t rFlags = flags;
		if ( oFlags & VRMENUOBJECT_FLAG_POLYGON_OFFSET )
		{
			rFlags |= VRMENU_RENDER_POLYGON_OFFSET;
		}
		if ( oFlags & VRMENUOBJECT_FLAG_NO_DEPTH )
		{
			rFlags |= VRMENU_RENDER_NO_DEPTH;
		}
		if ( oFlags & VRMENUOBJECT_FLAG_BILLBOARD )
		{
			rFlags |= VRMENU_RENDER_BILLBOARD;
		}

		if ( ShowPoses )
		{
            VR4Matrixf const poseMat( itemPose );
			debugLines.AddLine( itemPose.Position, itemPose.Position + poseMat.GetXBasis() * 0.05f,
                    V4Vectf( 0.0f, 1.0f, 0.0f, 1.0f ), V4Vectf( 0.0f, 1.0f, 0.0f, 1.0f ), 0, false );
			debugLines.AddLine( itemPose.Position, itemPose.Position + poseMat.GetYBasis() * 0.05f,
                    V4Vectf( 1.0f, 0.0f, 0.0f, 1.0f ), V4Vectf( 1.0f, 0.0f, 0.0f, 1.0f ), 0, false );
			debugLines.AddLine( itemPose.Position, itemPose.Position + poseMat.GetZBasis() * 0.05f,
                    V4Vectf( 0.0f, 0.0f, 1.0f, 1.0f ), V4Vectf( 0.0f, 0.0f, 1.0f, 1.0f ), 0, false );
		}

		// the menu object may have zero or more renderable surfaces (if 0, it may draw only text)
		submissionIndex = curIndex;
		VArray< VRMenuSurface > const & surfaces = obj->surfaces();
		for ( int i = 0; i < surfaces.length(); ++i )
		{
			VRMenuSurface const & surf = surfaces[i];
			if ( surf.isRenderable() )
			{
				SubmittedMenuObject & sub = submitted[curIndex];
				sub.surfaceIndex = i;
				sub.distanceIndex = distanceIndex >= 0 ? distanceIndex : curIndex;
				sub.pose = itemPose;
				sub.scale = scale;
				sub.flags = rFlags;
				sub.colorTableOffset = obj->colorTableOffset();
				sub.skipAdditivePass = !obj->isHilighted();
				sub.handle = obj->handle();
				// modulate surface color with parent's current color
				sub.color = surf.color() * curColor;
				sub.offsets = surf.anchorOffsets();
				sub.fadeDirection = obj->fadeDirection();
#if defined( OVR_BUILD_DEBUG )
				sub.SurfaceName = surf.name();
#endif
				curIndex++;
			}
		}

        VString const & text = obj->text();
        if ( ( oFlags & VRMENUOBJECT_DONT_RENDER_TEXT ) == 0 && text.length() > 0 )
		{
            VPosf const & textLocalPose = obj->textLocalPose();
            VPosf curTextPose;
// FIXME: this doesn't mirror the scale / rotation order for the localPose above
//            curTextPose.Position = itemPose.Position + ( itemPose.Orientation * scale.EntrywiseMultiply( textLocalPose.Position ) );
            curTextPose.Position = itemPose.Position + ( itemPose.Orientation * textLocalPose.Position * scale );
            curTextPose.Orientation = textLocalPose.Orientation * itemPose.Orientation;
            V3Vectf textNormal = curTextPose.Orientation * V3Vectf( 0.0f, 0.0f, 1.0f );
            V3Vectf position = curTextPose.Position + textNormal * 0.001f; // this is simply to prevent z-fighting right now
            V3Vectf textScale = scale * obj->textLocalScale();

            V4Vectf textColor = obj->textColor();
            // Apply parent's alpha influence
            textColor.w *= parentColor.w;
			VRMenuFontParms const & fp = obj->fontParms();
			fontParms_t fontParms;
			fontParms.AlignHoriz = fp.AlignHoriz;
			fontParms.AlignVert = fp.AlignVert;
			fontParms.Billboard = fp.Billboard;
			fontParms.TrackRoll = fp.TrackRoll;
			fontParms.ColorCenter = fp.ColorCenter;
			fontParms.AlphaCenter = fp.AlphaCenter;

			fontSurface.DrawText3D( font, fontParms, position, itemNormal, itemUp,
                    textScale.x * fp.Scale, textColor, text);

			if ( ShowDebugBounds )
			{
				// this shows a ruler for the wrap width when rendering text
                V3Vectf xofs( 0.1f, 0.0f, 0.0f );
				debugLines.AddLine( position - xofs, position + xofs,
                    V4Vectf( 0.0f, 1.0f, 0.0f, 1.0f ), V4Vectf( 1.0f, 0.0f, 0.0f, 1.0f ), 0, false );
                V3Vectf yofs( 0.0f, 0.1f, 0.0f );
				debugLines.AddLine( position - yofs, position + yofs,
                    V4Vectf( 0.0f, 1.0f, 0.0f, 1.0f ), V4Vectf( 1.0f, 0.0f, 0.0f, 1.0f ), 0, false );
                V3Vectf zofs( 0.0f, 0.0f, 0.1f );
				debugLines.AddLine( position - zofs, position + zofs,
                    V4Vectf( 0.0f, 1.0f, 0.0f, 1.0f ), V4Vectf( 1.0f, 0.0f, 0.0f, 1.0f ), 0, false );
			}
		}
        vInfo("AddPoint for" << text);
		//GetDebugLines().AddPoint( curModelPose.Position, 0.05f, 1, true );
	}

    cullBounds = obj->getTextLocalBounds( font ) * parentScale;

	// submit all children
    if ( obj->m_children.length() > 0 )
    {
		// If this object has the render hierarchy order flag, then it and all its children should
		// be depth sorted based on this object's distance + the inverse of the submission index.
		// (inverted because we want a higher submission index to render after a lower submission index)
		int di = distanceIndex;
		if ( di < 0 && ( oFlags & VRMenuObjectFlags_t( VRMENUOBJECT_RENDER_HIERARCHY_ORDER ) ) )
		{
			di = submissionIndex;
		}
	    for ( int i = 0; i < obj->m_children.length(); ++i )
	    {
		    menuHandle_t childHandle = obj->m_children[i];
		    VRMenuObjectLocal const * child = static_cast< VRMenuObjectLocal const * >( toObject( childHandle ) );
		    if ( child == NULL )
		    {
			    continue;
		    }

            VBoxf childCullBounds;
		    SubmitForRenderingRecursive( debugLines, font, fontSurface, flags, child, curModelPose, curColor, scale,
                    childCullBounds, submitted, maxIndices, curIndex, di );

            VPosf pose = child->localPose();
		    pose.Position = pose.Position * scale;
            childCullBounds = VBoxf::Transform( pose, childCullBounds );
            cullBounds = VBoxf::Union( cullBounds, childCullBounds );
	    }
    }

    obj->setCullBounds( cullBounds );


	if ( ShowDebugBounds )
	{
		OvrCollisionPrimitive const * cp = obj->collisionPrimitive();
		if ( cp != NULL )
		{
			cp->debugRender( debugLines, curModelPose );
		}
		{
			// for debug drawing, put the cull bounds in world space
            //LogBounds( obj->GetText().toCString(), "Transformed CullBounds", myCullBounds );
            debugLines.AddBounds( curModelPose, obj->cullBounds(), V4Vectf( 0.0f, 1.0f, 1.0f, 1.0f ) );
		}
		{
            VBoxf localBounds = obj->getTextLocalBounds( font ) * parentScale;
            //LogBounds( obj->GetText().toCString(), "localBounds", localBounds );
            debugLines.AddBounds( curModelPose, localBounds, V4Vectf( 1.0f, 0.0f, 0.0f, 1.0f ) );
            VBoxf textLocalBounds = obj->setTextLocalBounds( font );
            VPosf hilightPose = obj->hilightPose();
            textLocalBounds = VBoxf::Transform( VPosf( hilightPose.Orientation, hilightPose.Position * scale ), textLocalBounds );
            debugLines.AddBounds( curModelPose, textLocalBounds * parentScale, V4Vectf( 1.0f, 1.0f, 0.0f, 1.0f ) );
		}
	}

	// draw the hierarchy
	if ( ShowDebugHierarchy )
	{
		fontParms_t fp;
		fp.AlignHoriz = HORIZONTAL_CENTER;
		fp.AlignVert = VERTICAL_CENTER;
		fp.Billboard = true;
#if 0
		VRMenuObject const * parent = ToObject( obj->GetParentHandle() );
		if ( parent != NULL )
		{
            V3Vectf itemUp = curModelPose.Orientation * V3Vectf( 0.0f, 1.0f, 0.0f );
            V3Vectf itemNormal = curModelPose.Orientation * V3Vectf( 0.0f, 0.0f, 1.0f );
			fontSurface.DrawTextBillboarded3D( font, fp, curModelPose.Position, itemNormal, itemUp,
                    0.5f, V4Vectf( 1.0f, 0.0f, 1.0f, 1.0f ), obj->GetSurfaces()[0] ); //parent->GetText().toCString() );
		}
#endif
        debugLines.AddLine( parentModelPose.Position, curModelPose.Position, V4Vectf( 1.0f, 0.0f, 0.0f, 1.0f ), V4Vectf( 0.0f, 0.0f, 1.0f, 1.0f ), 5, false );
		if ( obj->surfaces().length() > 0 )
		{
			fontSurface.DrawTextBillboarded3D( font, fp, curModelPose.Position, 0.5f,
                    V4Vectf( 0.8f, 0.8f, 0.8f, 1.0f ), obj->surfaces()[0].name());
		}
	}
}

//==============================
// VRMenuMgrLocal::SubmitForRendering
// Submits the specified menu object and it's children
void VRMenuMgrLocal::submitForRendering( OvrDebugLines & debugLines, BitmapFont const & font,
        BitmapFontSurface & fontSurface, menuHandle_t const handle, VPosf const & worldPose,
        VRMenuRenderFlags_t const & flags )
{
	//vInfo("VRMenuMgrLocal::SubmitForRendering");
	if ( NumSubmitted >= MAX_SUBMITTED )
	{
		vWarn("Too many menu objects submitted!");
		return;
	}
	VRMenuObjectLocal * obj = static_cast< VRMenuObjectLocal* >( toObject( handle ) );
	if ( obj == NULL )
	{
		return;
	}

    VBoxf cullBounds;
    SubmitForRenderingRecursive( debugLines, font, fontSurface, flags, obj, worldPose, V4Vectf( 1.0f ),
            V3Vectf( 1.0f ), cullBounds, Submitted, MAX_SUBMITTED, NumSubmitted, -1 );
}

//==============================
// VRMenuMgrLocal::Finish
void VRMenuMgrLocal::finish( VR4Matrixf const & viewMatrix )
{
	if ( NumSubmitted == 0 )
	{
		return;
	}

    VR4Matrixf invViewMatrix = viewMatrix.Inverted(); // if the view is never scaled or sheared we could use Transposed() here instead
    V3Vectf viewPos = invViewMatrix.GetTranslation();

	// sort surfaces
	SortKeys.resize( NumSubmitted );
	for ( int i = 0; i < NumSubmitted; ++i )
	{
		// The sort key is a combination of the distance squared, reinterpreted as an integer, and the submission index.
		// This sorts on distance while still allowing submission order to contribute in the equal case.
		// The DistanceIndex is used to force a submitted object to use some other object's distance instead of its own,
		// allowing a group of objects to sort against all other object's based on a single distance. Objects uising the
		// same DistanceIndex will then be sorted against each other based only on their submission index.
		float const distSq = ( Submitted[Submitted[i].distanceIndex].pose.Position - viewPos ).LengthSq();
		int64_t sortKey = *reinterpret_cast< unsigned const* >( &distSq );
		SortKeys[i].Key = ( sortKey << 32ULL ) | ( NumSubmitted - i );	// invert i because we want items submitted sooner to be considered "further away"
	}

	std::sort(SortKeys.begin(), SortKeys.end());

}

//==============================
// VRMenuMgrLocal::RenderSubmitted
void VRMenuMgrLocal::renderSubmitted( VR4Matrixf const & worldMVP, VR4Matrixf const & viewMatrix ) const
{
	if ( NumSubmitted == 0 )
	{
		return;
	}


    VEglDriver::logErrorsEnum( "VRMenuMgrLocal::RenderSubmitted - pre" );

	//vInfo("VRMenuMgrLocal::RenderSubmitted");
    VR4Matrixf invViewMatrix = viewMatrix.Inverted();
    V3Vectf viewPos = invViewMatrix.GetTranslation();

	//vInfo("VRMenuMgrLocal::RenderSubmitted - rendering" << NumSubmitted << "objects");
	bool depthEnabled = true;
	glEnable( GL_DEPTH_TEST );
	bool polygonOffset = false;
	glDisable( GL_POLYGON_OFFSET_FILL );
	glPolygonOffset( 0.0f, -10.0f );

	for ( int i = 0; i < NumSubmitted; ++i )
	{
		int idx = abs( ( SortKeys[i].Key & 0xFFFFFFFF ) - NumSubmitted );
#if 0
		int di = SortKeys[i].Key >> 32ULL;
		float const df = *((float*)(&di ));
        vInfo("Surface '" << Submitted[idx].SurfaceName << "', sk =" << SortKeys[i].Key << ", df =" << df << ", idx =" << idx);
#endif
		SubmittedMenuObject const & cur = Submitted[idx];

		VRMenuObjectLocal const * obj = static_cast< VRMenuObjectLocal const * >( toObject( cur.handle ) );
		if ( obj != NULL )
		{
			// TODO: this could be made into a generic template for any glEnable() flag
			if ( cur.flags & VRMENU_RENDER_NO_DEPTH )
			{
				if ( depthEnabled )
				{
					glDisable( GL_DEPTH_TEST );
					depthEnabled = false;
				}
			}
			else
			{
				if ( !depthEnabled )
				{
					glEnable( GL_DEPTH_TEST );
					depthEnabled = true;
				}
			}
			if ( cur.flags & VRMENU_RENDER_POLYGON_OFFSET )
			{
				if ( !polygonOffset )
				{
					glEnable( GL_POLYGON_OFFSET_FILL );
					polygonOffset = true;
				}
			}
			else
			{
				if ( polygonOffset )
				{
					glDisable( GL_POLYGON_OFFSET_FILL );
					polygonOffset = false;
				}
			}
            V3Vectf translation( cur.pose.Position.x + cur.offsets.x, cur.pose.Position.y + cur.offsets.y, cur.pose.Position.z );

            VR4Matrixf transform( cur.pose.Orientation );
			if ( cur.flags & VRMENU_RENDER_BILLBOARD )
			{
                V3Vectf normal = viewPos - cur.pose.Position;
                V3Vectf up( 0.0f, 1.0f, 0.0f );
				float length = normal.Length();
                if ( length > VConstantsf::SmallestNonDenormal )
				{
					normal.Normalize();
                    if ( normal.Dot( up ) > VConstantsf::SmallestNonDenormal )
					{
                        transform = VR4Matrixf::CreateFromBasisVectors( normal, V3Vectf( 0.0f, 1.0f, 0.0f ) );
					}
				}
			}

            VR4Matrixf scaleMatrix;
			scaleMatrix.M[0][0] = cur.scale.x;
			scaleMatrix.M[1][1] = cur.scale.y;
			scaleMatrix.M[2][2] = cur.scale.z;

			transform *= scaleMatrix;
			transform.SetTranslation( translation );

            VR4Matrixf mvp = transform.Transposed() * worldMVP;
			obj->renderSurface( *this, mvp, cur );
		}
	}

	glDisable( GL_POLYGON_OFFSET_FILL );

    VEglDriver::logErrorsEnum( "VRMenuMgrLocal::RenderSubmitted - post" );
}

//==============================
// VRMenuMgrLocal::GetGUIGlProgram
VGlShader const * VRMenuMgrLocal::getGUIGlProgram( eGUIProgramType const programType ) const
{
    switch( programType )
    {
        case PROGRAM_DIFFUSE_ONLY:
            return &GUIProgramDiffuseOnly;
        case PROGRAM_ADDITIVE_ONLY:
            return &GUIProgramDiffuseOnly;
        case PROGRAM_DIFFUSE_PLUS_ADDITIVE:
            return &GUIProgramDiffusePlusAdditive;
        case PROGRAM_DIFFUSE_COMPOSITE:
            return &GUIProgramDiffuseComposite;
        case PROGRAM_DIFFUSE_COLOR_RAMP:
            return &GUIProgramDiffuseColorRamp;
        case PROGRAM_DIFFUSE_COLOR_RAMP_TARGET:
            return &GUIProgramDiffuseColorRampTarget;
        default:
            vFatal("Invalid gui program type");
            break;
    }
    return NULL;
}

//==============================
// OvrVRMenuMgr::Create
OvrVRMenuMgr * OvrVRMenuMgr::Create()
{
    VRMenuMgrLocal * mgr = new VRMenuMgrLocal;
    return mgr;
}

//==============================
// OvrVRMenuMgr::Free
void OvrVRMenuMgr::Free( OvrVRMenuMgr * & mgr )
{
    if ( mgr != NULL )
    {
        mgr->shutdown();
        delete mgr;
        mgr = NULL;
    }
}

} // namespace NervGear
