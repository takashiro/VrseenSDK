/************************************************************************************

Filename    :   VideoBrowser.cpp
Content     :
Created     :
Authors     :

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Oculus360Videos/ directory. An additional grant
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "VideoBrowser.h"
#include "GlTexture.h"

#include "Oculus360Videos.h"
#include "VrLocale.h"
#include "BitmapFont.h"
#include "OVR_TurboJpeg.h"
#include "linux/stat.h"
#include <unistd.h>

#include <VPath.h>
#include <VApkFile.h>

namespace NervGear
{

VideoBrowser * VideoBrowser::Create(
		App * app,
		OvrVideosMetaData & metaData,
		unsigned thumbWidth,
		float horizontalPadding,
		unsigned thumbHeight,
		float verticalPadding,
		unsigned 	numSwipePanels,
		float SwipeRadius )
{
	return new VideoBrowser( app, metaData,
		thumbWidth + horizontalPadding, thumbHeight + verticalPadding, SwipeRadius, numSwipePanels, thumbWidth, thumbHeight );
}

void VideoBrowser::onPanelActivated( const OvrMetaDatum * panelData )
{
	Oculus360Videos * videos = ( Oculus360Videos * )m_app->appInterface();
	vAssert( videos );
	videos->OnVideoActivated( panelData );
}

unsigned char * VideoBrowser::createAndCacheThumbnail(const VString &soureFile, const VString &cacheDestinationFile, int & outW, int & outH )
{
	// TODO
	return NULL;
}

unsigned char * VideoBrowser::loadThumbnail( const char * filename, int & width, int & height )
{
	vInfo("VideoBrowser::LoadThumbnail loading on" << filename);
	unsigned char * orig = NULL;

	if ( strstr( filename, "assets/" ) )
	{
		void * buffer = NULL;
        uint length = 0;
        const VApkFile &apk = VApkFile::CurrentApkFile();
        apk.read(filename, buffer, length);

		if ( buffer )
		{
			orig = TurboJpegLoadFromMemory( reinterpret_cast< const unsigned char * >( buffer ), length, &width, &height );
			free( buffer );
		}
	}
	else if ( strstr( filename, ".pvr" ) )
	{
		orig = LoadPVRBuffer( filename, width, height );
	}
	else
	{
		orig = TurboJpegLoadFromFile( filename, &width, &height );
	}

	if ( orig )
	{
		const int ThumbWidth = thumbWidth();
		const int ThumbHeight = thumbHeight();

		if ( ThumbWidth == width && ThumbHeight == height )
		{
			vInfo("VideoBrowser::LoadThumbnail skip resize on" << filename);
			return orig;
		}

        vInfo("VideoBrowser::LoadThumbnail resizing" << filename << "to" << ThumbWidth << ThumbHeight);
        uchar *outBuffer = VImage::ScaleRGBA( ( const unsigned char * )orig, width, height, ThumbWidth, ThumbHeight, IMAGE_FILTER_CUBIC );

		free( orig );

		if ( outBuffer )
		{
			width = ThumbWidth;
			height = ThumbHeight;

			return outBuffer;
		}
	}
	else
	{
		vInfo("Error: VideoBrowser::LoadThumbnail failed to load" << filename);
	}
	return NULL;
}

VString VideoBrowser::thumbName( const VString & s )
{
    VPath ts = s;
    ts.setExtension("pvr");
	return ts;
}

VString VideoBrowser::alternateThumbName( const VString & s )
{
    VPath ts = s;
    ts.setExtension("thm");
	return ts;
}

void VideoBrowser::onMediaNotFound( App * app, VString & title, VString & imageFile, VString & message )
{
	VrLocale::GetString( app->vrJni(), app->javaObject(), "@string/app_name", "@string/app_name", title );
	imageFile = "assets/sdcard.png";
	VrLocale::GetString( app->vrJni(), app->javaObject(), "@string/media_not_found", "@string/media_not_found", message );
	BitmapFont & font = app->defaultFont();
    NervGear::VArray< NervGear::VString > wholeStrs;
	wholeStrs.append( "Gear VR" );
	font.WordWrapText( message, 1.4f, wholeStrs );
}

VString VideoBrowser::getCategoryTitle(const VString &key, const VString &defaultStr) const
{
	VString outStr;
    VrLocale::GetString( m_app->vrJni(), m_app->javaObject(), key, defaultStr, outStr);
	return outStr;
}

VString VideoBrowser::getPanelTitle( const OvrMetaDatum & panelData ) const
{
	const OvrVideosMetaDatum * const videosDatum = static_cast< const OvrVideosMetaDatum * const >( &panelData );
	if ( videosDatum != NULL )
	{
		return videosDatum->Title;
	}
	return VString();
}

}
