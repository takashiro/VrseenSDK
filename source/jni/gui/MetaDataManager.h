#pragma once

#include "VString.h"
#include "VStringHash.h"

#include "VJson.h"
#include "VArray.h"
#include "VPath.h"
NV_NAMESPACE_BEGIN

struct OvrMetaDatum
{
    int id;
    VArray<VString> tags;
    VPath url;

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
    VArray< VString > goodExtensions;
    VArray< VString > badExtensions;
};

class OvrMetaData
{
public:
	struct Category
	{
		Category()
            : dirty( true )
		{}
        VString 			categoryTag;
        VString 			label;
        VArray< int > 	datumIndicies;
        bool 			dirty;
	};

	OvrMetaData()
        : m_version( -1.0 )
	{}

	virtual ~OvrMetaData() {}

	// Init meta data from contents on disk
    void					initFromDirectory( const char * relativePath, const VArray< VString > & searchPaths, const OvrMetaDataFileExtensions & fileExtensions );

	// Init meta data from a passed in list of files
    void					initFromFileList( const VArray< VString > & fileList, const OvrMetaDataFileExtensions & fileExtensions );

	// Check specific paths for media and reconcile against stored/new metadata (Maintained for SDK)
    void					initFromDirectoryMergeMeta( const char * relativePath, const VArray< VString > & searchPaths,
		const OvrMetaDataFileExtensions & fileExtensions, const char * metaFile, const char * packageName );

	// File list passed in and we reconcile against stored/new metadata
    void					initFromFileListMergeMeta( const VArray< VString > & fileList, const VArray< VString > & searchPaths,
        const OvrMetaDataFileExtensions & fileExtensions, const char * appFileStoragePath, const char * metaFile, const VJson &storedMetaData );

    void					processRemoteMetaFile( const char * metaFileString, const int startInsertionIndex /* index to insert remote categories*/ );

	// Rename a category after construction
    void					renameCategory( const VString &currentTag, const VString &newName );

	// Adds or removes tag and returns action taken
    TagAction				toggleTag( OvrMetaDatum * data, const VString & tag );

	// Returns metaData file if one is found, otherwise creates one using the default meta.json in the assets folder
    VJson createOrGetStoredMetaFile(const VString &appFileStoragePath, const char * metaFile );
    void					addCategory( const VString & name );

    const VArray< Category > categories() const 							{ return m_categories; }
    const Category & 		getCategory( const int index ) const 			{ return m_categories.at( index ); }
    Category & 				getCategory( const int index )   				{ return m_categories[index]; }
    const OvrMetaDatum &	getMetaDatum( const int index ) const;
    bool 					getMetaData( const Category & category, VArray< const OvrMetaDatum * > & outMetaData ) const;
    void					setCategoryDatumIndicies( const int index, const VArray< int >& datumIndicies );

protected:
	// Overload to fill extended data during initialization
    virtual OvrMetaDatum *	createMetaDatum( const char* fileName ) const = 0;
    virtual	void			extractExtendedData( const VJson &jsonDatum, OvrMetaDatum & outDatum ) const = 0;
    virtual	void			extendedDataToJson( const OvrMetaDatum & datum, VJsonObject &outDatumObject ) const = 0;
    virtual void			swapExtendedData( OvrMetaDatum * left, OvrMetaDatum * right ) const = 0;

	// Optional protected interface
    virtual bool			isRemote( const OvrMetaDatum * datum ) const	{	return true; }

	// Removes duplicate entries from newData
    virtual void            dedupMetaData( const VArray< OvrMetaDatum * > & existingData, VStringHash< OvrMetaDatum * > & newData );

private:
    Category * 				getCategory( const VString & categoryName );
    void					processMetaData(const VJsonObject &dataFile, const VArray< VString > & searchPaths, const char * metaFile );
    void					regenerateCategoryIndices();
    void					reconcileMetaData( VStringHash< OvrMetaDatum * > & storedMetaData );
    void					reconcileCategories( VArray< Category > & storedCategories );

    VJson			metaDataToJson() const;
    void					writeMetaFile( const char * metaFile ) const;
    bool 					shouldAddFile( const char * filename, const OvrMetaDataFileExtensions & fileExtensions ) const;
    void					extractVersion(const VJsonObject &dataFile, double & outVersion ) const;
    void					extractCategories(const VJsonObject &dataFile, VArray< Category > & outCategories ) const;
    void					extractMetaData(const VJsonObject &dataFile, const VArray< VString > & searchPaths, VStringHash< OvrMetaDatum * > & outMetaData ) const;
    void					extractRemoteMetaData( const VJson &dataFile, VStringHash< OvrMetaDatum * > & outMetaData ) const;

    VPath 					m_filePath;
    VArray< Category >		m_categories;
    VArray< OvrMetaDatum * >	m_etaData;
    VStringHash< int >		m_urlToIndex;
    double					m_version;
};

NV_NAMESPACE_END
