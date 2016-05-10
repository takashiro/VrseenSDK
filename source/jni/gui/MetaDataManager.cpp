/************************************************************************************

Filename    :   MetaDataManager.cpp
Content     :   A class to manage metadata used by FolderBrowser
Created     :   January 26, 2015
Authors     :   Jonathan E. Wright, Warsam Osman, Madhu Kalva

Copyright   :   Copyright 2015 Oculus VR, LLC. All Rights reserved.


*************************************************************************************/

#include "MetaDataManager.h"
#include "VAlgorithm.h"

#include "VDir.h"
#include "VPath.h"
#include "VApkFile.h"
#include "VJson.h"
#include "VArray.h"
#include "VLog.h"
#include "VStandardPath.h"

#include <algorithm>
#include <unistd.h>
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
	vInfo("OvrMetaData::InitFromDirectory( " << relativePath << " )");
	// Find all the files - checks all search paths
//	StringHash< VString > uniqueFileList = RelativeDirectoryFileList( searchPaths, relativePath );
    VArray< VString > uniqueFileList = VDir::Search(searchPaths, relativePath);
	VArray<VString> fileList;
//    for (const std::pair<VString, VString> &iter : uniqueFileList) {
    for (auto &iter : uniqueFileList) {
        fileList.append(iter);
	}
    std::sort(fileList.begin(),fileList.end());
	Category currentCategory;
    currentCategory.categoryTag = VPath(relativePath).baseName();
	// The label is the same as the tag by default.
	//Will be replaced if definition found in loaded metadata
	currentCategory.label = currentCategory.categoryTag;

	vInfo("OvrMetaData start category: " << currentCategory.categoryTag);
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
        if (!shouldAddFile(s, fileExtensions)) {
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
                VStringHash<int>::const_iterator iter = m_urlToIndex.find(datum->url);
                if ( iter == m_urlToIndex.end() ) {
                    m_urlToIndex.insert(datum->url, dataIndex);
                    m_etaData.append(datum);
					vInfo("OvrMetaData adding datum " << datum->url << " with index " << dataIndex << " to " << currentCategory.categoryTag);
					// Register with category
					currentCategory.datumIndicies.append( dataIndex );
                } else {
					vWarn("OvrMetaData::InitFromDirectory found duplicate url " << datum->url);
				}
			}
			else
			{
				vWarn("OvrMetaData::InitFromDirectory failed to find " << s);
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
	VStringHash< int > uniqueCategoryList;
	for ( int i = 0; i < fileList.length(); ++i )
	{
		const VString & filePath = fileList.at( i );
        const VString categoryTag = VPath(fileList.at(i)).dirName();
        VStringHash< int >::ConstIterator iter = uniqueCategoryList.find( categoryTag );
		int catIndex = -1;
        if ( iter == uniqueCategoryList.end() )
		{
			vInfo("category: " << categoryTag);
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

		vAssert( catIndex > -1 );
        Category & currentCategory = m_categories[catIndex];

		// See if we want this loose-file
        if (!shouldAddFile(filePath, fileExtensions)) {
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

            VStringHash< int >::ConstIterator iter = m_urlToIndex.find( datum->url );
            if ( iter == m_urlToIndex.end() )
			{
                m_urlToIndex.insert( datum->url, dataIndex );
				m_etaData.append( datum );
                vInfo("OvrMetaData::InitFromFileList adding datum" << datum->url << "with index" << dataIndex << "to" << currentCategory.categoryTag);
				// Register with category
				currentCategory.datumIndicies.append( dataIndex );
			}
			else
			{
				vWarn("OvrMetaData::InitFromFileList found duplicate url " << datum->url);
			}
		}
	}
}

void OvrMetaData::renameCategory(const VString &currentTag, const VString &newName )
{
	for ( int i = 0; i < m_categories.length(); ++i )
	{
        Category & cat = m_categories[i];
		if ( cat.categoryTag == currentTag )
		{
			cat.label = newName;
			break;
		}
	}
}

VJson LoadPackageMetaFile( const char* metaFile )
{
    uint bufferLength = 0;
    void * 	buffer = NULL;
	VString assetsMetaFile = "assets/";
	assetsMetaFile += metaFile;
    const VApkFile &apk = VApkFile::CurrentApkFile();
    apk.read(assetsMetaFile, buffer, bufferLength);
    if ( buffer != nullptr )
	{
		vWarn("LoadPackageMetaFile failed to read " << assetsMetaFile);
	}
    return VJson::Parse( static_cast<char*>(buffer) );
}

VJson OvrMetaData::createOrGetStoredMetaFile( const VString &appFileStoragePath, const char * metaFile )
{
	m_filePath = appFileStoragePath;
	m_filePath += metaFile;

	vInfo("CreateOrGetStoredMetaFile FilePath: " << m_filePath);

    VJson dataFile = VJson::Load(m_filePath);
    if (dataFile.isNull())
	{
		// If this is the first run, or we had an error loading the file, we copy the meta file from assets to app's cache
		writeMetaFile( metaFile );

		// try loading it again
        dataFile = VJson::Load(m_filePath);
        if ( dataFile.isNull() )
		{
			vWarn("OvrMetaData failed to load JSON meta file: " << metaFile);
		}
    } else {
        vInfo("OvrMetaData::CreateOrGetStoredMetaFile found" << m_filePath);
	}
	return dataFile;
}

void OvrMetaData::writeMetaFile(const char * metaFile) const
{
    vInfo("Writing metafile from apk");

    uint bufferLength = 0;
    void *buffer = NULL;
    VString assetsMetaFile = "assets/";
    assetsMetaFile += metaFile;
    const VApkFile &apk = VApkFile::CurrentApkFile();
    apk.read(assetsMetaFile, buffer, bufferLength);
    if (!buffer) {
        vWarn("OvrMetaData failed to read" << assetsMetaFile);
    } else {
        if (FILE *newMetaFile = fopen(m_filePath.toCString(), "w")) {
            uint writtenCount = fwrite( buffer, 1, bufferLength, newMetaFile );
            if ( writtenCount != bufferLength )
            {
                vFatal( "OvrMetaData::WriteMetaFile failed to write" << metaFile);
            }
            free( buffer );
            fclose( newMetaFile );
        } else {
            vFatal( "OvrMetaData failed to create" << m_filePath << "- check app permissions");
        }
    }
}

void OvrMetaData::initFromDirectoryMergeMeta( const char * relativePath, const VArray< VString > & searchPaths,
	const OvrMetaDataFileExtensions & fileExtensions, const char * metaFile, const char * packageName )
{
    VDir vdir;
	vInfo("OvrMetaData::InitFromDirectoryMergeMeta");

	VString appFileStoragePath = "/data/data/";
	appFileStoragePath += packageName;
	appFileStoragePath += "/files/";

	m_filePath = appFileStoragePath + metaFile;

	vAssert( vdir.contains( m_filePath, R_OK ) );

    initFromDirectory( relativePath, searchPaths, fileExtensions );

    //@to-do: Remove MetaData
    /*VJson dataFile = createOrGetStoredMetaFile( appFileStoragePath, metaFile );
    processMetaData( dataFile, searchPaths, metaFile );*/
}

void OvrMetaData::initFromFileListMergeMeta( const VArray< VString > & fileList, const VArray< VString > & searchPaths,
	const OvrMetaDataFileExtensions & fileExtensions, const char * appFileStoragePath, const char * metaFile, const NervGear::VJson &storedMetaData )
{
    vInfo("OvrMetaData::InitFromFileListMergeMeta");

    initFromFileList( fileList, fileExtensions );

    vAssert(storedMetaData.isObject());
    processMetaData(storedMetaData.toObject(), searchPaths, metaFile);
}

void OvrMetaData::processRemoteMetaFile( const char * metaFileString, const int startIndex )
{
	VJson remoteMetaFile = VJson::Parse( metaFileString );
    if ( remoteMetaFile.isObject() )
	{
		// First grab the version
		double remoteVersion = 0.0;
        extractVersion(remoteMetaFile.toObject(), remoteVersion );

		if ( remoteVersion <= m_version ) // We already have this metadata, don't need to process further
			return;

		m_version = remoteVersion;

		VArray< Category > remoteCategories;
		VStringHash< OvrMetaDatum * > remoteMetaData;
        extractCategories( remoteMetaFile.toObject() , remoteCategories );
        extractRemoteMetaData( remoteMetaFile.toObject() , remoteMetaData );

		// Merge in the remote categories
		// Ignore any duplicate categories
		VStringHash< bool > CurrentCategoriesSet; // using as set
		for ( int i = 0; i < m_categories.length(); ++i )
		{
			const Category & storedCategory = m_categories.at( i );
            CurrentCategoriesSet.insert( storedCategory.categoryTag, true );
		}

		for ( int remoteIndex = 0; remoteIndex < remoteCategories.length(); ++remoteIndex )
		{
			const Category & remoteCat = remoteCategories.at( remoteIndex );

            VStringHash< bool >::ConstIterator iter = CurrentCategoriesSet.find( remoteCat.categoryTag );
            if ( iter == CurrentCategoriesSet.end() )
			{
				const int targetIndex = startIndex + remoteIndex;
				vInfo("OvrMetaData::ProcessRemoteMetaFile merging " << remoteCat.categoryTag << " into category index " << targetIndex);
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
				vInfo("OvrMetaData::ProcessRemoteMetaFile discarding duplicate category " << remoteCat.categoryTag);
			}
		}

		// Append the remote data
		reconcileMetaData( remoteMetaData );

		// Recreate indices which may have changed after reconciliation
		regenerateCategoryIndices();

		// Serialize the new metadata
		VJson dataFile = metaDataToJson();
        if (dataFile.isNull()) {
			vFatal("OvrMetaData::ProcessMetaData failed to generate JSON meta file");
		}

        std::ofstream fp(m_filePath.toUtf8(), std::ios::binary);
		fp << dataFile;

		vInfo("OvrMetaData::ProcessRemoteMetaFile updated " << m_filePath);
	}
}

void OvrMetaData::processMetaData( const VJsonObject &dataFile, const VArray< VString > & searchPaths, const char * metaFile )
{
    // Grab the version from the loaded data
    extractVersion(dataFile, m_version );

    VArray< Category > storedCategories;
    VStringHash< OvrMetaDatum * > storedMetaData;
    extractCategories(dataFile, storedCategories );

    // Read in package data first
    VJson packageMeta = LoadPackageMetaFile( metaFile );
    if ( packageMeta.isObject() )
    {
        // If we failed to find a version in the serialized data, need to set it from the assets version
        if ( m_version < 0.0 )
        {
            extractVersion(packageMeta.toObject(), m_version );
            if ( m_version < 0.0 )
            {
                m_version = 0.0;
            }
        }
        extractCategories(packageMeta.toObject(), storedCategories );
        extractMetaData(packageMeta.toObject(), searchPaths, storedMetaData );
    }
    else
    {
        vWarn("ProcessMetaData LoadPackageMetaFile failed for " << metaFile);
    }

    // Read in the stored data - overriding any found in the package
    extractMetaData(dataFile, searchPaths, storedMetaData);

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
            Category & cat = m_categories[catIndex];
            if ( !cat.datumIndicies.isEmpty() )
            {
                finalCategories.append( cat );
            }
            else
            {
                vWarn("OvrMetaData::ProcessMetaData discarding empty " << cat.categoryTag);
            }
        }
        std::swap(finalCategories, m_categories);
    }

	// Rewrite new data
	VJson newDataFile = metaDataToJson();
    if ( newDataFile.isNull() )
	{
		vFatal("OvrMetaData::ProcessMetaData failed to generate JSON meta file");
	}

    std::ofstream fp(m_filePath.toUtf8(), std::ios::binary);
	fp << newDataFile;

    vInfo("OvrMetaData::ProcessMetaData created" << m_filePath);
}

