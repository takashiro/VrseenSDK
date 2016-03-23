#include <Alg.h>
#include <VPath.h>

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
    Title = VPath(url).baseName();
}

OvrMetaDatum * OvrVideosMetaData::createMetaDatum( const char* url ) const
{
	return new OvrVideosMetaDatum( url );
}

void OvrVideosMetaData::extractExtendedData( const VJson &jsonDatum, OvrMetaDatum & datum ) const
{
	OvrVideosMetaDatum * videoData = static_cast< OvrVideosMetaDatum * >( &datum );
	if ( videoData )
	{
        videoData->Title 					= jsonDatum.value( TITLE_INNER ).toString();
        videoData->Author 					= jsonDatum.value( AUTHOR_INNER ).toString();
        videoData->ThumbnailUrl 			= jsonDatum.value( THUMBNAIL_URL_INNER ).toString();
        videoData->StreamingType 			= jsonDatum.value( STREAMING_TYPE_INNER ).toString();
        videoData->StreamingProxy 			= jsonDatum.value( STREAMING_PROXY_INNER ).toString();
        videoData->StreamingSecurityLevel 	= jsonDatum.value( STREAMING_SECURITY_LEVEL_INNER ).toString();

		if ( videoData->Title.isEmpty() )
		{
            videoData->Title = VPath(datum.url).baseName();
		}

		if ( videoData->Author.isEmpty() )
		{
			videoData->Author = DEFAULT_AUTHOR_NAME;
		}
	}
}

void OvrVideosMetaData::extendedDataToJson(const OvrMetaDatum & datum, VJsonObject &outDatumObject ) const
{
    const OvrVideosMetaDatum * const videoData = static_cast< const OvrVideosMetaDatum * const >( &datum );
    if ( videoData ) {
        outDatumObject.insert(TITLE_INNER, videoData->Title);
        outDatumObject.insert(AUTHOR_INNER, videoData->Author);
        outDatumObject.insert(THUMBNAIL_URL_INNER, videoData->ThumbnailUrl);
        outDatumObject.insert(STREAMING_TYPE_INNER, videoData->StreamingType);
        outDatumObject.insert(STREAMING_PROXY_INNER, videoData->StreamingProxy);
        outDatumObject.insert(STREAMING_SECURITY_LEVEL_INNER, videoData->StreamingSecurityLevel);
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
