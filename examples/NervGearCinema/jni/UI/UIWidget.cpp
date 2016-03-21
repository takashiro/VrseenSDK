#include <VRMenuMgr.h>

#include "UI/UIWidget.h"
#include "UI/UIMenu.h"
#include "CinemaApp.h"

namespace OculusCinema {

UIWidget::UIWidget( CinemaApp &cinema ) :
	Cinema( cinema ),
	Parent( NULL ),
	Id(),
	Handle(),
	Object( NULL )

{
}

UIWidget::~UIWidget()
{
	//DeletePointerArray( MovieBrowserItems );
}

VRMenuObject * UIWidget::GetMenuObject() const
{
    return vApp->vrMenuMgr().toObject( GetHandle() );
}

void UIWidget::AddToMenuWithParms( UIMenu *menu, UIWidget *parent, VRMenuObjectParms &parms )
{
	Menu = menu;
	Parent = parent;

	Id = parms.Id;

    VArray< VRMenuObjectParms const * > parmArray;
    parmArray.append( &parms );

    menuHandle_t parentHandle = ( parent == NULL ) ? menu->GetVRMenu()->rootHandle() : parent->GetHandle();
    Menu->GetVRMenu()->addItems( vApp->vrMenuMgr(), vApp->defaultFont(), parmArray, parentHandle, false );
    parmArray.clear();

    if ( parent == NULL )
    {
        Handle = Menu->GetVRMenu()->handleForId( vApp->vrMenuMgr(), Id );
    }
    else
    {
        Handle = parent->GetMenuObject()->childHandleForId( vApp->vrMenuMgr(), Id );
    }
}

bool UIWidget::IsHilighted() const
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );
    return object->isHilighted();
}

void UIWidget::SetHilighted( bool const b )
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );
    object->setHilighted( b );
}

bool UIWidget::IsSelected() const
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );
    return object->isSelected();
}

void UIWidget::SetSelected( bool const b )
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );
    object->setSelected( b );
}

void UIWidget::SetLocalPose( const Posef &pose )
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );
	if ( object != NULL )
	{
        object->setLocalPose( pose );
	}
}

void UIWidget::SetLocalPose( const Quatf &orientation, const Vector3f &position )
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );
	if ( object != NULL )
	{
        object->setLocalPose( Posef( orientation, position ) );
	}
}

Posef const & UIWidget::GetLocalPose() const
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );
    return object->localPose();
}

void UIWidget::SetLocalPosition( Vector3f const & pos )
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );
	if ( object != NULL )
	{
        object->setLocalPosition( pos );
	}
}

Vector3f const & UIWidget::GetLocalPosition() const
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );
    return object->localPosition();
}

void UIWidget::SetLocalRotation( Quatf const & rot )
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );
	if ( object != NULL )
	{
        object->setLocalRotation( rot );
	}
}

Quatf const & UIWidget::GetLocalRotation() const
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );
    return object->localRotation();
}

void UIWidget::SetLocalScale( Vector3f const & scale )
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );
	if ( object != NULL )
	{
        object->setLocalScale( scale );
	}
}

void UIWidget::SetLocalScale( float const & scale )
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );
	if ( object != NULL )
	{
        object->setLocalScale( Vector3f( scale ) );
	}
}

Vector3f UIWidget::GetLocalScale() const
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );
    return object->localScale();
}

Posef UIWidget::GetWorldPose() const
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );

    Posef const & localPose = object->localPose();

	if ( Parent == NULL )
	{
		return localPose;
	}

	Posef parentModelPose = Parent->GetWorldPose();
	Vector3f parentScale = Parent->GetWorldScale();

	Posef curModelPose;
	curModelPose.Position = parentModelPose.Position + ( parentModelPose.Orientation * parentScale.EntrywiseMultiply( localPose.Position ) );
	curModelPose.Orientation = localPose.Orientation * parentModelPose.Orientation;

	return curModelPose;
}

Vector3f UIWidget::GetWorldPosition() const
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );

    Posef const & localPose = object->localPose();

	if ( Parent == NULL )
	{
		return localPose.Position;
	}

	Posef parentModelPose = Parent->GetWorldPose();
	Vector3f parentScale = Parent->GetWorldScale();

	return parentModelPose.Position + ( parentModelPose.Orientation * parentScale.EntrywiseMultiply( localPose.Position ) );
}

Quatf UIWidget::GetWorldRotation() const
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );

    Quatf const & rotation = object->localRotation();
	if ( Parent == NULL )
	{
		return rotation;
	}

	return rotation * Parent->GetWorldRotation();
}

Vector3f UIWidget::GetWorldScale() const
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );

    Vector3f scale = object->localScale();

	if ( Parent == NULL )
	{
		return scale;
	}

	Vector3f parentScale = Parent->GetWorldScale();
	return parentScale.EntrywiseMultiply( scale );
}

bool UIWidget::GetVisible() const
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );
    return ( object->flags() & VRMENUOBJECT_DONT_RENDER ) == 0;
}

void UIWidget::SetVisible( const bool visible )
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );
	if ( visible )
	{
        object->removeFlags( VRMENUOBJECT_DONT_RENDER );
	}
	else
	{
        object->addFlags( VRMENUOBJECT_DONT_RENDER );
	}
}

