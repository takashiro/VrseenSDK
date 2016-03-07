/************************************************************************************

Filename    :   VideosMetaData.cpp
Content     :   A class to manage metadata used by FolderBrowser
Created     :   February 19, 2015
Authors     :   Jonathan E. Wright, Warsam Osman, Madhu Kalva

Copyright   :   Copyright 2015 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/
#include <Alg.h>

#include "VideosMetaData.h"
#include "VrCommon.h"

using namespace NervGear;

namespace NervGear {

const char * const TITLE_INNER						= "title";
const char * const AUTHOR_INNER						= "author";
const char * const THUMBNAIL_URL_INNER 				= "thumbnail_url";
const char * const STREAMING_TYPE_INNER 			= "streaming_type";
const char * const STREAMING_PROXY_INNER 			= "streaming_proxy";
const char * const STREAMING_SECURITY_LEVEL_INNER 	= "streaming_security_level";
const char * const DEFAULT_AUTHOR_NAME				= "Unspecified Author";

OvrVideosMetaDatum::OvrVideosMetaDatum( const VString& url )
	: Author( DEFAULT_AUTHOR_NAME )
{
	Title = ExtractFileBase( url );
}

OvrMetaDatum * OvrVideosMetaData::createMetaDatum( const char* url ) const
{
	return new OvrVideosMetaDatum( url );
}

void OvrVideosMetaData::extractExtendedData( const Json &jsonDatum, OvrMetaDatum & datum ) const
{
	OvrVideosMetaDatum * videoData = static_cast< OvrVideosMetaDatum * >( &datum );
	if ( videoData )
	{
		videoData->Title 					= jsonDatum.value( TITLE_INNER ).toString().c_str();
		videoData->Author 					= jsonDatum.value( AUTHOR_INNER ).toString().c_str();
		videoData->ThumbnailUrl 			= jsonDatum.value( THUMBNAIL_URL_INNER ).toString().c_str();
		videoData->StreamingType 			= jsonDatum.value( STREAMING_TYPE_INNER ).toString().c_str();
		videoData->StreamingProxy 			= jsonDatum.value( STREAMING_PROXY_INNER ).toString().c_str();
		videoData->StreamingSecurityLevel 	= jsonDatum.value( STREAMING_SECURITY_LEVEL_INNER ).toString().c_str();

		if ( videoData->Title.isEmpty() )
		{
            videoData->Title = ExtractFileBase(datum.url);
		}

		if ( videoData->Author.isEmpty() )
		{
			videoData->Author = DEFAULT_AUTHOR_NAME;
		}
	}
}

void OvrVideosMetaData::extendedDataToJson( const OvrMetaDatum & datum, Json &outDatumObject ) const
{
    if ( outDatumObject.isObject() ) {
		const OvrVideosMetaDatum * const videoData = static_cast< const OvrVideosMetaDatum * const >( &datum );
        if ( videoData ) {
            outDatumObject.insert(TITLE_INNER, videoData->Title.toUtf8());
            outDatumObject.insert(AUTHOR_INNER, videoData->Author.toUtf8());
            outDatumObject.insert(THUMBNAIL_URL_INNER, videoData->ThumbnailUrl.toUtf8());
            outDatumObject.insert(STREAMING_TYPE_INNER, videoData->StreamingType.toUtf8());
            outDatumObject.insert(STREAMING_PROXY_INNER, videoData->StreamingProxy.toUtf8());
            outDatumObject.insert(STREAMING_SECURITY_LEVEL_INNER, videoData->StreamingSecurityLevel.toUtf8());
		}
	}
}

void OvrVideosMetaData::swapExtendedData( OvrMetaDatum * left, OvrMetaDatum * right ) const
{
	OvrVideosMetaDatum * leftVideoData = static_cast< OvrVideosMetaDatum * >( left );
	OvrVideosMetaDatum * rightVideoData = static_cast< OvrVideosMetaDatum * >( right );
	if ( leftVideoData && rightVideoData )
	{
        std::swap(leftVideoData->Title, rightVideoData->Title);
        std::swap(leftVideoData->Author, rightVideoData->Author);
	}
}

}
