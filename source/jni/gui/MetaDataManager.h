/************************************************************************************

Filename    :   MetaDataManager.h
Content     :   A class to manage metadata used by FolderBrowser
Created     :   January 26, 2015
Authors     :   Jonathan E. Wright, Warsam Osman, Madhu Kalva

Copyright   :   Copyright 2015 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#if !defined( OVR_MetaDataManager_h )
#define OVR_MetaDataManager_h

#include "Array.h"
#include "VString.h"
#include "StringHash.h"

#include "VJson.h"

namespace NervGear {

//==============================================================
// OvrMetaData
struct OvrMetaDatum
{
    int						id;
    Array< String >			tags;
    String					url;

protected:
	OvrMetaDatum() {}
};

enum TagAction
{
	TAG_ADDED,
	TAG_REMOVED,
	TAG_ERROR
};

struct OvrMetaDataFileExtensions
{
    Array< String > goodExtensions;
    Array< String > badExtensions;
};

class OvrMetaData
{
public:
	struct Category
	{
		Category()
            : dirty( true )
		{}
        String 			categoryTag;
        String 			label;
        Array< int > 	datumIndicies;
        bool 			dirty;
	};

	OvrMetaData()
        : m_version( -1.0 )
	{}

	virtual ~OvrMetaData() {}

	// Init meta data from contents on disk
    void					initFromDirectory( const char * relativePath, const Array< String > & searchPaths, const OvrMetaDataFileExtensions & fileExtensions );

	// Init meta data from a passed in list of files
    void					initFromFileList( const Array< String > & fileList, const OvrMetaDataFileExtensions & fileExtensions );

	// Check specific paths for media and reconcile against stored/new metadata (Maintained for SDK)
    void					initFromDirectoryMergeMeta( const char * relativePath, const Array< String > & searchPaths,
		const OvrMetaDataFileExtensions & fileExtensions, const char * metaFile, const char * packageName );

	// File list passed in and we reconcile against stored/new metadata
    void					initFromFileListMergeMeta( const Array< String > & fileList, const Array< String > & searchPaths,
		const OvrMetaDataFileExtensions & fileExtensions, const char * appFileStoragePath, const char * metaFile, const NervGear::Json &storedMetaData );

    void					processRemoteMetaFile( const char * metaFileString, const int startInsertionIndex /* index to insert remote categories*/ );

	// Rename a category after construction
    void					renameCategory( const char * currentTag, const char * newName );

	// Adds or removes tag and returns action taken
    TagAction				toggleTag( OvrMetaDatum * data, const String & tag );

	// Returns metaData file if one is found, otherwise creates one using the default meta.json in the assets folder
    NervGear::Json createOrGetStoredMetaFile( const char * appFileStoragePath, const char * metaFile );
    void					addCategory( const String & name );

    const Array< Category > categories() const 							{ return m_categories; }
    const Category & 		getCategory( const int index ) const 			{ return m_categories.at( index ); }
    Category & 				getCategory( const int index )   				{ return m_categories.at( index ); }
    const OvrMetaDatum &	getMetaDatum( const int index ) const;
    bool 					getMetaData( const Category & category, Array< const OvrMetaDatum * > & outMetaData ) const;
    void					setCategoryDatumIndicies( const int index, const Array< int >& datumIndicies );

protected:
	// Overload to fill extended data during initialization
    virtual OvrMetaDatum *	createMetaDatum( const char* fileName ) const = 0;
    virtual	void			extractExtendedData( const NervGear::Json &jsonDatum, OvrMetaDatum & outDatum ) const = 0;
    virtual	void			extendedDataToJson( const OvrMetaDatum & datum, NervGear::Json &outDatumObject ) const = 0;
    virtual void			swapExtendedData( OvrMetaDatum * left, OvrMetaDatum * right ) const = 0;

	// Optional protected interface
    virtual bool			isRemote( const OvrMetaDatum * datum ) const	{	return true; }

	// Removes duplicate entries from newData
    virtual void            dedupMetaData( const Array< OvrMetaDatum * > & existingData, StringHash< OvrMetaDatum * > & newData );

private:
    Category * 				getCategory( const String & categoryName );
    void					processMetaData( const NervGear::Json &dataFile, const Array< String > & searchPaths, const char * metaFile );
    void					regenerateCategoryIndices();
    void					reconcileMetaData( StringHash< OvrMetaDatum * > & storedMetaData );
    void					reconcileCategories( Array< Category > & storedCategories );

    NervGear::Json			metaDataToJson() const;
    void					writeMetaFile( const char * metaFile ) const;
    bool 					shouldAddFile( const char * filename, const OvrMetaDataFileExtensions & fileExtensions ) const;
    void					extractVersion( const NervGear::Json &dataFile, double & outVersion ) const;
    void					extractCategories( const NervGear::Json &dataFile, Array< Category > & outCategories ) const;
    void					extractMetaData( const NervGear::Json &dataFile, const Array< String > & searchPaths, StringHash< OvrMetaDatum * > & outMetaData ) const;
    void					extractRemoteMetaData( const NervGear::Json &dataFile, StringHash< OvrMetaDatum * > & outMetaData ) const;

    String 					m_filePath;
    Array< Category >		m_categories;
    Array< OvrMetaDatum * >	m_etaData;
    StringHash< int >		m_urlToIndex;
    double					m_version;
};

}

#endif // OVR_MetaDataManager_h
