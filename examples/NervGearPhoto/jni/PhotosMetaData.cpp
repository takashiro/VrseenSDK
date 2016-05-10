/************************************************************************************

Filename    :   PhotosMetaData.cpp
Content     :   A class to manage metadata used by FolderBrowser
Created     :   February 19, 2015
Authors     :   Jonathan E. Wright, Warsam Osman, Madhu Kalva

Copyright   :   Copyright 2015 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/

#include "PhotosMetaData.h"

#include <VAlgorithm.h>
#include <VPath.h>


namespace NervGear {

const char * const TITLE_INNER			= "title";
const char * const AUTHOR_INNER			= "author";
const char * const DEFAULT_AUTHOR_NAME	= "Unspecified Author";

OvrPhotosMetaDatum::OvrPhotosMetaDatum(const VString &url)
	: author( DEFAULT_AUTHOR_NAME )
{
    title = VPath(url).baseName();
}

OvrMetaDatum * OvrPhotosMetaData::createMetaDatum(const VString &url) const
{
	return new OvrPhotosMetaDatum( url );
}

void OvrPhotosMetaData::extractExtendedData( const NervGear::VJson & jsonDatum, OvrMetaDatum & datum ) const
{
	OvrPhotosMetaDatum * photoData = static_cast< OvrPhotosMetaDatum * >( &datum );
	if ( photoData )
	{
        photoData->title = jsonDatum.value( TITLE_INNER ).toString();
        photoData->author = jsonDatum.value( AUTHOR_INNER ).toString();

		if ( photoData->title.isEmpty() )
		{
            photoData->title = datum.url.baseName();
		}

		if ( photoData->author.isEmpty() )
		{
			photoData->author = DEFAULT_AUTHOR_NAME;
		}
	}
}

void OvrPhotosMetaData::extendedDataToJson( const OvrMetaDatum & datum, VJsonObject &outDatumObject ) const
{
    const OvrPhotosMetaDatum * const photoData = static_cast< const OvrPhotosMetaDatum * const >( &datum );
    if ( photoData )
    {
        outDatumObject.insert(TITLE_INNER, photoData->title);
        outDatumObject.insert(AUTHOR_INNER, photoData->author);
    }
}

void OvrPhotosMetaData::swapExtendedData( OvrMetaDatum * left, OvrMetaDatum * right ) const
{
	OvrPhotosMetaDatum * leftPhotoData = static_cast< OvrPhotosMetaDatum * >( left );
	OvrPhotosMetaDatum * rightPhotoData = static_cast< OvrPhotosMetaDatum * >( right );
	if ( leftPhotoData && rightPhotoData )
	{
        std::swap(leftPhotoData->title, rightPhotoData->title);
        std::swap(leftPhotoData->author, rightPhotoData->author);
	}
}

}
