/************************************************************************************

Filename    :   PanoBrowser.cpp
Content     :
Created     :
Authors     :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Oculus360Photos/ directory. An additional grant
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "PanoBrowser.h"
#include "String_Utils.h"
#include "Oculus360Photos.h"
#include "ImageData.h"
#include "VrLocale.h"
#include "BitmapFont.h"
#include "gui/GuiSys.h"
#include "PhotosMetaData.h"
#include "OVR_TurboJpeg.h"
#include "linux/stat.h"
#include "VrCommon.h"

#include <unistd.h>
#include <VPath.h>
#include "VDir.h"
namespace NervGear
{

static const int FAVORITES_FOLDER_INDEX = 0;


// Takes a <name_nz.jpg file and adds half of the _px and _nx images
// to make a 2:1 aspect image for a thumbnail with the images reversed
// as needed to make it look "forward"
//
// Note that this won't be an actual wide-angle projection, but it is at least continuous
// and somewhat reasonable looking.
unsigned char * CubeMapVista( const char * nzName, float const ratio, int & width, int & height )
{
	int	faceWidth = 0;
	int	faceHeight = 0;
	int	faceWidth2 = 0;
	int	faceHeight2 = 0;
	int	faceWidth3 = 0;
	int	faceHeight3 = 0;

	width = height = 0;

	unsigned char * nzData = NULL;
	unsigned char * pxData = NULL;
	unsigned char * nxData = NULL;
	unsigned char * combined = NULL;

	// make _nx.jpg and _px.jpg from provided _nz.jpg name
	const int len = strlen( nzName );
	if ( len < 6 )
	{
		return NULL;
	}
    VString	pxName( nzName );
    pxName[len - 6] = 'p';
    pxName[len - 5] = 'x';
    VString nxName( nzName );
    nxName[len - 5] = 'x';

    const char *pxNameStr = pxName.toCString();
    const char *nxNameStr = nxName.toCString();
	nzData = TurboJpegLoadFromFile( nzName, &faceWidth, &faceHeight );
    pxData = TurboJpegLoadFromFile( pxNameStr, &faceWidth2, &faceHeight2 );
    nxData = TurboJpegLoadFromFile( nxNameStr, &faceWidth3, &faceHeight3 );
    delete[] pxNameStr;
    delete[] nxNameStr;

	if ( nzData && pxData && nxData && ( faceWidth & 1 ) == 0 && faceWidth == faceHeight
		&& faceWidth2 == faceWidth && faceHeight2 == faceHeight
		&& faceWidth3 == faceWidth && faceHeight3 == faceHeight )
	{
		// find the total width and make sure it's an even number
		width = ( ( int )( floor( faceWidth * ratio ) ) >> 1 ) << 1;
		// subract the width of the center face, and split that into two halves
		int const half = ( width - faceWidth ) >> 1;
		height = faceHeight;
		combined = ( unsigned char * )malloc( width * height * 4 );
		for ( int y = 0; y < height; y++ )
		{
			int * dest = ( int * )combined + y * width;
			int * nz = ( int * )nzData + y * faceWidth;
			int * px = ( int * )pxData + y * faceWidth;
			int * nx = ( int * )nxData + y * faceWidth;
			for ( int x = 0; x < half; x++ )
			{
				dest[ x ] = nx[ half - 1 - x ];
				dest[ half + faceWidth + x ] = px[ faceWidth - 1 - x ];
			}
			for ( int x = 0; x < faceWidth; x++ )
			{
				dest[ half + x ] = nz[ faceWidth - 1 - x ];
			}
		}
	}

	if ( nzData )
	{
		free( nzData );
	}
	if ( nxData )
	{
		free( nxData );
	}
	if ( pxData )
	{
		free( pxData );
	}

	return combined;
}

PanoBrowser * PanoBrowser::Create(
	App *					app,
	OvrMetaData &			metaData,
	unsigned				thumbWidth,
	float					horizontalPadding,
	unsigned				thumbHeight,
	float					verticalPadding,
	unsigned 				numSwipePanels,
	float					swipeRadius )
{
	return new PanoBrowser(
		app,
		metaData,
		static_cast< float >( thumbWidth + horizontalPadding ),
		static_cast< float >( thumbHeight + verticalPadding ),
		swipeRadius,
		numSwipePanels,
		thumbWidth,
		thumbHeight );
}

VString PanoBrowser::getCategoryTitle( char const * key, char const * defaultStr ) const
{
	VString outStr;
    VrLocale::GetString(  m_app->vrJni(), m_app->javaObject(), key, defaultStr, outStr );
	return outStr;
}

VString PanoBrowser::getPanelTitle( const OvrMetaDatum & panelData ) const
{
	const OvrPhotosMetaDatum * const photosDatum = static_cast< const OvrPhotosMetaDatum * const >( &panelData );
	if ( photosDatum != NULL )
	{
		// look first in our own locale table for titles that were downloaded at run-time.
		VString outStr;
        VrLocale::GetString(  m_app->vrJni(), m_app->javaObject(), photosDatum->title.toCString(), photosDatum->title.toCString(), outStr );
		return outStr;
	}
	return VString();
}


void PanoBrowser::onPanelActivated( const OvrMetaDatum * panelData )
{
    Oculus360Photos * photos = ( Oculus360Photos * ) m_app->appInterface();
	OVR_ASSERT( photos );
	photos->onPanoActivated( panelData );
}

const OvrMetaDatum * PanoBrowser::nextFileInDirectory( const int step )
{
	// if currently browsing favorites, handle this here
    if ( activeFolderIndex() == FAVORITES_FOLDER_INDEX )
	{
        const int numFavorites = m_favoritesBuffer.length();
		// find the current
		int nextPanelIndex = -1;
        Oculus360Photos * photos = ( Oculus360Photos * )m_app->appInterface();
		OVR_ASSERT( photos );
		for ( nextPanelIndex = 0; nextPanelIndex < numFavorites; ++nextPanelIndex )
		{
            const Favorite & favorite = m_favoritesBuffer.at( nextPanelIndex );
			if ( photos->activePano() == favorite.data )
			{
				break;
			}
		}
		const OvrMetaDatum * panoData = NULL;
		for ( int count = 0; count < numFavorites; ++count )
		{
			nextPanelIndex += step;
			if ( nextPanelIndex >= numFavorites )
			{
				nextPanelIndex = 0;
			}
			else if ( nextPanelIndex < 0 )
			{
				nextPanelIndex = numFavorites - 1;
			}

            if ( m_favoritesBuffer.at( nextPanelIndex ).isFavorite )
			{
                panoData = m_favoritesBuffer.at( nextPanelIndex ).data;
				break;
			}
		}

		if ( panoData )
		{
            setCategoryRotation( activeFolderIndex(), nextPanelIndex );
		}

		return panoData;
	}
	else // otherwise pass it to base
	{
        return OvrFolderBrowser::nextFileInDirectory( step );
	}
}

// When browser opens - load in whatever is in local favoritebuffer to favorites folder
void PanoBrowser::onBrowserOpen()
{
	// When browser opens - load in whatever is in local favoritebuffer to favorites folder
	VArray< const OvrMetaDatum * > favoriteData;
	for ( int i = 0; i < m_favoritesBuffer.length(); ++i )
	{
		const Favorite & favorite = m_favoritesBuffer.at( i );
		if ( favorite.isFavorite )
		{
			favoriteData.append( favorite.data );
		}
	}

	if ( m_bufferDirty )
	{
        Oculus360Photos * photos = ( Oculus360Photos * )m_app->appInterface();
		if ( photos )
		{
            OvrPhotosMetaData * metaData = photos->metaData();
			if ( metaData )
			{
				rebuildFolder( *metaData, FAVORITES_FOLDER_INDEX, favoriteData );
				m_bufferDirty = false;
			}
		}
	}

	if ( activeFolderIndex() == FAVORITES_FOLDER_INDEX )
	{
		// Do we have any favorites at all?
		bool haveAnyFavorite = false;
        for ( int i = 0; i < m_favoritesBuffer.length(); ++i )
		{
            const Favorite & favorite = m_favoritesBuffer.at( i );
			if ( favorite.isFavorite )
			{
				haveAnyFavorite = true;
				break;
			}
		}

		// Favorites is empty and active folder - hide it in FolderBrowser by scrolling down
		if ( !haveAnyFavorite )
		{
			LOG( "PanoBrowser::AddToFavorites setting OvrFolderBrowser::MOVE_ROOT_DOWN" );
            setRootAdjust( OvrFolderBrowser::MOVE_ROOT_UP );
		}
	}
}

// Reload with what's currently in favorites folder in FolderBrowser
void PanoBrowser::ReloadFavoritesBuffer()
{
    Oculus360Photos * photos = ( Oculus360Photos * )m_app->appInterface();
	if ( photos != NULL )
	{
        OvrPhotosMetaData * metaData = photos->metaData();
		if ( metaData != NULL )
		{
			VArray< const OvrMetaDatum * > favoriteData;
            const OvrMetaData::Category & favCat = metaData->getCategory( 0 );
            if ( !metaData->getMetaData( favCat, favoriteData ) )
			{
				return;
			}
            m_favoritesBuffer.clear();
            for ( int i = 0; i < favoriteData.length(); ++i )
			{
				Favorite favorite;
                favorite.data = favoriteData.at( i );
				favorite.isFavorite = true;
                m_favoritesBuffer.append( favorite );
			}
		}
	}
}

// Add a panel to an existing folder
void PanoBrowser::addToFavorites( const OvrMetaDatum * panoData )
{
	// Check if already in favorites
    for ( int i = 0; i < m_favoritesBuffer.length(); ++i )
	{
        Favorite & favorite = m_favoritesBuffer.at( i );
		if ( panoData == favorite.data )
		{
			// already in favorites, so make sure it's marked as a favorite
			if ( !favorite.isFavorite )
			{
				favorite.isFavorite = true;
				m_bufferDirty = true;
			}
			return;
		}
	}

	// Not found in the buffer so add it
	Favorite favorite;
	favorite.data = panoData;
	favorite.isFavorite = true;

    m_favoritesBuffer.append( favorite );
	m_bufferDirty = true;
}

// Remove panel from an existing folder
void PanoBrowser::removeFromFavorites( const OvrMetaDatum * panoData )
{
	// First check if in fav buffer, and if so mark it as not a favorite
    for ( int i = 0; i < m_favoritesBuffer.length(); ++i )
	{
        Favorite & favorite = m_favoritesBuffer.at( i );
		if ( panoData == favorite.data )
		{
			favorite.isFavorite = false;
			m_bufferDirty = true;
			break;
		}
	}
}

int PanoBrowser::numPanosInActive() const
{
    const int activeFolderIndex = this->activeFolderIndex();
	if ( activeFolderIndex == FAVORITES_FOLDER_INDEX )
	{
		int numFavs = 0;
        for ( int i = 0; i < m_favoritesBuffer.length(); ++i )
		{
            if ( m_favoritesBuffer.at( i ).isFavorite )
			{
				++numFavs;
			}
		}
		return numFavs;
	}
    const OvrFolderBrowser::FolderView * folder = getFolderView( activeFolderIndex );
	OVR_ASSERT( folder );
    return folder->panels.length();
}


unsigned char * PanoBrowser::createAndCacheThumbnail( const char * soureFile, const char * cacheDestinationFile, int & outW, int & outH )
{
    VDir vdir;
	int		width = 0;
	int		height = 0;
	unsigned char * data = NULL;

	if ( strstr( soureFile, "_nz.jpg" ) )
	{
		data = CubeMapVista( soureFile, 1.6f, width, height );
	}
	else
	{
		data = TurboJpegLoadFromFile( soureFile, &width, &height );
	}

	if ( !data )
	{
		return NULL;
	}

    outW = thumbWidth();
    outH = thumbHeight();

	unsigned char * outBuffer = NULL;

	// For most panos - this is a simple and fast sampling into the typically much larger image
	if ( width >= outW && height >= outH )
	{
		outBuffer = ( unsigned char * )malloc( outW * outW * 4 );

		const int xJump = width / outW;
		const int yJump = height / outH;

		for ( int y = 0; y < outH; y++ )
		{
			for ( int x = 0; x < outW; x++ )
			{
				for ( int c = 0; c < 4; c++ )
				{
					outBuffer[ ( y * outW + x ) * 4 + c ] = data[ ( y * yJump * width + ( x * xJump ) ) * 4 + c ];
				}
			}
		}
	}
	else // otherwise we let ScaleImageRGBA upscale ( for users that really really want a low res pano )
	{
		outBuffer = ScaleImageRGBA( data, width, height, outW, outH, IMAGE_FILTER_CUBIC );
	}
	free( data );

	// write it out to cache
	LOG( "thumb create - writjpeg %s %p %dx%d", cacheDestinationFile, data, outW, outH );
//	MakePath( cacheDestinationFile, S_IRUSR | S_IWUSR );
	vdir.makePath( cacheDestinationFile, S_IRUSR | S_IWUSR );
	if ( vdir.contains( cacheDestinationFile, W_OK ) )
	{
		WriteJpeg( cacheDestinationFile, outBuffer, outW, outH );
	}

	return outBuffer;
}

unsigned char * PanoBrowser::loadThumbnail( const char * filename, int & width, int & height )
{
	return TurboJpegLoadFromFile( filename, &width, &height );
}

VString PanoBrowser::thumbName( const VString & s )
{
    VPath ts(s);
    ts.stripTrailing(".x");
    ts.setExtension("thm");
	//ts += ".jpg";
	return ts;
}

void PanoBrowser::onMediaNotFound( App * app, VString & title, VString & imageFile, VString & message )
{
	VrLocale::GetString( app->vrJni(), app->javaObject(), "@string/app_name", "@string/app_name", title );
	imageFile = "assets/sdcard.png";
	VrLocale::GetString( app->vrJni(), app->javaObject(), "@string/media_not_found", "@string/media_not_found", message );
	BitmapFont & font = app->defaultFont();
    VArray<VString> wholeStrs;
    wholeStrs.append( "Gear VR" );
	font.WordWrapText( message, 1.4f, wholeStrs );
}

bool PanoBrowser::onTouchUpNoFocused()
{
    Oculus360Photos * photos = ( Oculus360Photos * )m_app->appInterface();
	OVR_ASSERT( photos );
    if ( photos->activePano() != NULL && isOpen() && !gazingAtMenu() )
	{
        m_app->guiSys().closeMenu( m_app, this, false );
		return true;
	}
	return false;
}

}
