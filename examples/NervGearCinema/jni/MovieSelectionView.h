#if !defined( MovieSelectionView_h )
#define MovieSelectionView_h

#include <VArray.h>

#include "Lerp.h"
#include "View.h"
#include "CarouselBrowserComponent.h"
#include "MovieManager.h"
#include "UI/UITexture.h"
#include "UI/UIMenu.h"
#include "UI/UIContainer.h"
#include "UI/UILabel.h"
#include "UI/UIImage.h"

using namespace NervGear;

namespace OculusCinema {

class CinemaApp;
class CarouselBrowserComponent;
class MovieSelectionComponent;

class MovieSelectionView : public View
{
public:
										MovieSelectionView( CinemaApp &cinema );
	virtual 							~MovieSelectionView();

	virtual void 						OneTimeInit(const VString &launchIntent );
	virtual void						OneTimeShutdown();

	virtual void 						OnOpen();
	virtual void 						OnClose();

    virtual VR4Matrixf 					DrawEyeView( const int eye, const float fovDegrees );
    virtual VR4Matrixf 					Frame( const VrFrame & vrFrame );
	virtual bool						Command( const char * msg );
	virtual bool 						OnKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType );

    void 								SetMovieList( const VArray<const MovieDef *> &movies, const MovieDef *nextMovie );

	void 								SelectMovie( void );
	void 								SelectionHighlighted( bool isHighlighted );
    void 								SetCategory( const MovieCategory category );
	void								SetError( const char *text, bool showSDCard, bool showErrorIcon );
	void								ClearError();

private:
    class MovieCategoryButton
    {
    public:
    	MovieCategory 	Category;
    	VString			Text;
    	UILabel *		Button;
    	float			Width;
    	float			Height;

    					MovieCategoryButton( const MovieCategory category, const VString &text ) :
    						Category( category ), Text( text ), Button( NULL ), Width( 0.0f ), Height( 0.0f ) {}
    };

private:
    CinemaApp &							Cinema;

    UITexture 							SelectionTexture;
    UITexture							Is3DIconTexture;
    UITexture							ShadowTexture;
    UITexture							BorderTexture;
    UITexture							SwipeIconLeftTexture;
    UITexture							SwipeIconRightTexture;
    UITexture							ResumeIconTexture;
    UITexture							ErrorIconTexture;
    UITexture							SDCardTexture;

	UIMenu *							Menu;

	UIContainer *						CenterRoot;

	UILabel * 							ErrorMessage;
	UILabel * 							SDCardMessage;
	UILabel * 							PlainErrorMessage;

	bool								ErrorMessageClicked;

	UIContainer *						MovieRoot;
    UIContainer *						CategoryRoot;
	UIContainer *						TitleRoot;

	UILabel	*							MovieTitle;

	UIImage *							SelectionFrame;

	UIImage *							CenterPoster;
	uint								CenterIndex;
    V3Vectf							CenterPosition;

	UIImage *							LeftSwipes[ 3 ];
	UIImage * 							RightSwipes[ 3 ];

	UILabel	*							ResumeIcon;

	UILabel *							TimerIcon;
	UILabel *							TimerText;
	double								TimerStartTime;
	int									TimerValue;
	bool								ShowTimer;

	UILabel *							MoveScreenLabel;
	Lerp								MoveScreenAlpha;

	Lerp								SelectionFader;

	CarouselBrowserComponent *			MovieBrowser;
	VArray<CarouselItem *> 				MovieBrowserItems;
	VArray<PanelPose>					MoviePanelPositions;

    VArray<CarouselItemComponent *>	 	MoviePosterComponents;

	VArray<MovieCategoryButton>			Categories;
    MovieCategory			 			CurrentCategory;

	VArray<const MovieDef *> 			MovieList;
	int									MoviesIndex;

	const MovieDef *					LastMovieDisplayed;

	bool								RepositionScreen;
	bool								HadSelection;

private:
	const MovieDef *					GetSelectedMovie() const;

	void 								CreateMenu( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font );
    V3Vectf 							ScalePosition( const V3Vectf &startPos, const float scale, const float menuOffset ) const;
	void 								UpdateMenuPosition();

	void								StartTimer();

	void								UpdateMovieTitle();
	void								UpdateSelectionFrame( const VrFrame & vrFrame );

	bool								ErrorShown() const;
};

} // namespace OculusCinema

#endif // MovieSelectionView_h