void OvrMetaData::reconcileMetaData( VStringHash< OvrMetaDatum * > & storedMetaData )
{
    if ( storedMetaData.isEmpty() )
	{
		return;
	}
    dedupMetaData( m_etaData, storedMetaData );

	// Now for any remaining stored data - check if it's remote and just add it, sorted by the
	// assigned Id
	VArray< OvrMetaDatum * > sortedEntries;
    VStringHash< OvrMetaDatum * >::Iterator storedIter = storedMetaData.begin();
    for ( ; storedIter != storedMetaData.end(); ++storedIter )
	{
        OvrMetaDatum * storedDatum = storedIter->second;
		if ( isRemote( storedDatum ) )
		{
			vInfo("ReconcileMetaData metadata adding remote " << storedDatum->url);
			sortedEntries.append( storedDatum );
		}
	}
	std::sort(sortedEntries.begin(), sortedEntries.end(), OvrMetaDatumIdComparator);
	VArray< OvrMetaDatum * >::iterator sortedIter = sortedEntries.begin();
	for ( ; sortedIter != sortedEntries.end(); ++sortedIter )
	{
		m_etaData.append( *sortedIter );
	}
    storedMetaData.clear();
}

void OvrMetaData::dedupMetaData( const VArray< OvrMetaDatum * > & existingData, VStringHash< OvrMetaDatum * > & newData )
{
    // Fix the read in meta data using the stored
    for ( int i = 0; i < existingData.length(); ++i )
    {
        OvrMetaDatum * metaDatum = existingData.at( i );

        VStringHash< OvrMetaDatum * >::Iterator iter = newData.find( metaDatum->url );

        if ( iter != newData.end() )
        {
            OvrMetaDatum * storedDatum = iter->second;
            vInfo("DedupMetaData metadata for " << storedDatum->url);
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
		vWarn("OvrMetaData::ReconcileCategories failed to find expected category order -- missing assets/meta.json?");
	}

	finalCategories.append( favorites );

	VStringHash< bool > StoredCategoryMap; // using as set
	for ( int i = 0; i < storedCategories.length(); ++i )
	{
		const Category & storedCategory = storedCategories.at( i );
		vInfo("OvrMetaData::ReconcileCategories storedCategory: " << storedCategory.categoryTag);
        StoredCategoryMap.insert( storedCategory.categoryTag, true );
	}

	// Now add the read in categories if they differ
	for ( int i = 0; i < m_categories.length(); ++i )
	{
		const Category & readInCategory = m_categories.at( i );
        VStringHash< bool >::ConstIterator iter = StoredCategoryMap.find( readInCategory.categoryTag );

        if ( iter == StoredCategoryMap.end() )
		{
			vInfo("OvrMetaData::ReconcileCategories adding " << readInCategory.categoryTag);
			finalCategories.append( readInCategory );
		}
	}

	// Finally fill in the stored in categories after user made ones
	for ( int i = 1; i < storedCategories.length(); ++i )
	{
		const  Category & storedCat = storedCategories.at( i );
		vInfo("OvrMetaData::ReconcileCategories adding stored category " << storedCat.categoryTag);
		finalCategories.append( storedCat );
	}

	// Now replace Categories
    std::swap(m_categories, finalCategories);
}

void OvrMetaData::extractVersion(const VJsonObject &dataFile, double &outVersion) const
{
    outVersion = dataFile.value(VERSION).toDouble();
}

void OvrMetaData::extractCategories(const VJsonObject &dataFile, VArray< Category > & outCategories ) const
{
	const VJson &categories( dataFile.value( CATEGORIES ) );
	if ( categories.isArray() )
	{
        const VJsonArray &elements = categories.toArray();
		for (const VJson &category : elements) {
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
					vInfo("Extracting category: " << extractedCategory.categoryTag);
					outCategories.append( extractedCategory );
				}
			}
		}
	}
}

