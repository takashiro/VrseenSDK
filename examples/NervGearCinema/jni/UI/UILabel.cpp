#include <VRMenuMgr.h>

#include "UI/UILabel.h"
#include "UI/UIMenu.h"
#include "CinemaApp.h"

namespace OculusCinema {

UILabel::UILabel( CinemaApp &cinema ) :
	UIWidget( cinema )

{
}

UILabel::~UILabel()
{
}

void UILabel::AddToMenu( UIMenu *menu, UIWidget *parent )
{
	const Posef pose( Quatf( Vector3f( 0.0f, 1.0f, 0.0f ), 0.0f ), Vector3f( 0.0f, 0.0f, 0.0f ) );

	Vector3f defaultScale( 1.0f );
	VRMenuFontParms fontParms( true, true, false, false, false, 1.0f );
	
	VRMenuObjectParms parms( VRMENU_STATIC, Array< VRMenuComponent* >(), VRMenuSurfaceParms(),
			"", pose, defaultScale, fontParms, menu->AllocId(),
			VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

	AddToMenuWithParms( menu, parent, parms );
}

void UILabel::SetText( const char *text )
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    object->setText( text );
}

void UILabel::SetText( const VString &text )
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    object->setText(text);
}

void UILabel::SetTextWordWrapped( char const * text, class BitmapFont const & font, float const widthInMeters )
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    object->setTextWordWrapped( text, font, widthInMeters );
}

const VString & UILabel::GetText() const
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    return object->text();
}

void UILabel::SetFontScale( float const scale )
{
	VRMenuObject * object = GetMenuObject();
	assert( object );

    VRMenuFontParms parms = object->fontParms();
	parms.Scale = scale;
    object->setFontParms( parms );

}

float UILabel::GetFontScale() const
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    VRMenuFontParms parms = object->fontParms();
	return parms.Scale;
}

void UILabel::SetTextOffset( Vector3f const & pos )
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    object->setTextLocalPosition( pos );
}

Vector3f const & UILabel::GetTextOffset() const
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    return object->textLocalPosition();
}

void UILabel::SetTextColor( Vector4f const & c )
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    object->setTextColor( c );
}

Vector4f const & UILabel::GetTextColor() const
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    return object->textColor();
}

Bounds3f UILabel::GetTextLocalBounds( BitmapFont const & font ) const
{
	VRMenuObject * object = GetMenuObject();
	assert( object );
    return object->getTextLocalBounds( font );
}

} // namespace OculusCinema
