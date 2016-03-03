/************************************************************************************

Filename    :   VRMenuObject.h
Content     :   Menuing system for VR apps.
Created     :   May 23, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#if !defined( OVR_VRMenuObject_h )
#define OVR_VRMenuObject_h

#include "Types.h"
#include "VMath.h"
#include "Array.h"
#include "VString.h"
#include "TypesafeNumber.h"
#include "BitFlags.h"
#include "Android/GlUtils.h"	// GLuint
#include "Android/LogUtils.h"
#include "CollisionPrimitive.h"
#include "BitmapFont.h" // HorizontalJustification & VerticalJustification

NV_NAMESPACE_BEGIN

class App;
class OvrVRMenuMgr;

enum eVRMenuObjectType
{
	VRMENU_CONTAINER,	// a container for other controls
	VRMENU_STATIC,		// non-interactable item, may have text
	VRMENU_BUTTON,		// "clickable"
	VRMENU_MAX			// invalid
};

enum eVRMenuSurfaceImage
{
	VRMENUSURFACE_IMAGE_0,
	VRMENUSURFACE_IMAGE_1,
	VRMENUSURFACE_IMAGE_2,
	VRMENUSURFACE_IMAGE_MAX
};

enum eSurfaceTextureType
{
	SURFACE_TEXTURE_DIFFUSE,
	SURFACE_TEXTURE_ADDITIVE,
	SURFACE_TEXTURE_COLOR_RAMP,
	SURFACE_TEXTURE_COLOR_RAMP_TARGET,
	SURFACE_TEXTURE_MAX	// also use to indicate an uninitialized type
};

enum eVRMenuObjectFlags
{
	VRMENUOBJECT_FLAG_NO_FOCUS_GAINED,	// object will never get focus gained event
	VRMENUOBJECT_DONT_HIT_ALL,			// do not collide with object in hit testing
    VRMENUOBJECT_DONT_HIT_TEXT,         // do not collide with text in hit testing
    VRMENUOBJECT_HIT_ONLY_BOUNDS,       // only collide against the bounds, not the collision surface
    VRMENUOBJECT_BOUND_ALL,             // for hit testing, merge all bounds into a single AABB
	VRMENUOBJECT_FLAG_POLYGON_OFFSET,	// render with polygon offset
	VRMENUOBJECT_FLAG_NO_DEPTH,			// render without depth test
	VRMENUOBJECT_DONT_RENDER,			// don't render this object
	VRMENUOBJECT_DONT_RENDER_TEXT,		// don't render this object's text (useful for naming an object just for debugging)
	VRMENUOBJECT_NO_GAZE_HILIGHT,		// when hit this object won't change the cursor hilight state
	VRMENUOBJECT_RENDER_HIERARCHY_ORDER,	// this object and all its children will use this object's position for depth sorting, and sort vs. each other by submission index (i.e. parent's render first)
	VRMENUOBJECT_FLAG_BILLBOARD			// always face view plane normal
};

typedef BitFlagsT< eVRMenuObjectFlags > VRMenuObjectFlags_t;

enum eVRMenuObjectInitFlags
{
	VRMENUOBJECT_INIT_FORCE_POSITION	// use the position in the parms instead of an auto-calculated position
};

typedef BitFlagsT< eVRMenuObjectInitFlags > VRMenuObjectInitFlags_t;

enum eVRMenuId
{
	INVALID_MENU_ID = INT_MIN
};
typedef TypesafeNumberT< long long, eVRMenuId, INVALID_MENU_ID > VRMenuId_t;

// menu object handles
enum eMenuIdType
{
	INVALID_MENU_OBJECT_ID = 0
};
typedef TypesafeNumberT< uint64_t, eMenuIdType, INVALID_MENU_OBJECT_ID >	menuHandle_t;

// menu render flags
enum eVRMenuRenderFlags
{
	VRMENU_RENDER_NO_DEPTH,
	VRMENU_RENDER_NO_FONT_OUTLINE,
	VRMENU_RENDER_POLYGON_OFFSET,
	VRMENU_RENDER_BILLBOARD
};
typedef BitFlagsT< eVRMenuRenderFlags > VRMenuRenderFlags_t;

class VRMenuComponent;
class VRMenuComponent_OnFocusGained;
class VRMenuComponent_OnFocusLost;
class VRMenuComponent_OnDown;
class VRMenuComponent_OnUp;
class VRMenuComponent_OnSubmitForRendering;
class VRMenuComponent_OnRender;
class VRMenuComponent_OnTouchRelative;
class BitmapFont;
struct fontParms_t;

// border indices
enum eVRMenuBorder
{
	BORDER_LEFT,
	BORDER_BOTTOM,
	BORDER_RIGHT,
	BORDER_TOP
};

//==============================================================
// VRMenuSurfaceParms
class VRMenuSurfaceParms
{
public:
	VRMenuSurfaceParms() :
		SurfaceName( "" ),
		ImageNames(),
		Border( 0.0f, 0.0f, 0.0f, 0.0f ),
		Dims( 0.0f, 0.0f )

	{
		InitSurfaceTextureTypes();
	}

	explicit VRMenuSurfaceParms( char const * surfaceName ) :
		SurfaceName( surfaceName ),
		ImageNames(),
		Contents( CONTENT_SOLID ),
		Anchors( 0.5f, 0.5f ),
		Border( 0.0f, 0.0f, 0.0f, 0.0f ),
		Dims( 0.0f, 0.0f )

	{
		InitSurfaceTextureTypes();
	}
	VRMenuSurfaceParms( char const * surfaceName,
			char const * imageName0,
			eSurfaceTextureType textureType0,
			char const * imageName1,
			eSurfaceTextureType textureType1,
			char const * imageName2,
			eSurfaceTextureType textureType2 ) :
		SurfaceName( surfaceName ),
		ImageNames(),
		Contents( CONTENT_SOLID ),
		Anchors( 0.5f, 0.5f ),
		Border( 0.0f, 0.0f, 0.0f, 0.0f ),
		Dims( 0.0f, 0.0f )

	{
		InitSurfaceTextureTypes();
		ImageNames[VRMENUSURFACE_IMAGE_0] = imageName0;
		TextureTypes[VRMENUSURFACE_IMAGE_0] = textureType0;

		ImageNames[VRMENUSURFACE_IMAGE_1] = imageName1;
		TextureTypes[VRMENUSURFACE_IMAGE_1] = textureType1;

		ImageNames[VRMENUSURFACE_IMAGE_2] = imageName2;
		TextureTypes[VRMENUSURFACE_IMAGE_2] = textureType2;
	}
	VRMenuSurfaceParms( char const * surfaceName,
			char const * imageName0,
			eSurfaceTextureType textureType0,
			char const * imageName1,
			eSurfaceTextureType textureType1,
			char const * imageName2,
			eSurfaceTextureType textureType2,
			Vector2f const & anchors ) :
		SurfaceName( surfaceName ),
		ImageNames(),
		Contents( CONTENT_SOLID ),
		Anchors( anchors ),
		Border( 0.0f, 0.0f, 0.0f, 0.0f ),
		Dims( 0.0f, 0.0f )

	{
		InitSurfaceTextureTypes();
		ImageNames[VRMENUSURFACE_IMAGE_0] = imageName0;
		TextureTypes[VRMENUSURFACE_IMAGE_0] = textureType0;

		ImageNames[VRMENUSURFACE_IMAGE_1] = imageName1;
		TextureTypes[VRMENUSURFACE_IMAGE_1] = textureType1;

		ImageNames[VRMENUSURFACE_IMAGE_2] = imageName2;
		TextureTypes[VRMENUSURFACE_IMAGE_2] = textureType2;
	}
	VRMenuSurfaceParms( char const * surfaceName,
			char const * imageNames[],
			eSurfaceTextureType textureTypes[] ) :
		SurfaceName( surfaceName ),
		ImageNames(),
		TextureTypes(),
		Contents( CONTENT_SOLID ),
		Anchors( 0.5f, 0.5f ),
		Border( 0.0f, 0.0f, 0.0f, 0.0f ),
		Dims( 0.0f, 0.0f )

	{
		InitSurfaceTextureTypes();
		for ( int i = 0; i < VRMENUSURFACE_IMAGE_MAX && imageNames[i] != NULL; ++i )
		{
			ImageNames[i] = imageNames[i];
			TextureTypes[i] = textureTypes[i];
		}
	}
	VRMenuSurfaceParms( char const * surfaceName,
			GLuint imageTexId0,
			int width0,
			int height0,
			eSurfaceTextureType textureType0,
			GLuint imageTexId1,
			int width1,
			int height1,
			eSurfaceTextureType textureType1,
			GLuint imageTexId2,
			int width2,
			int height2,
			eSurfaceTextureType textureType2 ) :
		SurfaceName( surfaceName ),
		ImageNames(),
		Contents( CONTENT_SOLID ),
		Anchors( 0.5f, 0.5f ),
		Border( 0.0f, 0.0f, 0.0f, 0.0f ),
		Dims( 0.0f, 0.0f )

	{
		InitSurfaceTextureTypes();
		ImageTexId[VRMENUSURFACE_IMAGE_0] = imageTexId0;
		TextureTypes[VRMENUSURFACE_IMAGE_0] = textureType0;
		ImageWidth[VRMENUSURFACE_IMAGE_0] = width0;
		ImageHeight[VRMENUSURFACE_IMAGE_0] = height0;

		ImageTexId[VRMENUSURFACE_IMAGE_1] = imageTexId1;
		TextureTypes[VRMENUSURFACE_IMAGE_1] = textureType1;
		ImageWidth[VRMENUSURFACE_IMAGE_1] = width1;
		ImageHeight[VRMENUSURFACE_IMAGE_1] = height1;

		ImageTexId[VRMENUSURFACE_IMAGE_2] = imageTexId2;
		TextureTypes[VRMENUSURFACE_IMAGE_2] = textureType2;
		ImageWidth[VRMENUSURFACE_IMAGE_2] = width2;
		ImageHeight[VRMENUSURFACE_IMAGE_2] = height2;
    }

    VString              SurfaceName;		// for debugging only
	VString              ImageNames[VRMENUSURFACE_IMAGE_MAX];
	GLuint				ImageTexId[VRMENUSURFACE_IMAGE_MAX];
	short				ImageWidth[VRMENUSURFACE_IMAGE_MAX];
	short				ImageHeight[VRMENUSURFACE_IMAGE_MAX];
	eSurfaceTextureType	TextureTypes[VRMENUSURFACE_IMAGE_MAX];
	ContentFlags_t		Contents;
	Vector2f			Anchors;
	Vector4f			Border;						// if set to non-zero, sets the border on a sliced sprite
	Vector2f			Dims;						// if set to zero, use texture size, non-zero sets dims to absolute size

private:
	void InitSurfaceTextureTypes()
	{
		for ( int i = 0; i < VRMENUSURFACE_IMAGE_MAX; ++i )
		{
			TextureTypes[i] = SURFACE_TEXTURE_MAX;
			ImageTexId[i] = 0;
			ImageWidth[i] = 0;
			ImageHeight[i] = 0;
        }
    }
};

//==============================================================
// VRMenuFontParms
class VRMenuFontParms
{
public:
	VRMenuFontParms() :
		AlignHoriz( HORIZONTAL_CENTER ),
		AlignVert( VERTICAL_CENTER ),
		Billboard( false ),
		TrackRoll( false ),
		Outline( true ),
		ColorCenter( 0.0f ),
		AlphaCenter( 0.5f ),
		Scale( 1.0f )
    {
    }

	VRMenuFontParms( bool const centerHoriz,
			bool const centerVert,
			bool const billboard,
			bool const trackRoll,
			bool const outline,
			float const colorCenter,
			float const alphaCenter,
			float const scale ) :
		AlignHoriz( centerHoriz ? HORIZONTAL_CENTER : HORIZONTAL_LEFT ),
		AlignVert( centerVert ? VERTICAL_CENTER : VERTICAL_BASELINE ),
		Billboard( billboard ),
		TrackRoll( trackRoll ),
		Outline( outline ),
		ColorCenter( colorCenter ),
		AlphaCenter( alphaCenter ),
		Scale( scale )
	{
	}

	VRMenuFontParms( bool const centerHoriz,
			bool const centerVert,
			bool const billboard,
			bool const trackRoll,
			bool const outline,
			float const scale ) :
		AlignHoriz( centerHoriz ? HORIZONTAL_CENTER : HORIZONTAL_LEFT ),
		AlignVert( centerVert ? VERTICAL_CENTER : VERTICAL_BASELINE ),
		Billboard( billboard ),
		TrackRoll( trackRoll ),
		Outline( outline ),
		ColorCenter( outline ? 0.5f : 0.0f ),
		AlphaCenter( outline ? 0.425f : 0.5f ),
		Scale( scale )
	{
	}

	VRMenuFontParms( HorizontalJustification const alignHoriz,
			VerticalJustification const alignVert,
			bool const billboard,
			bool const trackRoll,
			bool const outline,
			float const colorCenter,
			float const alphaCenter,
			float const scale ) :
		AlignHoriz( alignHoriz ),
		AlignVert( alignVert ),
		Billboard( billboard ),
		TrackRoll( trackRoll ),
		Outline( outline ),
		ColorCenter( colorCenter ),
		AlphaCenter( alphaCenter ),
		Scale( scale )
	{
	}

	VRMenuFontParms( HorizontalJustification const alignHoriz,
			VerticalJustification const alignVert,
			bool const billboard,
			bool const trackRoll,
			bool const outline,
			float const scale ) :
		AlignHoriz( alignHoriz ),
		AlignVert( alignVert ),
		Billboard( billboard ),
		TrackRoll( trackRoll ),
		Outline( outline ),
		ColorCenter( outline ? 0.5f : 0.0f ),
		AlphaCenter( outline ? 0.425f : 0.5f ),
		Scale( scale )
	{
	}

	HorizontalJustification	AlignHoriz;    	// horizontal justification around the specified x coordinate
    VerticalJustification	AlignVert;     	// vertical justification around the specified y coordinate
	bool	Billboard;		// true to always face the camera
	bool	TrackRoll;		// when billboarding, track with camera roll
	bool	Outline;		// true if font should rended with an outline
	float	ColorCenter;	// center value for color -- used for outlining
	float	AlphaCenter;	// center value for alpha -- used for outlined or non-outlined

	float	Scale;			// scale factor for the text
};

//==============================================================
// VRMenumObjectParms
//
// Parms passed to the VRMenuObject factory to create a VRMenuObject
class VRMenuObjectParms
{
public:
	VRMenuObjectParms(	eVRMenuObjectType const type,
			Array< VRMenuComponent* > const & components,
			VRMenuSurfaceParms const & surfaceParms,
			char const * text,
			Posef const & localPose,
			Vector3f const & localScale,
			VRMenuFontParms const & fontParms,
			VRMenuId_t const id,
			VRMenuObjectFlags_t const flags,
			VRMenuObjectInitFlags_t const initFlags ) :
		Type( type ),
		Flags( flags ),
		InitFlags( initFlags ),
		Components( components ),
		SurfaceParms(),
		Text( text ),
		LocalPose( localPose ),
		LocalScale( localScale ),
        TextLocalPose( Quatf(), Vector3f( 0.0f ) ),
        TextLocalScale( 1.0f ),
		FontParms( fontParms ),
        Color( 1.0f ),
        TextColor( 1.0f ),
		Id( id ),
		Contents( CONTENT_SOLID )
	{
		SurfaceParms.append( surfaceParms );
	}

    // same as above with additional text local parms
	VRMenuObjectParms(	eVRMenuObjectType const type,
			Array< VRMenuComponent* > const & components,
			VRMenuSurfaceParms const & surfaceParms,
			char const * text,
			Posef const & localPose,
			Vector3f const & localScale,
            Posef const & textLocalPose,
            Vector3f const & textLocalScale,
			VRMenuFontParms const & fontParms,
			VRMenuId_t const id,
			VRMenuObjectFlags_t const flags,
			VRMenuObjectInitFlags_t const initFlags ) :
		Type( type ),
		Flags( flags ),
		InitFlags( initFlags ),
		Components( components ),
		SurfaceParms(),
		Text( text ),
		LocalPose( localPose ),
		LocalScale( localScale ),
        TextLocalPose( textLocalPose ),
        TextLocalScale( textLocalScale ),
		FontParms( fontParms ),
        Color( 1.0f ),
        TextColor( 1.0f ),
		Id( id ),
		Contents( CONTENT_SOLID )
    {
		SurfaceParms.append( surfaceParms );
    }

    // takes an array of surface parms
	VRMenuObjectParms(	eVRMenuObjectType const type,
			Array< VRMenuComponent* > const & components,
			Array< VRMenuSurfaceParms > const & surfaceParms,
			char const * text,
			Posef const & localPose,
			Vector3f const & localScale,
            Posef const & textLocalPose,
            Vector3f const & textLocalScale,
			VRMenuFontParms const & fontParms,
			VRMenuId_t const id,
			VRMenuObjectFlags_t const flags,
			VRMenuObjectInitFlags_t const initFlags ) :
		Type( type ),
		Flags( flags ),
		InitFlags( initFlags ),
		Components( components ),
		SurfaceParms( surfaceParms ),
		Text( text ),
		LocalPose( localPose ),
		LocalScale( localScale ),
        TextLocalPose( textLocalPose ),
        TextLocalScale( textLocalScale ),
		FontParms( fontParms ),
        Color( 1.0f ),
        TextColor( 1.0f ),
		Id( id ),
		Contents( CONTENT_SOLID )
    {
    }

	eVRMenuObjectType		    Type;							// type of menu object
	VRMenuObjectFlags_t		    Flags;							// bit flags
	VRMenuObjectInitFlags_t	    InitFlags;						// intialization-only flags (not referenced beyond object initialization)
    Array< VRMenuComponent* >   Components;						// list of pointers to components
	Array< VRMenuSurfaceParms >	SurfaceParms;					// list of surface parameters for the object. Each parm will result in one surface, and surfaces will render in the same order as this list.
	VString      			    Text;							// text to display on this object (if any)
	Posef					    LocalPose;						// local-space position and orientation
	Vector3f				    LocalScale;						// local-space scale
    Posef                       TextLocalPose;                  // offset of text, local to surface
    Vector3f                    TextLocalScale;                 // scale of text, local to surface
	VRMenuFontParms			    FontParms;						// parameters for rendering the object's text
    Vector4f                    Color;                          // color modulation for surfaces
    Vector4f                    TextColor;                      // color of text
	VRMenuId_t				    Id;								// user identifier, so the client using a menu can find a particular object
	VRMenuId_t					ParentId;						// id of object that should be made this object's parent.
	ContentFlags_t				Contents;						// collision contents for the menu object
};

//==============================================================
// HitTestResult
// returns the results of a hit test vs menu objects
class HitTestResult : public OvrCollisionResult
{
public:
	HitTestResult &	operator=( OvrCollisionResult & rhs )
	{
		this->HitHandle.Release();
		this->RayStart = Vector3f::ZERO;
		this->RayDir = Vector3f::ZERO;
		this->t = rhs.t;
		this->uv = rhs.uv;
		return *this;
	}

	menuHandle_t	HitHandle;
	Vector3f		RayStart;
	Vector3f		RayDir;
};

//==============================================================
// VRMenuObject
class VRMenuObject
{
public:
	static float const	TEXELS_PER_METER;
	static float const	DEFAULT_TEXEL_SCALE;

	friend class VRMenuMgr;
	friend class VRMenuMgrLocal;

	// Initialize the object after creation
	virtual void				init( VRMenuObjectParms const & parms ) = 0;

	// Frees all of this object's children
	virtual void				freeChildren( OvrVRMenuMgr & menuMgr ) = 0;

	// Adds a child to this menu object
	virtual void				addChild( OvrVRMenuMgr & menuMgr, menuHandle_t const handle ) = 0;
	// Removes a child from this menu object, but does not free the child object.
	virtual void				removeChild( OvrVRMenuMgr & menuMgr, menuHandle_t const handle ) = 0;
	// Removes a child from tis menu object and frees it, along with any children it may have.
	virtual void				freeChild( OvrVRMenuMgr & menuMgr, menuHandle_t const handle ) = 0;
	// Returns true if the handle is in this menu's tree of children
	virtual bool				isDescendant( OvrVRMenuMgr & menuMgr, menuHandle_t const handle ) const = 0;

	// Update this menu for a frame, including testing the gaze direction against the bounds
	// of this menu and all its children
	virtual void				frame( OvrVRMenuMgr & menuMgr, Matrix4f const & viewMatrix ) = 0;

	// Test the ray against this object and all child objects, returning the first object that
	// was hit by the ray. The ray should be in parent-local space - for the current root menu
	// this is always world space.
	virtual menuHandle_t		hitTest( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font, Posef const & worldPose,
                                        Vector3f const & rayStart, Vector3f const & rayDir, ContentFlags_t const testContents,
										HitTestResult & result ) const = 0;

	//--------------------------------------------------------------
	// components
	//--------------------------------------------------------------
	virtual void				addComponent( VRMenuComponent * component ) = 0;
	virtual	void				removeComponent( VRMenuComponent * component ) = 0;

	virtual Array< VRMenuComponent* > const & componentList() const = 0;

	// TODO We might want to refactor these into a single GetComponent which internally manages unique ids (using hashed class names for ex. )
	// Helper for getting component - returns NULL if it fails. Required Component class to overload GetTypeId and define unique TYPE_ID
	template< typename ComponentType >
	ComponentType*				GetComponentById() const
	{
		return static_cast< ComponentType* >( getComponentById_Impl( ComponentType::TYPE_ID ) );
	}
	virtual VRMenuComponent *		getComponentById_Impl( int id ) const = 0;
	// Helper for getting component - returns NULL if it fails. Required Component class to overload GetTypeName and define unique TYPE_NAME
	template< typename ComponentType >
	ComponentType*				GetComponentByName() const
	{
		return static_cast< ComponentType* >( getComponentByName_Impl( ComponentType::TYPE_NAME ) );
	}
	virtual VRMenuComponent *		getComponentByName_Impl( const char * typeName ) const = 0;

	//--------------------------------------------------------------
	// accessors
	//--------------------------------------------------------------
	virtual eVRMenuObjectType	type() const = 0;
	virtual menuHandle_t		handle() const = 0;
	virtual menuHandle_t		parentHandle() const = 0;
	virtual void				setParentHandle( menuHandle_t const h ) = 0;

	virtual	VRMenuObjectFlags_t const &	flags() const = 0;
	virtual	void				setFlags( VRMenuObjectFlags_t const & flags ) = 0;
	virtual	void				addFlags( VRMenuObjectFlags_t const & flags ) = 0;
	virtual	void				removeFlags( VRMenuObjectFlags_t const & flags ) = 0;

    virtual VString const &	text() const = 0;
    virtual void				setText( const VString &text ) = 0;
	virtual void				setTextWordWrapped( char const * text, class BitmapFont const & font, float const widthInMeters ) = 0;

	virtual bool				isHilighted() const = 0;
	virtual void				setHilighted( bool const b ) = 0;
	virtual bool				isSelected() const = 0;
	virtual void				setSelected( bool const b ) = 0;
	virtual	int					numChildren() const = 0;
	virtual menuHandle_t		getChildHandleForIndex( int const index ) const = 0;

	virtual Posef const &		localPose() const = 0;
	virtual void				setLocalPose( Posef const & pose ) = 0;
	virtual Vector3f const &	localPosition() const = 0;
	virtual void				setLocalPosition( Vector3f const & pos ) = 0;
	virtual Quatf const &		localRotation() const = 0;
	virtual void				setLocalRotation( Quatf const & rot ) = 0;
	virtual Vector3f            localScale() const = 0;
	virtual void				setLocalScale( Vector3f const & scale ) = 0;

    virtual Posef const &       hilightPose() const = 0;
    virtual void                setHilightPose( Posef const & pose ) = 0;
    virtual float               hilightScale() const = 0;
    virtual void                setHilightScale( float const setColor ) = 0;

    virtual void                setTextLocalPose( Posef const & pose ) = 0;
    virtual Posef const &       textLocalPose() const = 0;
    virtual void                setTextLocalPosition( Vector3f const & pos ) = 0;
    virtual Vector3f const &    textLocalPosition() const = 0;
    virtual void                setTextLocalRotation( Quatf const & rot ) = 0;
    virtual Quatf const &       textLocalRotation() const = 0;
    virtual Vector3f            textLocalScale() const = 0;
    virtual void                setTextLocalScale( Vector3f const & scale ) = 0;

	virtual	void				setLocalBoundsExpand( Vector3f const mins, Vector3f const & maxs ) = 0;

    virtual Bounds3f			getTextLocalBounds( BitmapFont const & font ) const = 0;
	virtual Bounds3f            setTextLocalBounds( BitmapFont const & font ) const = 0;

	virtual Bounds3f const &	cullBounds() const = 0;
	virtual void				setCullBounds( Bounds3f const & bounds ) const = 0;

	virtual	Vector2f const &	colorTableOffset() const = 0;
	virtual void				setColorTableOffset( Vector2f const & ofs ) = 0;

	virtual	Vector4f const &	color() const = 0;
	virtual	void				setColor( Vector4f const & c ) = 0;

	virtual	Vector4f const &	textColor() const = 0;
	virtual	void				setTextColor( Vector4f const & c ) = 0;

	virtual VRMenuId_t			id() const = 0;
	// pass -1 to use full depth of the tree.
	virtual menuHandle_t		childHandleForId( OvrVRMenuMgr & menuMgr, VRMenuId_t const id ) const = 0;

	virtual void				setFontParms( VRMenuFontParms const & fontParms ) = 0;
	virtual VRMenuFontParms const & fontParms() const = 0;

	virtual	Vector3f const &	fadeDirection() const = 0;
	virtual void				setFadeDirection( Vector3f const & dir ) = 0;

	virtual void				setVisible( bool visible ) = 0;

	// returns the index of the first surface with SURFACE_TEXTURE_ADDITIVE.
	// If singular is true, then the matching surface must have only one texture map and it must be of that type.
	virtual int					findSurfaceWithTextureType( eSurfaceTextureType const type, bool const singular ) const = 0;
	virtual	void				setSurfaceColor( int const surfaceIndex, Vector4f const & color ) = 0;
	virtual Vector4f const &	getSurfaceColor( int const surfaceIndex ) const = 0;
	virtual	void				setSurfaceVisible( int const surfaceIndex, bool const v ) = 0;
	virtual bool				getSurfaceVisible( int const surfaceIndex ) const = 0;
	virtual int					numSurfaces() const = 0;
	virtual int					allocSurface() = 0;
	virtual void 				createFromSurfaceParms( int const surfaceIndex, VRMenuSurfaceParms const & parms ) = 0;

    virtual void                setSurfaceTexture( int const surfaceIndex, int const textureIndex,
                                        eSurfaceTextureType const type, GLuint const texId,
                                        int const width, int const height ) = 0;

	virtual void                setSurfaceTextureTakeOwnership( int const surfaceIndex, int const textureIndex,
										eSurfaceTextureType const type, GLuint const texId,
										int const width, int const height ) = 0;

	virtual void 				regenerateSurfaceGeometry( int const surfaceIndex, const bool freeSurfaceGeometry ) = 0;

	virtual Vector2f const &	getSurfaceDims( int const surfaceIndex ) const = 0;
	virtual void				setSurfaceDims( int const surfaceIndex, Vector2f const &dims ) = 0;	// requires call to RegenerateSurfaceGeometry() to take effect

	virtual Vector4f const &	getSurfaceBorder( int const surfaceIndex ) = 0;
	virtual void				setSurfaceBorder( int const surfaceIndex, Vector4f const & border ) = 0;	// requires call to RegenerateSurfaceGeometry() to take effect

	//--------------------------------------------------------------
	// collision
	//--------------------------------------------------------------
	virtual void							setCollisionPrimitive( OvrCollisionPrimitive * c ) = 0;
	virtual OvrCollisionPrimitive *			collisionPrimitive() = 0;
	virtual OvrCollisionPrimitive const *	collisionPrimitive() const = 0;

	virtual ContentFlags_t		contents() const = 0;
	virtual void				setContents( ContentFlags_t const c ) = 0;

    // menu objects can only be created / deleted by the menu manager
	virtual ~VRMenuObject() { }
};

NV_NAMESPACE_END

#endif // OVR_VRMenuObject_h