void OvrMetaData::extractMetaData(const VJsonObject &dataFile, const VArray< VString > & searchPaths, VStringHash< OvrMetaDatum * > & outMetaData ) const
{
	const VJson &data( dataFile.value( DATA ) );
	if ( data.isArray() )
	{
		int jsonIndex = m_etaData.length();

        const VJsonArray &datums = data.toArray();
		for (const VJson &datum : datums) {
			if ( datum.isObject() )
			{
				OvrMetaDatum * metaDatum = createMetaDatum( "" );
				if ( !metaDatum )
				{
					continue;
				}

				metaDatum->id = jsonIndex++;
				const VJson &tags( datum.value( TAGS ) );
				if ( tags.isArray() )
				{
                    const VJsonArray &elements = tags.toArray();
					for (const VJson &tag : elements) {
						if ( tag.isObject() )
						{
                            metaDatum->tags.append(tag.value( CATEGORY ).toString());
						}
					}
				}

				vAssert( !metaDatum->tags.isEmpty() );

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
				vInfo("OvrMetaData::ExtractMetaData adding datum " << metaDatum->url);

                VStringHash< OvrMetaDatum * >::Iterator iter = outMetaData.find( metaDatum->url );
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

void OvrMetaData::extractRemoteMetaData( const VJson &dataFile, VStringHash< OvrMetaDatum * > & outMetaData ) const
{
    if ( dataFile.isNull() )
	{
		return;
	}

	const VJson &data( dataFile.value( DATA ) );
	if ( data.isArray() )
	{
		int jsonIndex = m_etaData.length();

        const VJsonArray elements = data.toArray();
		for (const VJson &jsonDatum : elements) {
			if ( jsonDatum.isObject() )
			{
				OvrMetaDatum * metaDatum = createMetaDatum( "" );
				if ( !metaDatum )
				{
					continue;
				}
				metaDatum->id = jsonIndex++;
				const VJson &tags( jsonDatum.value( TAGS ) );
				if ( tags.isArray() )
				{
                    const VJsonArray &elements = tags.toArray();
					for (const VJson &tag : elements) {
						if ( tag.isObject() )
						{
                            metaDatum->tags.append(tag.value( CATEGORY ).toString());
						}
					}
				}

				vAssert( !metaDatum->tags.isEmpty() );

                metaDatum->url = jsonDatum.value( URL_INNER ).toString();
				extractExtendedData( jsonDatum, *metaDatum );

                VStringHash< OvrMetaDatum * >::Iterator iter = outMetaData.find( metaDatum->url );
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
        Category & cat = m_categories[catIndex];
		cat.datumIndicies.clear();
	}

	// Delete any data only tagged as "Favorite" - this is a fix for user created "Favorite" folder which is a special case
	// Not doing this will show photos already favorited that the user cannot unfavorite
	for ( int metaDataIndex = 0; metaDataIndex < m_etaData.length(); ++metaDataIndex )
	{
		OvrMetaDatum & metaDatum = *m_etaData.at( metaDataIndex );
		VArray< VString > & tags = metaDatum.tags;

		vAssert( metaDatum.tags.length() > 0 );
		if ( tags.length() == 1 )
		{
			if ( tags.at( 0 ) == FAVORITES_TAG )
			{
				vInfo("Removing broken metadatum " << metaDatum.url);
				m_etaData.removeAtUnordered( metaDataIndex );
			}
		}
	}

	// Fix the indices
	for ( int metaDataIndex = 0; metaDataIndex < m_etaData.length(); ++metaDataIndex )
	{
		OvrMetaDatum & datum = *m_etaData.at( metaDataIndex );
		VArray< VString > & tags = datum.tags;

		vAssert( tags.length() > 0 );

		if ( tags.length() == 1 )
		{
			vAssert( tags.at( 0 ) != FAVORITES_TAG );
		}


		if ( tags.at( 0 ) == FAVORITES_TAG && tags.length() > 1 )
		{
           std::swap(tags[0], tags[1]);
		}

		for ( int tagIndex = 0; tagIndex < tags.length(); ++tagIndex )
		{
			const VString & tag = tags[ tagIndex ];
			if ( !tag.isEmpty() )
			{
				if ( Category * category = getCategory( tag ) )
				{
					vInfo("OvrMetaData inserting index " << metaDataIndex << " for datum " << datum.url << " to " << category->categoryTag);

					// fix the metadata index itself
					datum.id = metaDataIndex;

					// Update the category with the new index
					category->datumIndicies.append( metaDataIndex );
				}
				else
				{
                    vWarn("OvrMetaData::RegenerateCategoryIndices failed to find category with tag"
                          << tag << "for datum" << datum.url << "at index" << metaDataIndex);
				}
			}
		}
	}
}

VJson OvrMetaData::metaDataToJson() const
{
    VJsonObject DataFile;

	// Add version
    DataFile.insert(VERSION, m_version);

	// Add categories
    VJsonArray newCategoriesObject;

	for ( int c = 0; c < m_categories.length(); ++c )
	{
        VJsonObject catObject;

		const Category & cat = m_categories.at( c );
        catObject.insert(TAG, cat.categoryTag);
        catObject.insert(LABEL, cat.label);
		vInfo("OvrMetaData::MetaDataToJson adding category " << cat.categoryTag);
        newCategoriesObject.append(std::move(catObject));
	}
    DataFile.insert(CATEGORIES, std::move(newCategoriesObject));

	// Add meta data
    VJsonArray newDataObject;

	for ( int i = 0; i < m_etaData.length(); ++i )
	{
		const OvrMetaDatum & metaDatum = *m_etaData.at( i );

        VJsonObject datumObject;
        extendedDataToJson(metaDatum, datumObject);
        datumObject.insert(URL_INNER, metaDatum.url);
		vInfo("OvrMetaData::MetaDataToJson adding datum url " << metaDatum.url);

        VJsonArray newTagsObject;
        for (const VString &tag : metaDatum.tags) {
            VJsonObject tagObject;
            tagObject.insert(CATEGORY, tag);
            newTagsObject.append(std::move(tagObject));
		}

        datumObject.insert(TAGS, std::move(newTagsObject));

        newDataObject.append(std::move(datumObject));
	}
    DataFile.insert(DATA, std::move(newDataObject));

	return DataFile;
}

TagAction OvrMetaData::toggleTag( OvrMetaDatum * metaDatum, const VString & newTag )
{
    VJson DataFile = VJson::Load(m_filePath);
    if ( DataFile.isNull() )
	{
		vFatal("OvrMetaData failed to load JSON meta file: " << m_filePath);
	}

	vAssert( metaDatum );

	// First update the local data
	TagAction action = TAG_ERROR;
	for ( int t = 0; t < metaDatum->tags.length(); ++t )
	{
		if ( metaDatum->tags.at( t ) == newTag )
		{
			// Handle case which leaves us with no tags - ie. broken state
			if ( metaDatum->tags.length() < 2 )
			{
				vWarn("ToggleTag attempt to remove only tag: " << newTag << " on " << metaDatum->url);
				return TAG_ERROR;
			}
			vInfo("ToggleTag TAG_REMOVED tag: " << newTag << " on " << metaDatum->url);
			action = TAG_REMOVED;
			metaDatum->tags.removeAt( t );
			break;
		}
	}

	if ( action == TAG_ERROR )
	{
		vInfo("ToggleTag TAG_ADDED tag: " << newTag << " on " << metaDatum->url);
		metaDatum->tags.append( newTag );
		action = TAG_ADDED;
	}

	// Then serialize
    VJsonArray newTagsObject;

	for ( int t = 0; t < metaDatum->tags.length(); ++t )
	{
        VJsonObject tagObject;
        tagObject.insert(CATEGORY, metaDatum->tags.at(t));
        newTagsObject.append(std::move(tagObject));
	}

    const VJson &data = DataFile.value(DATA);
	if (data.isArray() && (int) data.size() > metaDatum->id) {
		VJson datum = data.at(metaDatum->id);
        if (datum.isObject() && datum.object().contains(TAGS)) {
            datum.object().insert(TAGS, std::move(newTagsObject));

            std::ofstream fp(m_filePath.toUtf8(), std::ios::binary);
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
        Category & category = m_categories[i];
		if ( category.categoryTag == categoryName )
		{
			return &category;
		}
	}
	return NULL;
}

const OvrMetaDatum & OvrMetaData::getMetaDatum( const int index ) const
{
	vAssert( index >= 0 && index < m_etaData.length() );
	return *m_etaData.at( index );
}


bool OvrMetaData::getMetaData( const Category & category, VArray< const OvrMetaDatum * > & outMetaData ) const
{
	const int numPanos = category.datumIndicies.length();
	for ( int i = 0; i < numPanos; ++i )
	{
		const int metaDataIndex = category.datumIndicies.at( i );
		vAssert( metaDataIndex >= 0 && metaDataIndex < m_etaData.length() );
		//const OvrMetaDatum * panoData = &MetaData.At( metaDataIndex );
        //vInfo("Getting MetaData " << metaDataIndex << " title " << panoData->Title << " from category " << category.CategoryName);
		outMetaData.append( m_etaData.at( metaDataIndex ) );
	}
	return true;
}

bool OvrMetaData::shouldAddFile(const VString &fileName, const OvrMetaDataFileExtensions &fileExtensions) const
{
    for (const VString &ext : fileExtensions.badExtensions) {
        if (fileName.endsWith(ext)) {
			return false;
		}
	}

    for (const VString &ext : fileExtensions.goodExtensions) {
        if (fileName.endsWith(ext)) {
			return true;
		}
	}

	return false;
}

void OvrMetaData::setCategoryDatumIndicies( const int index, const VArray< int >& datumIndicies )
{
	vAssert( index < m_categories.length() );

	if ( index < m_categories.length() )
	{
        m_categories[index].datumIndicies = datumIndicies;
	}
}

}
