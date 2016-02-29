/************************************************************************************

Filename    :   VideosMetaData.h
Content     :   A class to manage metadata used by 360 videos folder browser
Created     :   February 20, 2015
Authors     :   Jonathan E. Wright, Warsam Osman, Madhu Kalva

Copyright   :   Copyright 2015 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/


#if !defined( OVR_VideosMetaData_h )
#define OVR_VideosMetaData_h

#include "VString.h"
#include "gui/MetaDataManager.h"
#include "VJson.h"

namespace NervGear {

//==============================================================
// OvrVideosMetaDatum
struct OvrVideosMetaDatum : public OvrMetaDatum
{
	String	Author;
	String	Title;
	String	ThumbnailUrl;
	String  StreamingType;
	String  StreamingProxy;
	String  StreamingSecurityLevel;

	OvrVideosMetaDatum( const String& url );
};

//==============================================================
// OvrVideosMetaData
class OvrVideosMetaData : public OvrMetaData
{
public:
	virtual ~OvrVideosMetaData() {}

protected:
	OvrMetaDatum *createMetaDatum( const char* url ) const override;
	void extractExtendedData( const NervGear::Json &jsonDatum, OvrMetaDatum & outDatum ) const override;
	void extendedDataToJson( const OvrMetaDatum & datum, NervGear::Json &outDatumObject ) const override;
	void swapExtendedData( OvrMetaDatum * left, OvrMetaDatum * right ) const override;
};

}

#endif // OVR_VideosMetaData_h
