/************************************************************************************

 Filename    :   PhotosMetaData.h
 Content     :   A class to manage metadata used by FolderBrowser
 Created     :   February 19, 2015
 Authors     :   Jonathan E. Wright, Warsam Osman, Madhu Kalva

 Copyright   :   Copyright 2015 Oculus VR, LLC. All Rights reserved.


 *************************************************************************************/

#if !defined( OVR_PhotosMetaData_h )
#define OVR_PhotosMetaData_h

#include <VJson.h>

#include <VString.h>
#include <gui/MetaDataManager.h>

namespace NervGear {

//==============================================================
// OvrPhotosMetaDatum
struct OvrPhotosMetaDatum: public OvrMetaDatum {
    String author;
    String title;

	OvrPhotosMetaDatum(const String& url);
};

//==============================================================
// OvrPhotosMetaData
class OvrPhotosMetaData: public OvrMetaData {
public:
	virtual ~OvrPhotosMetaData() {
	}

protected:
	OvrMetaDatum *createMetaDatum(const char* url) const override;
	void extractExtendedData( const NervGear::Json & jsonDatum, OvrMetaDatum & outDatum ) const override;
	void extendedDataToJson( const OvrMetaDatum & datum, NervGear::Json &outDatumObject ) const override;
	void swapExtendedData( OvrMetaDatum * left, OvrMetaDatum * right ) const override;
};

}

#endif // OVR_PhotosMetaData_h
