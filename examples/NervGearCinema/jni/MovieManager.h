#pragma once

#include "VString.h"
#include "VArray.h"
#include "GlTexture.h"

namespace OculusCinema {

class CinemaApp;

using namespace NervGear;

enum MovieFormat
{
	VT_UNKNOWN,
	VT_2D,
	VT_LEFT_RIGHT_3D,			// Left & right are scaled horizontally by 50%.
	VT_LEFT_RIGHT_3D_FULL,		// Left & right are unscaled.
	VT_TOP_BOTTOM_3D,			// Top & bottom are scaled vertically by 50%.
	VT_TOP_BOTTOM_3D_FULL,		// Top & bottom are unscaled.
};

enum MovieCategory {
	CATEGORY_MYVIDEOS,
	CATEGORY_TRAILERS
};

class MovieDef
{
public:
	VString			Filename;

	VString			Title;

	bool			Is3D;
	MovieFormat 	Format;

	GLuint			Poster;
	int				PosterWidth;
	int				PosterHeight;

	VString			Theater;
	MovieCategory	Category;

	bool            IsEncrypted;
	bool			AllowTheaterSelection;


	MovieDef() : Filename(), Title(), Is3D( false ), Format( VT_2D ), Poster( 0 ), PosterWidth( 0 ), PosterHeight( 0 ),
			Theater(), Category( CATEGORY_MYVIDEOS ), IsEncrypted( false ), AllowTheaterSelection( false ) {}
};

class MovieManager
{
public:
							MovieManager( CinemaApp &cinema );
							~MovieManager();

	void					OneTimeInit(const VString &launchIntent );
	void					OneTimeShutdown();

	VArray<const MovieDef *>	GetMovieList( MovieCategory category ) const;

	static const VString 	GetMovieTitleFromFilename( const char *filepath );

public:
    VArray<MovieDef *> 		Movies;

    static const int 		PosterWidth;
    static const int 		PosterHeight;

    static const char *		SupportedFormats[];

private:
	CinemaApp &				Cinema;

	void					LoadMovies();
	MovieFormat				FormatFromString( const VString &formatString ) const;
	MovieCategory 			CategoryFromString( const VString &categoryString ) const;
	void 					ReadMetaData( MovieDef *movie );
	void 					LoadPoster( MovieDef *movie );
	void 					MoviesInDirectory(VArray<VString> &movies, const VString &dirName ) const;
	VArray<VString> 			ScanMovieDirectories() const;
	bool					IsSupportedMovieFormat( const VString &extension ) const;
};

} // namespace OculusCinema

