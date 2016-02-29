#if !defined( UILabel_h )
#define UILabel_h

#include <gui/VRMenu.h>

#include "UI/UIWidget.h"

using namespace NervGear;

namespace OculusCinema {

class CinemaApp;

class UILabel : public UIWidget
{
public:
										UILabel( CinemaApp &cinema );
										~UILabel();

	void 								AddToMenu( UIMenu *menu, UIWidget *parent = NULL );

	void								SetText( const char *text );
	void								SetText( const String &text );
	void								SetTextWordWrapped( char const * text, class BitmapFont const & font, float const widthInMeters );
	const String & 						GetText() const;

	void								SetFontScale( float const scale );
	float 								GetFontScale() const;

	void     				           	SetTextOffset( Vector3f const & pos );
    Vector3f const &    				GetTextOffset() const;

	Vector4f const &					GetTextColor() const;
	void								SetTextColor( Vector4f const & c );

	Bounds3f            				GetTextLocalBounds( BitmapFont const & font ) const;
};

} // namespace OculusCinema

#endif // UILabel_h
