/************************************************************************************

Filename    :   VRMenuObject.h
Content     :   Menuing system for VR apps.
Created     :   May 23, 2014
Authors     :   Jonathan E. Wright

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#if !defined( OVR_VRMenuObjectLocal_h )
#define OVR_VRMenuObjectLocal_h

#include "VRMenuObject.h"
#include "VRMenuMgr.h"
#include "CollisionPrimitive.h"
#include "BitmapFont.h"

NV_NAMESPACE_BEGIN

typedef SInt16 guiIndex_t;

struct textMetrics_t {
	textMetrics_t() :
        w( 0.0f ),
        h( 0.0f ),
        ascent( 0.0f ),
		descent( 0.0f ),
		fontHeight( 0.0f )
	{
    }

	float w;
	float h;
	float ascent;
	float descent;
	float fontHeight;
};

class App;

//==============================================================
// VRMenuSurfaceTexture
class VRMenuSurfaceTexture
{
public:
	VRMenuSurfaceTexture();

    bool	loadTexture( eSurfaceTextureType const type, char const * imageName, bool const allowDefault );
    void 	loadTexture( eSurfaceTextureType const type, const GLuint texId, const int width, const int height );
    void	free();
    void	setOwnership( const bool isOwner )	{ m_ownsTexture = isOwner; }

    GLuint				handle() const { return m_handle; }
    int					width() const { return m_width; }
    int					height() const { return m_height; }
    eSurfaceTextureType	type() const { return m_type; }

private:
    GLuint				m_handle;	// GL texture handle
    int					m_width;	// width of the image
    int					m_height;	// height of the image
    eSurfaceTextureType	m_type;	// specifies how this image is used for rendering
    bool                m_ownsTexture;    // if true, free texture on a reload or deconstruct
};

//==============================================================
// SubmittedMenuObject
class SubmittedMenuObject
{
public:
	SubmittedMenuObject() :
        surfaceIndex( -1 ),
        distanceIndex( -1 ),
        scale( 1.0f ),
        color( 1.0f ),
        colorTableOffset( 0.0f ),
        skipAdditivePass( false ),
        fadeDirection( 0.0f )
	{
	}

    menuHandle_t				handle;				// handle of the object
    int							surfaceIndex;		// surface of the object
    int							distanceIndex;		// use the position at this index to calc sort distance
    Posef						pose;				// pose in model space
    Vector3f					scale;				// scale of the object
    Vector4f					color;				// color of the object
    Vector2f					colorTableOffset;	// color table offset for color ramp fx
    bool						skipAdditivePass;	// true to skip any additive multi-texture pass
    VRMenuRenderFlags_t			flags;				// various flags
    Vector2f					offsets;			// offsets based on anchors (width / height * anchor.x / .y)
    Vector3f					fadeDirection;		// Fades vertices based on direction - default is zero vector which indicates off
#if defined( OVR_BUILD_DEBUG )
	VString						SurfaceName;		// for debugging only
#endif
};


//==============================================================
// VRMenuSurface
class VRMenuSurface
{
public:
    // artificial bounds in Z, which is technically 0 for surfaces that are an image
    static const float Z_BOUNDS;

	VRMenuSurface();
	~VRMenuSurface();

    void							createFromSurfaceParms( VRMenuSurfaceParms const & parms );

    void							free();

    void 							regenerateSurfaceGeometry();

    void							render( OvrVRMenuMgr const & menuMgr, Matrix4f const & mvp, SubmittedMenuObject const & sub ) const;

    Bounds3f const &				localBounds() const { return m_tris.bounds(); }

    bool							isRenderable() const { return ( m_geo.vertexCount > 0 && m_geo.indexCount > 0 ) && m_visible; }

    bool							intersectRay( Vector3f const & start, Vector3f const & dir, Posef const & pose,
									        Vector3f const & scale, ContentFlags_t const testContents, OvrCollisionResult & result ) const;
    // the ray should already be in local space
    bool							intersectRay( Vector3f const & localStart, Vector3f const & localDir,
									        Vector3f const & scale, ContentFlags_t const testContents, OvrCollisionResult & result ) const;
    void							loadTexture( int const textureIndex, eSurfaceTextureType const type,
									        const GLuint texId, const int width, const int height );

    Vector4f const&					color() const { return m_color; }
    void							setColor( Vector4f const & color ) { m_color = color; }

    Vector2f const &				dims() const { return m_dims; }
    void							setDims( Vector2f const &dims ) { m_dims = dims; } // requires call to CreateFromSurfaceParms or RegenerateSurfaceGeometry() to take effect

    Vector2f const &				anchors() const { return m_anchors; }
    void							setAnchors( Vector2f const & a ) { m_anchors = a; }

    Vector2f						anchorOffsets() const;

    Vector4f const &				border() const { return m_border; }
    void							setBorder( Vector4f const & a ) { m_border = a; }	// requires call to CreateFromSurfaceParms or RegenerateSurfaceGeometry() to take effect

    VRMenuSurfaceTexture const &	getTexture( int const index )  const { return m_textures[index]; }
    void							setOwnership( int const index, bool const isOwner );
	
    bool							isVisible() const { return m_visible; }
    void							setVisible( bool const v ) { m_visible = v; }

    VString const &					name() const { return m_furfaceName; }

private:
    VRMenuSurfaceTexture			m_textures[VRMENUSURFACE_IMAGE_MAX];
    GlGeometry						m_geo;				// VBO for this surface
    OvrTriCollisionPrimitive		m_tris;				// per-poly collision object
    Vector4f						m_color;				// Color, modulated with object color
    Vector2f						m_textureDims;		// texture width and height
    Vector2f						m_dims;				// width and height
    Vector2f						m_anchors;			// anchors
    Vector4f						m_border;				// border size for sliced sprite
    VString      					m_furfaceName;		// name of the surface for debugging
    ContentFlags_t					m_contents;
    bool							m_visible;			// must be true to render -- used to animate between different surfaces

    eGUIProgramType					m_programType;

private:
    void							createImageGeometry(  int const textureWidth, int const textureHeight, const Vector2f &dims, const Vector4f &border, ContentFlags_t const content );
	// This searches the loaded textures for the specified number of matching types. For instance,
	// ( SURFACE_TEXTURE_DIFFUSE, 2 ) would only return true if two of the textures were of type
	// SURFACE_TEXTURE_DIFFUSE.  This is used to determine, based on surface types, which shaders
	// to use to render the surface, independent of how the texture maps are ordered.
    bool							hasTexturesOfType( eSurfaceTextureType const t, int const requiredCount ) const;
	// Returns the index in Textures[] of the n-th occurence of type t.
    int								indexForTextureType( eSurfaceTextureType const t, int const occurenceCount ) const;
};

//==============================================================
// VRMenuObjectLocal
// base class for all menu objects

class VRMenuObjectLocal : public VRMenuObject
{
public:
	friend class VRMenuObject;
	friend class VRMenuMgr;
	friend class VRMenuMgrLocal;

	// Initialize the object after creation
    virtual void				init( VRMenuObjectParms const & parms );

	// Frees all of this object's children
    virtual void				freeChildren( OvrVRMenuMgr & menuMgr );

	// Adds a child to this menu object
    virtual void				addChild( OvrVRMenuMgr & menuMgr, menuHandle_t const handle );
	// Removes a child from this menu object, but does not free the child object.
    virtual void				removeChild( OvrVRMenuMgr & menuMgr, menuHandle_t const handle );
	// Removes a child from tis menu object and frees it, along with any children it may have.
    virtual void				freeChild( OvrVRMenuMgr & menuMgr, menuHandle_t const handle );
	// Returns true if the handle is in this menu's tree of children
    virtual bool				isDescendant( OvrVRMenuMgr & menuMgr, menuHandle_t const handle ) const;

	// Update this menu for a frame, including testing the gaze direction against the bounds
	// of this menu and all its children
    virtual void				frame( OvrVRMenuMgr & menuMgr, Matrix4f const & viewMatrix );

    // Tests a ray (in object's local space) for intersection.
    bool                        intersectRay( Vector3f const & localStart, Vector3f const & localDir, Vector3f const & parentScale, Bounds3f const & bounds,
                                        float & bounds_t0, float & bounds_t1, ContentFlags_t const testContents, 
										OvrCollisionResult & result ) const;

	// Test the ray against this object and all child objects, returning the first object that was 
	// hit by the ray. The ray should be in parent-local space - for the current root menu this is 
	// always world space.
    virtual menuHandle_t		hitTest( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font, Posef const & worldPose,
                                        Vector3f const & rayStart, Vector3f const & rayDir, ContentFlags_t const testContents, 
										HitTestResult & result ) const;

	//--------------------------------------------------------------
	// components
	//--------------------------------------------------------------
    virtual void				addComponent( VRMenuComponent * component );
    virtual	void				removeComponent( VRMenuComponent * component );

    virtual Array< VRMenuComponent* > const & componentList() const { return m_components; }

    virtual VRMenuComponent *	getComponentById_Impl( int id ) const;
    virtual VRMenuComponent *	getComponentByName_Impl( const char * typeName ) const;

	//--------------------------------------------------------------
	// accessors
	//--------------------------------------------------------------
    virtual eVRMenuObjectType	type() const { return m_type; }
    virtual menuHandle_t		handle() const { return m_handle; }
    virtual menuHandle_t		parentHandle() const { return m_parentHandle; }
    virtual void				setParentHandle( menuHandle_t const h ) { m_parentHandle = h; }

    virtual	VRMenuObjectFlags_t const &	flags() const { return m_flags; }
    virtual	void				setFlags( VRMenuObjectFlags_t const & flags ) { m_flags = flags; }
    virtual	void				addFlags( VRMenuObjectFlags_t const & flags ) { m_flags |= flags; }
    virtual	void				removeFlags( VRMenuObjectFlags_t const & flags ) { m_flags &= ~flags; }

    virtual VString const &	text() const { return m_text; }
    void setText(const VString &text) override { m_text = text; m_textDirty = true; }
    virtual void				setTextWordWrapped( char const * text, class BitmapFont const & font, float const widthInMeters );

    virtual bool				isHilighted() const { return m_hilighted; }
    virtual void				setHilighted( bool const b ) { m_hilighted = b; }
    virtual bool				isSelected() const { return m_selected; }
    virtual void				setSelected( bool const b ) { m_selected = b; }
    virtual	int					numChildren() const { return m_children.sizeInt(); }
    virtual menuHandle_t		getChildHandleForIndex( int const index ) const { return m_children[index]; }

    virtual Posef const &		localPose() const { return m_localPose; }
    virtual void				setLocalPose( Posef const & pose ) { m_localPose = pose; }
    virtual Vector3f const &	localPosition() const { return m_localPose.Position; }
    virtual void				setLocalPosition( Vector3f const & pos ) { m_localPose.Position = pos; }
    virtual Quatf const &		localRotation() const { return m_localPose.Orientation; }
    virtual void				setLocalRotation( Quatf const & rot ) { m_localPose.Orientation = rot; }
    virtual Vector3f            localScale() const;
    virtual void				setLocalScale( Vector3f const & scale ) { m_localScale = scale; }

    virtual Posef const &       hilightPose() const { return m_hilightPose; }
    virtual void                setHilightPose( Posef const & pose ) { m_hilightPose = pose; }
    virtual float               hilightScale() const { return m_hilightScale; }
    virtual void                setHilightScale( float const s ) { m_hilightScale = s; }

    virtual void                setTextLocalPose( Posef const & pose ) { m_textLocalPose = pose; }
    virtual Posef const &       textLocalPose() const { return m_textLocalPose; }
    virtual void                setTextLocalPosition( Vector3f const & pos ) { m_textLocalPose.Position = pos; }
    virtual Vector3f const &    textLocalPosition() const { return m_textLocalPose.Position; }
    virtual void                setTextLocalRotation( Quatf const & rot ) { m_textLocalPose.Orientation = rot; }
    virtual Quatf const &       textLocalRotation() const { return m_textLocalPose.Orientation; }
    virtual Vector3f            textLocalScale() const;
    virtual void                setTextLocalScale( Vector3f const & scale ) { m_textLocalScale = scale; }

    virtual	void				setLocalBoundsExpand( Vector3f const mins, Vector3f const & maxs );

    virtual Bounds3f			getTextLocalBounds( BitmapFont const & font ) const;
    virtual Bounds3f            setTextLocalBounds( BitmapFont const & font ) const;

    virtual Bounds3f const &	cullBounds() const { return m_cullBounds; }
    virtual void				setCullBounds( Bounds3f const & bounds ) const { m_cullBounds = bounds; }

    virtual	Vector2f const &	colorTableOffset() const;
    virtual void				setColorTableOffset( Vector2f const & ofs );

    virtual	Vector4f const &	color() const;
    virtual	void				setColor( Vector4f const & c );

    virtual	Vector4f const &	textColor() const { return m_textColor; }
    virtual	void				setTextColor( Vector4f const & c ) { m_textColor = c; }

    virtual VRMenuId_t			id() const { return m_id; }
    virtual menuHandle_t		childHandleForId( OvrVRMenuMgr & menuMgr, VRMenuId_t const id ) const;

    virtual void				setFontParms( VRMenuFontParms const & fontParms ) { m_fontParms = fontParms; }
    virtual VRMenuFontParms const & fontParms() const { return m_fontParms; }

    virtual	Vector3f const &	fadeDirection() const { return m_fadeDirection;  }
    virtual void				setFadeDirection( Vector3f const & dir ) { m_fadeDirection = dir;  }

    virtual void				setVisible( bool visible );

	// returns the index of the first surface with SURFACE_TEXTURE_ADDITIVE.
	// If singular is true, then the matching surface must have only one texture map and it must be of that type.
    virtual int					findSurfaceWithTextureType( eSurfaceTextureType const type, bool const singular ) const;
    virtual	void				setSurfaceColor( int const surfaceIndex, Vector4f const & color );
    virtual Vector4f const &	getSurfaceColor( int const surfaceIndex ) const;
    virtual	void				setSurfaceVisible( int const surfaceIndex, bool const v );
    virtual bool				getSurfaceVisible( int const surfaceIndex ) const;
    virtual int					numSurfaces() const;
    virtual int					allocSurface();
    virtual void 				createFromSurfaceParms( int const surfaceIndex, VRMenuSurfaceParms const & parms );

    virtual void                setSurfaceTexture( int const surfaceIndex, int const textureIndex,
                                        eSurfaceTextureType const type, GLuint const texId, 
                                        int const width, int const height );

    virtual void                setSurfaceTextureTakeOwnership( int const surfaceIndex, int const textureIndex,
										eSurfaceTextureType const type, GLuint const texId,
										int const width, int const height );

    virtual void 				regenerateSurfaceGeometry( int const surfaceIndex, const bool freeSurfaceGeometry );

    virtual Vector2f const &	getSurfaceDims( int const surfaceIndex ) const;
    virtual void				setSurfaceDims( int const surfaceIndex, Vector2f const &dims );

    virtual Vector4f const &	getSurfaceBorder( int const surfaceIndex );
    virtual void				setSurfaceBorder( int const surfaceIndex, Vector4f const & border );

	//--------------------------------------------------------------
	// collision
	//--------------------------------------------------------------
    virtual void							setCollisionPrimitive( OvrCollisionPrimitive * c );
    virtual OvrCollisionPrimitive *			collisionPrimitive() { return m_collisionPrimitive; }
    virtual OvrCollisionPrimitive const *	collisionPrimitive() const { return m_collisionPrimitive; }

    virtual ContentFlags_t		contents() const { return m_contents; }
    virtual void				setContents( ContentFlags_t const c ) { m_contents = c; }

	//--------------------------------------------------------------
	// surfaces (non-virtual)
	//--------------------------------------------------------------
    VRMenuSurface const &			getSurface( int const s ) const { return m_surfaces[s]; }
    VRMenuSurface &					getSurface( int const s ) { return m_surfaces[s]; }
    Array< VRMenuSurface > const &	surfaces() const { return m_surfaces; }

    float						wrapWidth() const { return m_wrapWidth; }


private:
    eVRMenuObjectType			m_type;			// type of this object
    menuHandle_t				m_handle;			// handle of this object
    menuHandle_t				m_parentHandle;	// handle of this object's parent
    VRMenuId_t					m_id;				// opaque id that the creator of the menu can use to identify a menu object
    VRMenuObjectFlags_t			m_flags;			// various bit flags
    Posef						m_localPose;		// local-space position and orientation
    Vector3f					m_localScale;		// local-space scale of this item
    Posef                       m_hilightPose;    // additional pose applied when hilighted
    float                       m_hilightScale;   // additional scale when hilighted
    Posef                       m_textLocalPose;  // local-space position and orientation of text, local to this node (i.e. after LocalPose / LocalScale are applied)
    Vector3f                    m_textLocalScale; // local-space scale of the text at this node
    VString					m_text;			// text to display on this object
    Array< menuHandle_t >		m_children;		// array of direct children of this object
    Array< VRMenuComponent* >	m_components;		// array of components on this object
    OvrCollisionPrimitive *		m_collisionPrimitive;		// collision surface, if any
    ContentFlags_t				m_contents;		// content flags for this object

	// may be cleaner to put texture and geometry in a separate surface structure
    Array< VRMenuSurface >		m_surfaces;
    Vector4f					m_color;				// color modulation
    Vector4f                    m_textColor;          // color modulation for text
    Vector2f					m_colorTableOffset;	// offset for color-ramp shader fx
    VRMenuFontParms				m_fontParms;			// parameters for text rendering
    Vector3f                    m_fadeDirection;		// Fades vertices based on direction - default is zero vector which indicates off

    bool						m_hilighted;		// true if hilighted
    bool						m_selected;		// true if selected
    mutable bool                m_textDirty;      // if true, recalculate text bounds

    // cached state
    Vector3f					m_minsBoundsExpand;	// amount to expand local bounds mins
    Vector3f					m_maxsBoundsExpand;	// amount to expand local bounds maxs
    mutable Bounds3f			m_cullBounds;			// bounds of this object and all its children in the local space of its parent
    mutable textMetrics_t       m_textMetrics;		// cached metrics for the text

    float						m_wrapWidth;

private:
	// only VRMenuMgrLocal static methods can construct and destruct a menu object.
								VRMenuObjectLocal( VRMenuObjectParms const & parms, menuHandle_t const handle );
	virtual						~VRMenuObjectLocal();

	// Render the specified surface.
    virtual void				renderSurface( OvrVRMenuMgr const & menuMgr, Matrix4f const & mvp,
                                        SubmittedMenuObject const & sub ) const;

    bool						intersectRayBounds( Vector3f const & start, Vector3f const & dir,
										Vector3f const & mins, Vector3f const & maxs,
										ContentFlags_t const testContents, float & t0, float & t1 ) const;

	// Test the ray against this object and all child objects, returning the first object that was 
	// hit by the ray. The ray should be in parent-local space - for the current root menu this is 
	// always world space.
    virtual bool				 hitTest_r( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font, Posef const & parentPose, Vector3f const & parentScale,
                                        Vector3f const & rayStart, Vector3f const & rayDir,  ContentFlags_t const testContents, 
                                        HitTestResult & result ) const;

    int							getComponentIndex( VRMenuComponent * component ) const;
};

NV_NAMESPACE_END

#endif // OVR_VRMenuObjectLocal_h
