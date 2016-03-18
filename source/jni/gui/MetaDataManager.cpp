/************************************************************************************

Filename    :   MetaDataManager.cpp
Content     :   A class to manage metadata used by FolderBrowser
Created     :   January 26, 2015
Authors     :   Jonathan E. Wright, Warsam Osman, Madhu Kalva

Copyright   :   Copyright 2015 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#include "MetaDataManager.h"
#include "Alg.h"
#include "Android/LogUtils.h"

#include "VrCommon.h"
#include "unistd.h"

#include "VPath.h"
#include "VApkFile.h"
#include "VJson.h"

#include <fstream>

using namespace NervGear;

namespace NervGear {

//==============================
// OvrMetaData

const char * const VERSION = "Version";
const char * const CATEGORIES = "Categories";
const char * const DATA = "Data";
const char * const TITLE = "Title";
const char * const URL = "Url";
const char * const FAVORITES_TAG = "Favorites";
const char * const TAG = "tag";
const char * const LABEL = "label";
const char * const TAGS = "tags";
const char * const CATEGORY = "category";
const char * const URL_INNER = "url";

static bool OvrMetaDatumIdComparator( const OvrMetaDatum * a, const OvrMetaDatum * b)
{
	return a->id < b->id;
}

void OvrMetaData::initFromDirectory( const char * relativePath, const VArray< VString > & searchPaths, const OvrMetaDataFileExtensions & fileExtensions )
{
	LOG( "OvrMetaData::InitFromDirectory( %s )", relativePath );

	// Find all the files - checks all search paths
	StringHash< VString > uniqueFileList = RelativeDirectoryFileList( searchPaths, relativePath );
	VArray<VString> fileList;
    for (const std::pair<VString, VString> &iter : uniqueFileList) {
        fileList.append(iter.first);
	}
	SortStringArray( fileList );
	Category currentCategory;
    currentCategory.categoryTag = VPath(relativePath).baseName();
	// The label is the same as the tag by default.
	//Will be replaced if definition found in loaded metadata
	currentCategory.label = currentCategory.categoryTag;

	LOG( "OvrMetaData start category: %s", currentCategory.categoryTag.toCString() );
	VArray< VString > subDirs;
	// Grab the categories and loose files
	for ( int i = 0; i < fileList.length(); i++ )
	{
		const VString & s = fileList[ i ];
        const VString fileBase = VPath(s).baseName();
		// subdirectory - add category
        if (s.endsWith('/')) {
            subDirs.append(s);
			continue;
		}

		// See if we want this loose-file
		if ( !shouldAddFile( s.toCString(), fileExtensions ) )
		{
			continue;
		}

		// Add loose file
		const int dataIndex = m_etaData.length();
        OvrMetaDatum * datum = createMetaDatum( fileBase.toCString() );
		if ( datum )
		{
			datum->id = dataIndex;
			datum->tags.append( currentCategory.categoryTag );
			if ( GetFullPath( searchPaths, s, datum->url ) )
			{
                StringHash<int>::const_iterator iter = m_urlToIndex.find(datum->url);
                if ( iter == m_urlToIndex.end() ) {
                    m_urlToIndex.insert(datum->url, dataIndex);
                    m_etaData.append(datum);
					LOG( "OvrMetaData adding datum %s with index %d to %s", datum->url.toCString(), dataIndex, currentCategory.categoryTag.toCString() );
					// Register with category
					currentCategory.datumIndicies.append( dataIndex );
                } else {
					WARN( "OvrMetaData::InitFromDirectory found duplicate url %s", datum->url.toCString() );
				}
			}
			else
			{
				WARN( "OvrMetaData::InitFromDirectory failed to find %s", s.toCString() );
			}
		}
	}

	if ( !currentCategory.datumIndicies.isEmpty() )
	{
		m_categories.append( currentCategory );
	}

	// Recurse into subdirs
	for ( int i = 0; i < subDirs.length(); ++i )
	{
		const VString & subDir = subDirs.at( i );
        initFromDirectory( subDir.toCString(), searchPaths, fileExtensions );
	}
}

void OvrMetaData::initFromFileList( const VArray< VString > & fileList, const OvrMetaDataFileExtensions & fileExtensions )
{
	// Create unique categories
	StringHash< int > uniqueCategoryList;
	for ( int i = 0; i < fileList.length(); ++i )
	{
		const VString & filePath = fileList.at( i );
        const VString categoryTag = VPath(fileList.at(i)).dirName();
        StringHash< int >::ConstIterator iter = uniqueCategoryList.find( categoryTag );
		int catIndex = -1;
        if ( iter == uniqueCategoryList.end() )
		{
			LOG( " category: %s", categoryTag.toCString() );
			Category cat;
			cat.categoryTag = categoryTag;
			// The label is the same as the tag by default.
			// Will be replaced if definition found in loaded metadata
			cat.label = cat.categoryTag;
			catIndex = m_categories.length();
			m_categories.append( cat );
            uniqueCategoryList.insert( categoryTag, catIndex );
		}
		else
		{
            catIndex = iter->second;
		}

		OVR_ASSERT( catIndex > -1 );
		Category & currentCategory = m_categories.at( catIndex );

		// See if we want this loose-file
		if ( !shouldAddFile( filePath.toCString(), fileExtensions ) )
		{
			continue;
		}

		// Add loose file

		const int dataIndex = m_etaData.length();
        OvrMetaDatum * datum = createMetaDatum( filePath.toCString() );

		if ( datum )
		{
			datum->id = dataIndex;
			datum->url = filePath;
			datum->tags.append( currentCategory.categoryTag );

            StringHash< int >::ConstIterator iter = m_urlToIndex.find( datum->url );
            if ( iter == m_urlToIndex.end() )
			{
                m_urlToIndex.insert( datum->url, dataIndex );
				m_etaData.append( datum );
				LOG( "OvrMetaData::InitFromFileList adding datum %s with index %d to %s", datum->url.toCString(),
					dataIndex, currentCategory.categoryTag.toCString() );
				// Register with category
				currentCategory.datumIndicies.append( dataIndex );
			}
			else
			{
				WARN( "OvrMetaData::InitFromFileList found duplicate url %s", datum->url.toCString() );
			}
		}
	}
}

void OvrMetaData::renameCategory(const VString &currentTag, const VString &newName )
{
	for ( int i = 0; i < m_categories.length(); ++i )
	{
		Category & cat = m_categories.at( i );
		if ( cat.categoryTag == currentTag )
		{
			cat.label = newName;
			break;
		}
	}
}

Json LoadPackageMetaFile( const char* metaFile )
{
    uint bufferLength = 0;
    void * 	buffer = NULL;
	VString assetsMetaFile = "assets/";
	assetsMetaFile += metaFile;
    const VApkFile &apk = VApkFile::CurrentApkFile();
    apk.read(assetsMetaFile, buffer, bufferLength);
    if ( buffer != nullptr )
	{
		WARN( "LoadPackageMetaFile failed to read %s", assetsMetaFile.toCString() );
	}
    return Json::Parse( static_cast<char*>(buffer) );
}

Json OvrMetaData::createOrGetStoredMetaFile( const VString &appFileStoragePath, const char * metaFile )
{
	m_filePath = appFileStoragePath;
	m_filePath += metaFile;

	LOG( "CreateOrGetStoredMetaFile FilePath: %s", m_filePath.toCString() );

    Json dataFile = Json::Load( m_filePath.toCString() );
	if ( dataFile.isInvalid() )
	{
		// If this is the first run, or we had an error loading the file, we copy the meta file from assets to app's cache
		writeMetaFile( metaFile );

		// try loading it again
        dataFile = Json::Load( m_filePath.toCString() );
		if ( dataFile.isInvalid() )
		{
			WARN( "OvrMetaData failed to load JSON meta file: %s", metaFile );
		}
	}
	else
	{
		LOG( "OvrMetaData::CreateOrGetStoredMetaFile found %s", m_filePath.toCString() );
	}
	return dataFile;
}

void OvrMetaData::writeMetaFile( const char * metaFile ) const
{
	LOG( "Writing metafile from apk" );

	if ( FILE * newMetaFile = fopen( m_filePath.toCString(), "w" ) )
	{
        uint bufferLength = 0;
		void * 	buffer = NULL;
		VString assetsMetaFile = "assets/";
		assetsMetaFile += metaFile;
        const VApkFile &apk = VApkFile::CurrentApkFile();
        apk.read(assetsMetaFile, buffer, bufferLength);
		if ( !buffer )
		{
			WARN( "OvrMetaData failed to read %s", assetsMetaFile.toCString() );
		}
		else
		{
            uint writtenCount = fwrite( buffer, 1, bufferLength, newMetaFile );
			if ( writtenCount != bufferLength )
			{
				FAIL( "OvrMetaData::WriteMetaFile failed to write %s", metaFile );
			}
			free( buffer );
		}
		fclose( newMetaFile );
	}
	else
	{
		FAIL( "OvrMetaData failed to create %s - check app permissions", m_filePath.toCString() );
	}
}

void OvrMetaData::initFromDirectoryMergeMeta( const char * relativePath, const VArray< VString > & searchPaths,
	const OvrMetaDataFileExtensions & fileExtensions, const char * metaFile, const char * packageName )
{
	LOG( "OvrMetaData::InitFromDirectoryMergeMeta" );

	VString appFileStoragePath = "/data/data/";
	appFileStoragePath += packageName;
	appFileStoragePath += "/files/";

	m_filePath = appFileStoragePath + metaFile;

	OVR_ASSERT( HasPermission( m_filePath, R_OK ) );

    Json dataFile = createOrGetStoredMetaFile( appFileStoragePath, metaFile );

    initFromDirectory( relativePath, searchPaths, fileExtensions );
	processMetaData( dataFile, searchPaths, metaFile );
}

void OvrMetaData::initFromFileListMergeMeta( const VArray< VString > & fileList, const VArray< VString > & searchPaths,
	const OvrMetaDataFileExtensions & fileExtensions, const char * appFileStoragePath, const char * metaFile, const NervGear::Json &storedMetaData )
{
	LOG( "OvrMetaData::InitFromFileListMergeMeta" );

    initFromFileList( fileList, fileExtensions );
	processMetaData( storedMetaData, searchPaths, metaFile );
}

void OvrMetaData::processRemoteMetaFile( const char * metaFileString, const int startIndex )
{
	Json remoteMetaFile = Json::Parse( metaFileString );
	if ( remoteMetaFile.isValid() )
	{
		// First grab the version
		double remoteVersion = 0.0;
		extractVersion( remoteMetaFile, remoteVersion );

		if ( remoteVersion <= m_version ) // We already have this metadata, don't need to process further
			return;

		m_version = remoteVersion;

		VArray< Category > remoteCategories;
		StringHash< OvrMetaDatum * > remoteMetaData;
		extractCategories( remoteMetaFile , remoteCategories );
		extractRemoteMetaData( remoteMetaFile , remoteMetaData );

		// Merge in the remote categories
		// Ignore any duplicate categories
		StringHash< bool > CurrentCategoriesSet; // using as set
		for ( int i = 0; i < m_categories.length(); ++i )
		{
			const Category & storedCategory = m_categories.at( i );
            CurrentCategoriesSet.insert( storedCategory.categoryTag, true );
		}

		for ( int remoteIndex = 0; remoteIndex < remoteCategories.length(); ++remoteIndex )
		{
			const Category & remoteCat = remoteCategories.at( remoteIndex );

            StringHash< bool >::ConstIterator iter = CurrentCategoriesSet.find( remoteCat.categoryTag );
            if ( iter == CurrentCategoriesSet.end() )
			{
				const int targetIndex = startIndex + remoteIndex;
				LOG( "OvrMetaData::ProcessRemoteMetaFile merging %s into category index %d", remoteCat.categoryTag.toCString(), targetIndex );
				if ( startIndex >= 0 && startIndex < m_categories.length() )
				{
					m_categories.insert( m_categories.begin()+targetIndex, remoteCat );
				}
				else
				{
					m_categories.append( remoteCat );
				}
			}
			else
			{
				LOG( "OvrMetaData::ProcessRemoteMetaFile discarding duplicate category %s", remoteCat.categoryTag.toCString() );
			}
		}

		// Append the remote data
		reconcileMetaData( remoteMetaData );

		// Recreate indices which may have changed after reconciliation
		regenerateCategoryIndices();

		// Serialize the new metadata
		Json dataFile = metaDataToJson();
		if ( dataFile.isValid() )
		{
			FAIL( "OvrMetaData::ProcessMetaData failed to generate JSON meta file" );
		}

        std::ofstream fp(m_filePath.toCString(), std::ios::binary);
		fp << dataFile;

		LOG( "OvrMetaData::ProcessRemoteMetaFile updated %s", m_filePath.toCString() );
	}
}

void OvrMetaData::processMetaData( const NervGear::Json &dataFile, const VArray< VString > & searchPaths, const char * metaFile )
{
	if ( dataFile.isValid() )
	{
		// Grab the version from the loaded data
		extractVersion( dataFile, m_version );

		VArray< Category > storedCategories;
		StringHash< OvrMetaDatum * > storedMetaData;
		extractCategories( dataFile, storedCategories );

		// Read in package data first
		Json packageMeta = LoadPackageMetaFile( metaFile );
		if ( packageMeta.isValid() )
		{
			// If we failed to find a version in the serialized data, need to set it from the assets version
			if ( m_version < 0.0 )
			{
				extractVersion( packageMeta, m_version );
				if ( m_version < 0.0 )
				{
					m_version = 0.0;
				}
			}
			extractCategories( packageMeta, storedCategories );
			extractMetaData( packageMeta, searchPaths, storedMetaData );
		}
		else
		{
			WARN( "ProcessMetaData LoadPackageMetaFile failed for %s", metaFile );
		}

		// Read in the stored data - overriding any found in the package
		extractMetaData( dataFile, searchPaths, storedMetaData );

		// Reconcile the stored data vs the data read in
		reconcileCategories( storedCategories );
		reconcileMetaData( storedMetaData );

		// Recreate indices which may have changed after reconciliation
		regenerateCategoryIndices();

		// Delete any newly empty categories except Favorites
		if ( !m_categories.isEmpty() )
		{
			VArray< Category > finalCategories;
			finalCategories.append( m_categories.at( 0 ) );
			for ( int catIndex = 1; catIndex < m_categories.length(); ++catIndex )
			{
				Category & cat = m_categories.at( catIndex );
				if ( !cat.datumIndicies.isEmpty() )
				{
					finalCategories.append( cat );
				}
				else
				{
					WARN( "OvrMetaData::ProcessMetaData discarding empty %s", cat.categoryTag.toCString() );
				}
			}
            std::swap(finalCategories, m_categories);
		}
	}
	else
	{
		WARN( "OvrMetaData::ProcessMetaData NULL dataFile" );
	}

	// Rewrite new data
	Json newDataFile = metaDataToJson();
	if ( newDataFile.isInvalid() )
	{
		FAIL( "OvrMetaData::ProcessMetaData failed to generate JSON meta file" );
	}

    std::ofstream fp(m_filePath.toCString(), std::ios::binary);
	fp << newDataFile;

	LOG( "OvrMetaData::ProcessMetaData created %s", m_filePath.toCString() );
}

void OvrMetaData::reconcileMetaData( StringHash< OvrMetaDatum * > & storedMetaData )
{
    if ( storedMetaData.isEmpty() )
	{
		return;
	}
    dedupMetaData( m_etaData, storedMetaData );

	// Now for any remaining stored data - check if it's remote and just add it, sorted by the
	// assigned Id
	VArray< OvrMetaDatum * > sortedEntries;
    StringHash< OvrMetaDatum * >::Iterator storedIter = storedMetaData.begin();
    for ( ; storedIter != storedMetaData.end(); ++storedIter )
	{
        OvrMetaDatum * storedDatum = storedIter->second;
		if ( isRemote( storedDatum ) )
		{
			LOG( "ReconcileMetaData metadata adding remote %s", storedDatum->url.toCString() );
			sortedEntries.append( storedDatum );
		}
	}
	Alg::QuickSortSlicedSafe( sortedEntries, 0, sortedEntries.size(), OvrMetaDatumIdComparator);
	VArray< OvrMetaDatum * >::iterator sortedIter = sortedEntries.begin();
	for ( ; sortedIter != sortedEntries.end(); ++sortedIter )
	{
		m_etaData.append( *sortedIter );
	}
    storedMetaData.clear();
}

void OvrMetaData::dedupMetaData( const VArray< OvrMetaDatum * > & existingData, StringHash< OvrMetaDatum * > & newData )
{
    // Fix the read in meta data using the stored
    for ( int i = 0; i < existingData.length(); ++i )
    {
        OvrMetaDatum * metaDatum = existingData.at( i );

        StringHash< OvrMetaDatum * >::Iterator iter = newData.find( metaDatum->url );

        if ( iter != newData.end() )
        {
            OvrMetaDatum * storedDatum = iter->second;
            LOG( "DedupMetaData metadata for %s", storedDatum->url.toCString() );
            std::swap(storedDatum->tags, metaDatum->tags);
            swapExtendedData( storedDatum, metaDatum );
            newData.remove( iter->first );
        }
    }
}

void OvrMetaData::reconcileCategories( VArray< Category > & storedCategories )
{
	if ( storedCategories.isEmpty() )
	{
		return;
	}

	// Reconcile categories
	// We want Favorites always at the top
	// Followed by user created categories
	// Finally we want to maintain the order of the retail categories (defined in assets/meta.json)
	VArray< Category > finalCategories;

	Category favorites = storedCategories.at( 0 );
	if ( favorites.categoryTag != FAVORITES_TAG )
	{
		WARN( "OvrMetaData::ReconcileCategories failed to find expected category order -- missing assets/meta.json?" );
	}

	finalCategories.append( favorites );

	StringHash< bool > StoredCategoryMap; // using as set
	for ( int i = 0; i < storedCategories.length(); ++i )
	{
		const Category & storedCategory = storedCategories.at( i );
		LOG( "OvrMetaData::ReconcileCategories storedCategory: %s", storedCategory.categoryTag.toCString() );
        StoredCategoryMap.insert( storedCategory.categoryTag, true );
	}

	// Now add the read in categories if they differ
	for ( int i = 0; i < m_categories.length(); ++i )
	{
		const Category & readInCategory = m_categories.at( i );
        StringHash< bool >::ConstIterator iter = StoredCategoryMap.find( readInCategory.categoryTag );

        if ( iter == StoredCategoryMap.end() )
		{
			LOG( "OvrMetaData::ReconcileCategories adding %s", readInCategory.categoryTag.toCString() );
			finalCategories.append( readInCategory );
		}
	}

	// Finally fill in the stored in categories after user made ones
	for ( int i = 1; i < storedCategories.length(); ++i )
	{
		const  Category & storedCat = storedCategories.at( i );
		LOG( "OvrMetaData::ReconcileCategories adding stored category %s", storedCat.categoryTag.toCString() );
		finalCategories.append( storedCat );
	}

	// Now replace Categories
    std::swap(m_categories, finalCategories);
}

void OvrMetaData::extractVersion(const Json &dataFile, double & outVersion ) const
{
	if ( dataFile.isInvalid() )
	{
		return;
	}

	if ( dataFile.isObject() )
	{
		outVersion = dataFile.value( VERSION ).toDouble();
	}
}

void OvrMetaData::extractCategories(const Json &dataFile, VArray< Category > & outCategories ) const
{
	if ( dataFile.isInvalid() )
	{
		return;
	}

	const Json &categories( dataFile.value( CATEGORIES ) );
	if ( categories.isArray() )
	{
        const JsonArray &elements = categories.toArray();
		for (const Json &category : elements) {
			if ( category.isObject() )
			{
				Category extractedCategory;
                extractedCategory.categoryTag = category.value( TAG ).toString();
                extractedCategory.label = category.value( LABEL ).toString();

				// Check if we already have this category
				bool exists = false;
				for ( int i = 0; i < outCategories.length(); ++i )
				{
					const Category & existingCat = outCategories.at( i );
					if ( extractedCategory.categoryTag == existingCat.categoryTag )
					{
						exists = true;
						break;
					}
				}

				if ( !exists )
				{
					LOG( "Extracting category: %s", extractedCategory.categoryTag.toCString() );
					outCategories.append( extractedCategory );
				}
			}
		}
	}
}

void OvrMetaData::extractMetaData(const Json &dataFile, const VArray< VString > & searchPaths, StringHash< OvrMetaDatum * > & outMetaData ) const
{
	if ( dataFile.isInvalid() )
	{
		return;
	}

	const Json &data( dataFile.value( DATA ) );
	if ( data.isArray() )
	{
		int jsonIndex = m_etaData.length();

        const JsonArray &datums = data.toArray();
		for (const Json &datum : datums) {
			if ( datum.isObject() )
			{
				OvrMetaDatum * metaDatum = createMetaDatum( "" );
				if ( !metaDatum )
				{
					continue;
				}

				metaDatum->id = jsonIndex++;
				const Json &tags( datum.value( TAGS ) );
				if ( tags.isArray() )
				{
                    const JsonArray &elements = tags.toArray();
					for (const Json &tag : elements) {
						if ( tag.isObject() )
						{
                            metaDatum->tags.append(tag.value( CATEGORY ).toString());
						}
					}
				}

				OVR_ASSERT( !metaDatum->tags.isEmpty() );

                const VString relativeUrl = datum.value( URL_INNER ).toString();
				metaDatum->url = relativeUrl;
				bool foundPath = false;
                const bool isRemote = this->isRemote( metaDatum );

				// Get the absolute path if this is a local file
				if ( !isRemote )
				{
					foundPath = GetFullPath( searchPaths, relativeUrl, metaDatum->url );
					if ( !foundPath )
					{
						// if we fail to find the file, check for encrypted extension (TODO: Might put this into a virtual function if necessary, benign for now)
						foundPath = GetFullPath( searchPaths, relativeUrl + ".x", metaDatum->url );
					}
				}

				// if we fail to find the local file or it's a remote file, the Url is left as read in from the stored data
				if ( isRemote || !foundPath )
				{
					metaDatum->url = relativeUrl;
				}

				extractExtendedData( datum, *metaDatum );
				LOG( "OvrMetaData::ExtractMetaData adding datum %s", metaDatum->url.toCString() );

                StringHash< OvrMetaDatum * >::Iterator iter = outMetaData.find( metaDatum->url );
                if ( iter == outMetaData.end() )
				{
                    outMetaData.insert( metaDatum->url, metaDatum );
				}
				else
				{
                    iter->second = metaDatum;
				}
			}
		}
	}
}

void OvrMetaData::extractRemoteMetaData( const Json &dataFile, StringHash< OvrMetaDatum * > & outMetaData ) const
{
	if ( dataFile.isInvalid() )
	{
		return;
	}

	const Json &data( dataFile.value( DATA ) );
	if ( data.isArray() )
	{
		int jsonIndex = m_etaData.length();

        const JsonArray elements = data.toArray();
		for (const Json &jsonDatum : elements) {
			if ( jsonDatum.isObject() )
			{
				OvrMetaDatum * metaDatum = createMetaDatum( "" );
				if ( !metaDatum )
				{
					continue;
				}
				metaDatum->id = jsonIndex++;
				const Json &tags( jsonDatum.value( TAGS ) );
				if ( tags.isArray() )
				{
                    const JsonArray &elements = tags.toArray();
					for (const Json &tag : elements) {
						if ( tag.isObject() )
						{
                            metaDatum->tags.append(tag.value( CATEGORY ).toString());
						}
					}
				}

				OVR_ASSERT( !metaDatum->tags.isEmpty() );

                metaDatum->url = jsonDatum.value( URL_INNER ).toString();
				extractExtendedData( jsonDatum, *metaDatum );

                StringHash< OvrMetaDatum * >::Iterator iter = outMetaData.find( metaDatum->url );
                if ( iter == outMetaData.end() )
				{
                    outMetaData.insert( metaDatum->url, metaDatum );
				}
				else
				{
                    iter->second = metaDatum;
				}
			}
		}
	}
}


void OvrMetaData::regenerateCategoryIndices()
{
	for ( int catIndex = 0; catIndex < m_categories.length(); ++catIndex )
	{
		Category & cat = m_categories.at( catIndex );
		cat.datumIndicies.clear();
	}

	// Delete any data only tagged as "Favorite" - this is a fix for user created "Favorite" folder which is a special case
	// Not doing this will show photos already favorited that the user cannot unfavorite
	for ( int metaDataIndex = 0; metaDataIndex < m_etaData.length(); ++metaDataIndex )
	{
		OvrMetaDatum & metaDatum = *m_etaData.at( metaDataIndex );
		VArray< VString > & tags = metaDatum.tags;

		OVR_ASSERT( metaDatum.tags.length() > 0 );
		if ( tags.length() == 1 )
		{
			if ( tags.at( 0 ) == FAVORITES_TAG )
			{
				LOG( "Removing broken metadatum %s", metaDatum.url.toCString() );
				m_etaData.removeAtUnordered( metaDataIndex );
			}
		}
	}

	// Fix the indices
	for ( int metaDataIndex = 0; metaDataIndex < m_etaData.length(); ++metaDataIndex )
	{
		OvrMetaDatum & datum = *m_etaData.at( metaDataIndex );
		VArray< VString > & tags = datum.tags;

		OVR_ASSERT( tags.length() > 0 );

		if ( tags.length() == 1 )
		{
			OVR_ASSERT( tags.at( 0 ) != FAVORITES_TAG );
		}


		if ( tags.at( 0 ) == FAVORITES_TAG && tags.length() > 1 )
		{
			Alg::Swap( tags.at( 0 ), tags.at( 1 ) );
		}

		for ( int tagIndex = 0; tagIndex < tags.length(); ++tagIndex )
		{
			const VString & tag = tags[ tagIndex ];
			if ( !tag.isEmpty() )
			{
				if ( Category * category = getCategory( tag ) )
				{
					LOG( "OvrMetaData inserting index %d for datum %s to %s", metaDataIndex, datum.url.toCString(), category->categoryTag.toCString() );

					// fix the metadata index itself
					datum.id = metaDataIndex;

					// Update the category with the new index
					category->datumIndicies.append( metaDataIndex );
				}
				else
				{
					WARN( "OvrMetaData::RegenerateCategoryIndices failed to find category with tag %s for datum %s at index %d",
						tag.toCString(), datum.url.toCString(), metaDataIndex );
				}
			}
		}
	}
}

Json OvrMetaData::metaDataToJson() const
{
	Json DataFile(Json::Object);

	// Add version
	DataFile[VERSION] = m_version;

	// Add categories
	Json newCategoriesObject(Json::Array);

	for ( int c = 0; c < m_categories.length(); ++c )
	{
		Json catObject(Json::Object);

		const Category & cat = m_categories.at( c );
		catObject[TAG] = std::string(cat.categoryTag.toCString());
		catObject[LABEL] = std::string(cat.label.toCString());
		LOG( "OvrMetaData::MetaDataToJson adding category %s", cat.categoryTag.toCString() );
		newCategoriesObject.append( catObject );
	}
	DataFile[CATEGORIES] = newCategoriesObject;

	// Add meta data
	Json newDataObject(Json::Array);

	for ( int i = 0; i < m_etaData.length(); ++i )
	{
		const OvrMetaDatum & metaDatum = *m_etaData.at( i );

		Json datumObject(Json::Object);
		extendedDataToJson( metaDatum, datumObject );
		datumObject.insert( URL_INNER, std::string(metaDatum.url.toCString()) );
		LOG( "OvrMetaData::MetaDataToJson adding datum url %s", metaDatum.url.toCString() );

		Json newTagsObject(Json::Array);
		for ( int t = 0; t < metaDatum.tags.length(); ++t )
		{
			Json tagObject(Json::Object);
			tagObject.insert( CATEGORY, std::string(metaDatum.tags.at( t ).toCString()) );
			newTagsObject.append( tagObject );
		}

		datumObject.insert( TAGS, newTagsObject );

		newDataObject.append( datumObject );
	}
	DataFile[DATA] = newDataObject;

	return DataFile;
}

TagAction OvrMetaData::toggleTag( OvrMetaDatum * metaDatum, const VString & newTag )
{
    Json DataFile = Json::Load( m_filePath.toCString() );
	if ( DataFile.isInvalid() )
	{
		FAIL( "OvrMetaData failed to load JSON meta file: %s", m_filePath.toCString() );
	}

	OVR_ASSERT( DataFile.isValid() );
	OVR_ASSERT( metaDatum );

	// First update the local data
	TagAction action = TAG_ERROR;
	for ( int t = 0; t < metaDatum->tags.length(); ++t )
	{
		if ( metaDatum->tags.at( t ) == newTag )
		{
			// Handle case which leaves us with no tags - ie. broken state
			if ( metaDatum->tags.length() < 2 )
			{
				WARN( "ToggleTag attempt to remove only tag: %s on %s", newTag.toCString(), metaDatum->url.toCString() );
				return TAG_ERROR;
			}
			LOG( "ToggleTag TAG_REMOVED tag: %s on %s", newTag.toCString(), metaDatum->url.toCString() );
			action = TAG_REMOVED;
			metaDatum->tags.removeAt( t );
			break;
		}
	}

	if ( action == TAG_ERROR )
	{
		LOG( "ToggleTag TAG_ADDED tag: %s on %s", newTag.toCString(), metaDatum->url.toCString() );
		metaDatum->tags.append( newTag );
		action = TAG_ADDED;
	}

	// Then serialize
	Json newTagsObject(Json::Array);

	for ( int t = 0; t < metaDatum->tags.length(); ++t )
	{
		Json tagObject(Json::Object);
		tagObject.insert(CATEGORY, std::string(metaDatum->tags.at( t ).toCString()));
		newTagsObject.append(tagObject);
	}

	const Json &data = DataFile.value( DATA );
	if (data.isArray() && (int) data.size() > metaDatum->id) {
		Json datum = data.at(metaDatum->id);
		if (datum.contains(TAGS)) {
			datum[TAGS] = newTagsObject;

            std::ofstream fp(m_filePath.toCString(), std::ios::binary);
			fp << DataFile;
		}
	}

	return action;
}

void OvrMetaData::addCategory( const VString & name )
{
	Category cat;
	cat.categoryTag = name;
	m_categories.append( cat );
}

OvrMetaData::Category * OvrMetaData::getCategory( const VString & categoryName )
{
	const int numCategories = m_categories.length();
	for ( int i = 0; i < numCategories; ++i )
	{
		Category & category = m_categories.at( i );
		if ( category.categoryTag == categoryName )
		{
			return &category;
		}
	}
	return NULL;
}

const OvrMetaDatum & OvrMetaData::getMetaDatum( const int index ) const
{
	OVR_ASSERT( index >= 0 && index < m_etaData.length() );
	return *m_etaData.at( index );
}


bool OvrMetaData::getMetaData( const Category & category, VArray< const OvrMetaDatum * > & outMetaData ) const
{
	const int numPanos = category.datumIndicies.length();
	for ( int i = 0; i < numPanos; ++i )
	{
		const int metaDataIndex = category.datumIndicies.at( i );
		OVR_ASSERT( metaDataIndex >= 0 && metaDataIndex < m_etaData.length() );
		//const OvrMetaDatum * panoData = &MetaData.At( metaDataIndex );
        //LOG( "Getting MetaData %d title %s from category %s", metaDataIndex, panoData->Title.toCString(), category.CategoryName.toCString() );
		outMetaData.append( m_etaData.at( metaDataIndex ) );
	}
	return true;
}

bool OvrMetaData::shouldAddFile( const char * filename, const OvrMetaDataFileExtensions & fileExtensions ) const
{
	const int pathLen = strlen( filename );
	for ( int index = 0; index < fileExtensions.badExtensions.length(); ++index )
	{
		const VString & ext = fileExtensions.badExtensions.at( index );
        const int extLen = (int) ext.length();
		if ( pathLen > extLen && OVR_stricmp( filename + pathLen - extLen, ext.toCString() ) == 0 )
		{
			return false;
		}
	}

	for ( int index = 0; index < fileExtensions.goodExtensions.length(); ++index )
	{
		const VString & ext = fileExtensions.goodExtensions.at( index );
        const int extLen = (int) ext.length();
		if ( pathLen > extLen && OVR_stricmp( filename + pathLen - extLen, ext.toCString() ) == 0 )
		{
			return true;
		}
	}

	return false;
}

void OvrMetaData::setCategoryDatumIndicies( const int index, const VArray< int >& datumIndicies )
{
	OVR_ASSERT( index < m_categories.length() );

	if ( index < m_categories.length() )
	{
        m_categories[index].datumIndicies = datumIndicies;
	}
}

}
