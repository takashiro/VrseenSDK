#if !defined( UIWidget_h )
#define UIWidget_h

#include "gui/VRMenu.h"

using namespace NervGear;

namespace OculusCinema {

class CinemaApp;
class UIMenu;
class UITexture;

class UIWidget
{
public:
										UIWidget( CinemaApp &cinema );
										~UIWidget();

	menuHandle_t						GetHandle() const { return Handle; }
	VRMenuObject *						GetMenuObject() const;

	UIWidget *							GetParent() const { return Parent; }

	bool								IsHilighted() const;
	void								SetHilighted( bool const b );
	bool								IsSelected() const;
	void								SetSelected( bool const b );

    void								SetLocalPose( const VPosf &pose );
    void								SetLocalPose( const VQuatf &orientation, const V3Vectf &position );
    VPosf const & 						GetLocalPose() const;
    V3Vectf const &					GetLocalPosition() const;
    void								SetLocalPosition( V3Vectf const & pos );
    VQuatf const &						GetLocalRotation() const;
    void								SetLocalRotation( VQuatf const & rot );
    V3Vectf            				GetLocalScale() const;
    void								SetLocalScale( V3Vectf const & scale );
	void								SetLocalScale( float const & scale );

    VPosf 								GetWorldPose() const;
    V3Vectf 							GetWorldPosition() const;
    VQuatf 								GetWorldRotation() const;
    V3Vectf            				GetWorldScale() const;

    V2Vectf const &					GetColorTableOffset() const;
    void								SetColorTableOffset( V2Vectf const & ofs );
    V4Vectf const &					GetColor() const;
    void								SetColor( V4Vectf const & c );
	bool								GetVisible() const;
	void								SetVisible( const bool visible );

	void								SetImage( const int surfaceIndex, const eSurfaceTextureType textureType, char const * imageName );
	void								SetImage( const int surfaceIndex, const eSurfaceTextureType textureType, const GLuint image, const int width, const int height );
	void								SetImage( const int surfaceIndex, const eSurfaceTextureType textureType, const UITexture &image );
	void								SetImage( const int surfaceIndex, const eSurfaceTextureType textureType, const UITexture &image, const float dimsX, const float dimsY );
    void								SetImage( const int surfaceIndex, const eSurfaceTextureType textureType, const UITexture &image, const float dimsX, const float dimsY, const V4Vectf &border );
	void 								SetImage( const int surfaceIndex, VRMenuSurfaceParms const & parms );

	void 								RegenerateSurfaceGeometry( int const surfaceIndex, const bool freeSurfaceGeometry );

    V2Vectf const &					GetSurfaceDims( int const surfaceIndex ) const;
    void								SetSurfaceDims( int const surfaceIndex, V2Vectf const &dims );	// requires call to RegenerateSurfaceGeometry() to take effect

    V4Vectf const &					GetSurfaceBorder( int const surfaceIndex );
    void								SetSurfaceBorder( int const surfaceIndex, V4Vectf const & border );	// requires call to RegenerateSurfaceGeometry() to take effect

	bool								GetSurfaceVisible( int const surfaceIndex ) const;
	void								SetSurfaceVisible( int const surfaceIndex, const bool visible );

    void								SetLocalBoundsExpand( V3Vectf const mins, V3Vectf const & maxs );

	void								AddComponent( VRMenuComponent * component );
	void								RemoveComponent( VRMenuComponent * component ) ;
	VArray< VRMenuComponent* > const & 	GetComponentList() const;

protected:
	void 								AddToMenuWithParms( UIMenu *menu, UIWidget *parent, VRMenuObjectParms &parms );

    CinemaApp &							Cinema;

	UIMenu *							Menu;

	UIWidget *							Parent;

	VRMenuId_t 							Id;
	menuHandle_t						Handle;
	VRMenuObject *						Object;
};

} // namespace OculusCinema

#endif // UIMenu_h