void UIWidget::SetImage( const int surfaceIndex, const eSurfaceTextureType textureType, char const * imageName )
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );

    while( object->numSurfaces() <= surfaceIndex )
	{
        object->allocSurface();
	}

	VRMenuSurfaceParms parms( "",
		imageName, textureType,
		NULL, SURFACE_TEXTURE_MAX,
		NULL, SURFACE_TEXTURE_MAX );

    object->createFromSurfaceParms( 0, parms );
}

void UIWidget::SetImage( const int surfaceIndex, const eSurfaceTextureType textureType, const GLuint image, const int width, const int height )
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );

    while( object->numSurfaces() <= surfaceIndex )
	{
        object->allocSurface();
	}

	VRMenuSurfaceParms parms( "",
		image, width, height, textureType,
		0, 0, 0, SURFACE_TEXTURE_MAX,
		0, 0, 0, SURFACE_TEXTURE_MAX );

    object->createFromSurfaceParms( 0, parms );
}

void UIWidget::SetImage( const int surfaceIndex, const eSurfaceTextureType textureType, const UITexture &image )
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );

    while( object->numSurfaces() <= surfaceIndex )
	{
        object->allocSurface();
	}

	VRMenuSurfaceParms parms( "",
		image.Texture, image.Width, image.Height, textureType,
		0, 0, 0, SURFACE_TEXTURE_MAX,
		0, 0, 0, SURFACE_TEXTURE_MAX );

    object->createFromSurfaceParms( 0, parms );
}

void UIWidget::SetImage( const int surfaceIndex, const eSurfaceTextureType textureType, const UITexture &image, const float dimsX, const float dimsY )
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );

    while( object->numSurfaces() <= surfaceIndex )
	{
        object->allocSurface();
	}

	VRMenuSurfaceParms parms( "",
		image.Texture, image.Width, image.Height, textureType,
		0, 0, 0, SURFACE_TEXTURE_MAX,
		0, 0, 0, SURFACE_TEXTURE_MAX );

	parms.Dims.x = dimsX;
	parms.Dims.y = dimsY;

    object->createFromSurfaceParms( 0, parms );
}

void UIWidget::SetImage( const int surfaceIndex, const eSurfaceTextureType textureType, const UITexture &image, const float dimsX, const float dimsY, const Vector4f &border )
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );

    while( object->numSurfaces() <= surfaceIndex )
	{
        object->allocSurface();
	}

	VRMenuSurfaceParms parms( "",
		image.Texture, image.Width, image.Height, textureType,
		0, 0, 0, SURFACE_TEXTURE_MAX,
		0, 0, 0, SURFACE_TEXTURE_MAX );

	parms.Dims.x = dimsX;
	parms.Dims.y = dimsY;
	parms.Border = border;

    object->createFromSurfaceParms( 0, parms );
}

void UIWidget::SetImage( const int surfaceIndex, VRMenuSurfaceParms const & parms )
{
	VRMenuObject * object = GetMenuObject();
	OVR_ASSERT( object );

    while( object->numSurfaces() <= surfaceIndex )
	{
        object->allocSurface();
	}

    object->createFromSurfaceParms( 0, parms );
}

void UIWidget::AddComponent( VRMenuComponent * component )
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    object->addComponent( component );
}

void UIWidget::RemoveComponent( VRMenuComponent * component )
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    object->removeComponent( component );
}

VArray< VRMenuComponent* > const & UIWidget::GetComponentList() const
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    return object->componentList();
}

void UIWidget::SetColorTableOffset( Vector2f const & ofs )
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    object->setColorTableOffset( ofs );
}

Vector2f const & UIWidget::GetColorTableOffset() const
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    return object->colorTableOffset();
}

void UIWidget::SetColor( Vector4f const & c )
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    object->setColor( c );
}

Vector4f const & UIWidget::GetColor() const
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    return object->color();
}

void UIWidget::RegenerateSurfaceGeometry( int const surfaceIndex, const bool freeSurfaceGeometry )
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    object->regenerateSurfaceGeometry( surfaceIndex, freeSurfaceGeometry );
}

Vector2f const & UIWidget::GetSurfaceDims( int const surfaceIndex ) const
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    return object->getSurfaceDims( surfaceIndex );
}

void UIWidget::SetSurfaceDims( int const surfaceIndex, Vector2f const &dims )
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    object->setSurfaceDims( surfaceIndex, dims );
}

Vector4f const & UIWidget::GetSurfaceBorder( int const surfaceIndex )
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    return object->getSurfaceBorder( surfaceIndex );
}

void UIWidget::SetSurfaceBorder( int const surfaceIndex, Vector4f const & border )
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    return object->setSurfaceBorder( surfaceIndex, border );
}

bool UIWidget::GetSurfaceVisible( int const surfaceIndex ) const
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    return object->getSurfaceVisible( surfaceIndex );
}

void UIWidget::SetSurfaceVisible( int const surfaceIndex, const bool visible )
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    object->setSurfaceVisible( surfaceIndex, visible );
}

void UIWidget::SetLocalBoundsExpand( Vector3f const mins, Vector3f const & maxs )
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    object->setLocalBoundsExpand( mins, maxs );
}

} // namespace OculusCinema
