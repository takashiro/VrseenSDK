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
	void								SetText( const VString &text );
	void								SetTextWordWrapped(const VString &text, class BitmapFont const & font, float const widthInMeters );
	const VString & 						GetText() const;

	void								SetFontScale( float const scale );
	float 								GetFontScale() const;

    void     				           	SetTextOffset( V3Vectf const & pos );
    V3Vectf const &                     GetTextOffset() const;

    V4Vectf const &					GetTextColor() const;
    void								SetTextColor( V4Vectf const & c );

    VBoxf            				GetTextLocalBounds( BitmapFont const & font ) const;
};

} // namespace OculusCinema

#endif // UILabel_h
