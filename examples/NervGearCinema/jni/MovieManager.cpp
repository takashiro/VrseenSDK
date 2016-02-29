/************************************************************************************

Filename    :   MovieManager.cpp
Content     :
Created     :	9/10/2014
Authors     :   Jim Dos�

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <fstream>

#include "String_Utils.h"
#include "VJson.h"

#include "MovieManager.h"
#include "CinemaApp.h"
#include "PackageFiles.h"
#include "Native.h"


namespace OculusCinema {

const int MovieManager::PosterWidth = 228;
const int MovieManager::PosterHeight = 344;

static const char * searchDirs[] =
{
	"DCIM",
	"Movies",
	"Oculus/Movies",
	NULL
};

const char *MovieManager::SupportedFormats[] =
{
	".mp4",
	".m4v",
	".3gp",
	".3g2",
	".ts",
	".webm",
	".mkv",
	".wmv",
	".asf",
	".avi",
	".flv",
	NULL
};

//=======================================================================================

MovieManager::MovieManager( CinemaApp &cinema ) :
    Movies(),
	Cinema( cinema )
{
}

MovieManager::~MovieManager()
{
}

void MovieManager::OneTimeInit( const char * launchIntent )
{
	LOG( "MovieManager::OneTimeInit" );
	const double start = ovr_GetTimeInSeconds();

	LoadMovies();

    LOG( "MovieManager::OneTimeInit: %i movies loaded, %3.1f seconds", Movies.sizeInt(), ovr_GetTimeInSeconds() - start );
}

void MovieManager::OneTimeShutdown()
{
	LOG( "MovieManager::OneTimeShutdown" );
}

void MovieManager::LoadMovies()
{
	LOG( "LoadMovies" );

	const double start = ovr_GetTimeInSeconds();

	Array<String> movieFiles = ScanMovieDirectories();
    LOG( "%i movies scanned, %3.1f seconds", movieFiles.sizeInt(), ovr_GetTimeInSeconds() - start );

    for( UPInt i = 0; i < movieFiles.size(); i++ )
	{
		MovieDef *movie = new MovieDef();
        Movies.append( movie );

		movie->Filename = movieFiles[ i ];

		// set reasonable defaults for when there's no metadata
        movie->Title = GetMovieTitleFromFilename( movie->Filename.toCString() );
        movie->Is3D = ( NULL != strstr( movie->Filename.toCString(), "/3D/" ) );
		movie->Format = VT_UNKNOWN;
		movie->Theater = "";

        if ( NULL != strstr( movie->Filename.toCString(), "/DCIM/" ) )
		{
			// Everything in the DCIM folder goes to my videos
			movie->Category = CATEGORY_MYVIDEOS;
			movie->AllowTheaterSelection = true;
		}
        else if ( NULL != strstr( movie->Filename.toCString(), "/Trailers/" ) )
		{
			movie->Category = CATEGORY_TRAILERS;
			movie->AllowTheaterSelection = true;
		}
		else
		{
			movie->Category = CATEGORY_MYVIDEOS;
			movie->AllowTheaterSelection = true;
		}

		ReadMetaData( movie );
		LoadPoster( movie );
	}

    LOG( "%i movies panels loaded, %3.1f seconds", Movies.sizeInt(), ovr_GetTimeInSeconds() - start );
}

MovieFormat MovieManager::FormatFromString( const String &formatString ) const
{
    String format = formatString.toUpper();
	if ( format == "2D" )
	{
		return VT_2D;
	}

	if ( ( format == "3D" ) || ( format == "3DLR" ) )
	{
		return VT_LEFT_RIGHT_3D;
	}

	if ( format == "3DLRF" )
	{
		return VT_LEFT_RIGHT_3D_FULL;
	}

	if ( format == "3DTB" )
	{
		return VT_TOP_BOTTOM_3D;
	}

	if ( format == "3DTBF" )
	{
		return VT_TOP_BOTTOM_3D_FULL;
	}

	return VT_UNKNOWN;
}

MovieCategory MovieManager::CategoryFromString( const String &categoryString ) const
{
    String category = categoryString.toUpper();
	if ( category == "TRAILERS" )
	{
		return CATEGORY_TRAILERS;
	}

	return CATEGORY_MYVIDEOS;
}

void MovieManager::ReadMetaData( MovieDef *movie )
{
	String filename = movie->Filename;
    filename.stripExtension();
    filename.append( ".txt" );

	const char* error = NULL;

    if ( !Cinema.fileExists( filename.toCString() ) )
	{
		return;
	}

    Json metaData;
    std::ifstream fs(filename.toCString(), std::ios::binary);
    fs >> metaData;
    if (metaData.isValid()) {
        if (metaData.contains("title")) {
            movie->Title = metaData.value("title").toString();
		}

        if ( metaData.contains( "format" ) )
		{
            movie->Format = FormatFromString( metaData.value("format").toString() );
			movie->Is3D = ( ( movie->Format != VT_UNKNOWN ) && ( movie->Format != VT_2D ) );
		}

        if ( metaData.contains( "theater" ) )
		{
            movie->Theater = metaData.value("theater").toString();
		}

        if ( metaData.contains( "category" ) )
		{
            movie->Category = CategoryFromString( metaData.value("category").toString() );
		}

        if ( metaData.contains("encrypted"))
		{
            movie->IsEncrypted = metaData.value("encrypted").toBool();
        }

        LOG( "Loaded metadata: %s", filename.toCString() );
	}
	else
	{
        LOG( "Error loading metadata for %s: %s", filename.toCString(), ( error == NULL ) ? "NULL" : error );
	}
}

void MovieManager::LoadPoster( MovieDef *movie )
{
	String posterFilename = movie->Filename;
    posterFilename.stripExtension();
    posterFilename.append( ".png" );

    movie->Poster = LoadTextureFromBuffer( posterFilename.toCString(), MemBufferFile( posterFilename.toCString() ),
			TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), movie->PosterWidth, movie->PosterHeight );

	if ( movie->Poster == 0 )
	{
        if ( Cinema.isExternalSDCardDir( posterFilename.toCString() ) )
		{
			// Since we're unable to write to the external sd card and writing to the
			// cache directory doesn't seem to work, just disable generation of
			// thumbnails for the external sd card.
#if 0
			// we can't write to external sd cards, so change the filename to be in the cache
			posterFilename = Native::GetExternalCacheDirectory( Cinema.app ) + posterFilename;

			// check if we have the thumbnail in the cache
            movie->Poster = LoadTextureFromBuffer( posterFilename.toCString(), MemBufferFile( posterFilename.toCString() ),
				TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), movie->PosterWidth, movie->PosterHeight );

			if ( movie->Poster == 0 )
			{
                LOG( "No thumbnail found at %s", posterFilename.toCString() );
			}
#endif
		}
		else
		{
			// no thumbnail found, so create it.  if it's on an external sdcard, posterFilename will contain the new filename at this point and will load it from the cache
            if ( ( movie->Poster == 0 ) && Native::CreateVideoThumbnail( Cinema.app, movie->Filename.toCString(), posterFilename.toCString(), PosterWidth, PosterHeight ) )
			{
                movie->Poster = LoadTextureFromBuffer( posterFilename.toCString(), MemBufferFile( posterFilename.toCString() ),
					TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), movie->PosterWidth, movie->PosterHeight );
			}
		}
	}

	// if all else failed, then just use the default poster
	if ( movie->Poster == 0 )
	{
		movie->Poster = LoadTextureFromApplicationPackage( "assets/default_poster.png",
				TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), movie->PosterWidth, movie->PosterHeight );
	}

	BuildTextureMipmaps( movie->Poster );
	MakeTextureTrilinear( movie->Poster );
	MakeTextureClamped( movie->Poster );
}

bool MovieManager::IsSupportedMovieFormat( const String &extension ) const
{
	for( int i = 0; SupportedFormats[ i ] != NULL; i++ )
	{
		if ( extension == SupportedFormats[ i ] )
		{
			return true;
		}
	}
	return false;
}

void MovieManager::MoviesInDirectory( Array<String> &movies, const char * dirName ) const {
	LOG( "scanning directory: %s", dirName );
	DIR * dir = opendir( dirName );
	if ( dir != NULL )
	{
		struct dirent * entry;
        struct stat st;
		while( ( entry = readdir( dir ) ) != NULL ) {
	        if ( ( strcmp( entry->d_name, "." ) == 0 ) || ( strcmp( entry->d_name, ".." ) == 0 ) )
	        {
	            continue;
	        }

	        if ( fstatat( dirfd( dir ), entry->d_name, &st, 0 ) < 0 )
	        {
	        	LOG( "fstatat error on %s: %s", entry->d_name, strerror( errno ) );
	            continue;
	        }

	        if ( S_ISDIR( st.st_mode ) )
	        {
	        	char subDir[ 1000 ];
	        	StringUtils::SPrintf( subDir, "%s/%s", dirName, entry->d_name );
	        	MoviesInDirectory( movies, subDir );
	        	continue;
	        }

	        // skip files that begin with "._" since they get created
	        // when you copy movies onto the phones using Macs.
	        if ( strncmp( entry->d_name, "._", 2 ) == 0 )
	        {
	        	continue;
	        }

			String filename = entry->d_name;
            String ext = filename.extension().toLower();
			if ( IsSupportedMovieFormat( ext ) )
			{
				String fullpath = dirName;
                fullpath.append( "/" );
                fullpath.append( filename );
                LOG( "Adding movie: %s", fullpath.toCString() );
                movies.append( fullpath );
			}
		}

		closedir( dir );
	}
}

Array<String> MovieManager::ScanMovieDirectories() const {
	Array<String> movies;

	for( int i = 0; searchDirs[ i ] != NULL; i++ )
	{
		MoviesInDirectory( movies, Cinema.externalRetailDir( searchDirs[ i ] ) );
		MoviesInDirectory( movies, Cinema.retailDir( searchDirs[ i ] ) );
		MoviesInDirectory( movies, Cinema.sdcardDir( searchDirs[ i ] ) );
		MoviesInDirectory( movies, Cinema.externalSDCardDir( searchDirs[ i ] ) );
	}

	return movies;
}

const String MovieManager::GetMovieTitleFromFilename( const char *filepath )
{
	String filename = StringUtils::GetFileBaseString( filepath );

	// change _ to space
    String displayName = StringUtils::ReplaceChar( filename.toCString(), '_', ' ' );
	return displayName;
}

Array<const MovieDef *> MovieManager::GetMovieList( MovieCategory category ) const
{
	Array<const MovieDef *> result;

    for( UPInt i = 0; i < Movies.size(); i++ )
	{
		if ( Movies[ i ]->Category == category )
		{
            result.append( Movies[ i ] );
		}
	}

	return result;
}

} // namespace OculusCinema
