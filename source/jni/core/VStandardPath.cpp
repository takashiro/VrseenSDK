#include "VStandardPath.h"

#include <stdio.h>
#include "unistd.h"
#include "Android/JniUtils.h"
#include "App.h"
#include "VDir.h"

NV_NAMESPACE_BEGIN

struct VStandardPath::Private
{
    VString storageFolderPaths[VStandardPath::StorageTypeCount][VStandardPath::FolderTypeCount];
    jclass vrLibClass;
    jmethodID internalCacheMemoryId;
};

const char* StorageName[VStandardPath::StorageTypeCount] =
{
    "Phone Internal", 	// "/data/data/"
    "Phone External",  	// "/storage/emulated/0" or "/sdcard"
    "SD Card External" 	// "/storage/extSdCard"
};

const char* FolderName[VStandardPath::FolderTypeCount] =
{
    "Root",
    "Files",
    "Cache"
};

	VStandardPath::VStandardPath( JNIEnv * jni, jobject activityObj )
        : d(new Private)
    {
        d->vrLibClass = JniUtils::GetGlobalClassReference( jni, "com/vrseen/nervgear/VrLib" );

		// Internal memory
        jmethodID internalRootDirID = JniUtils::GetStaticMethodID( jni, d->vrLibClass, "getInternalStorageRootDir", "()Ljava/lang/String;" );
		if( internalRootDirID )
		{
            VString returnString = JniUtils::Convert(jni, (jstring) jni->CallStaticObjectMethod(d->vrLibClass, internalRootDirID));
            d->storageFolderPaths[InternalStorage][RootFolder] = returnString;
		}

        jmethodID internalFilesDirID = JniUtils::GetStaticMethodID( jni, d->vrLibClass, "getInternalStorageFilesDir", "(Landroid/app/Activity;)Ljava/lang/String;" );
		if( internalFilesDirID )
		{
            VString returnString = JniUtils::Convert(jni, (jstring) jni->CallStaticObjectMethod(d->vrLibClass, internalFilesDirID, activityObj));
            d->storageFolderPaths[InternalStorage][FilesFolder] = returnString;
		}

        jmethodID internalCacheDirID = JniUtils::GetStaticMethodID( jni, d->vrLibClass, "getInternalStorageCacheDir", "(Landroid/app/Activity;)Ljava/lang/String;" );
		if( internalCacheDirID )
		{
            VString returnString = JniUtils::Convert(jni, (jstring) jni->CallStaticObjectMethod(d->vrLibClass, internalCacheDirID, activityObj));
            d->storageFolderPaths[InternalStorage][CacheFolder] = returnString;
		}

        d->internalCacheMemoryId = JniUtils::GetStaticMethodID( jni, d->vrLibClass, "getInternalCacheMemoryInBytes", "(Landroid/app/Activity;)J" );

		// Primary external memory
        jmethodID primaryRootDirID = JniUtils::GetStaticMethodID( jni, d->vrLibClass, "getPrimaryExternalStorageRootDir", "(Landroid/app/Activity;)Ljava/lang/String;" );
		if( primaryRootDirID )
		{
            VString returnString = JniUtils::Convert(jni, (jstring) jni->CallStaticObjectMethod(d->vrLibClass, primaryRootDirID, activityObj));
            d->storageFolderPaths[PrimaryExternalStorage][RootFolder] = returnString;
		}

        jmethodID primaryFilesDirID = JniUtils::GetStaticMethodID( jni, d->vrLibClass, "getPrimaryExternalStorageFilesDir", "(Landroid/app/Activity;)Ljava/lang/String;" );
		if( primaryFilesDirID )
		{
            VString returnString = JniUtils::Convert(jni, (jstring) jni->CallStaticObjectMethod(d->vrLibClass, primaryFilesDirID, activityObj));
            d->storageFolderPaths[PrimaryExternalStorage][FilesFolder] = returnString;
		}

        jmethodID primaryCacheDirID = JniUtils::GetStaticMethodID( jni, d->vrLibClass, "getPrimaryExternalStorageCacheDir", "(Landroid/app/Activity;)Ljava/lang/String;" );
		if( primaryCacheDirID )
		{
            VString returnString = JniUtils::Convert(jni, (jstring) jni->CallStaticObjectMethod(d->vrLibClass, primaryCacheDirID, activityObj));
            d->storageFolderPaths[PrimaryExternalStorage][CacheFolder] = returnString;
		}

		// secondary external memory
        jmethodID secondaryRootDirID = JniUtils::GetStaticMethodID( jni, d->vrLibClass, "getSecondaryExternalStorageRootDir", "()Ljava/lang/String;" );
		if( secondaryRootDirID )
		{
            VString returnString = JniUtils::Convert(jni, (jstring) jni->CallStaticObjectMethod(d->vrLibClass, secondaryRootDirID));
            d->storageFolderPaths[SecondaryExternalStorage][RootFolder] = returnString;
		}

        jmethodID secondaryFilesDirID = JniUtils::GetStaticMethodID( jni, d->vrLibClass, "getSecondaryExternalStorageFilesDir", "(Landroid/app/Activity;)Ljava/lang/String;" );
		if( secondaryFilesDirID )
		{
            VString returnString = JniUtils::Convert(jni, (jstring) jni->CallStaticObjectMethod(d->vrLibClass, secondaryFilesDirID, activityObj));
            d->storageFolderPaths[SecondaryExternalStorage][FilesFolder] = returnString;
		}

        jmethodID secondaryCacheDirID = JniUtils::GetStaticMethodID( jni, d->vrLibClass, "getSecondaryExternalStorageCacheDir", "(Landroid/app/Activity;)Ljava/lang/String;" );
		if( secondaryCacheDirID )
		{
            VString returnString = JniUtils::Convert(jni, (jstring) jni->CallStaticObjectMethod(d->vrLibClass, secondaryCacheDirID, activityObj));
            d->storageFolderPaths[SecondaryExternalStorage][CacheFolder] = returnString;
        }
    }

    VStandardPath::~VStandardPath()
    {
        delete d;
    }

    void VStandardPath::PushBackSearchPathIfValid(StorageType toStorage, FolderType toFolder, const char * subfolder, VArray<VString> &searchPaths ) const
	{
		PushBackSearchPathIfValidPermission( toStorage, toFolder, subfolder, R_OK, searchPaths );
	}

    void VStandardPath::PushBackSearchPathIfValidPermission(StorageType toStorage, FolderType toFolder, const char * subfolder, mode_t permission, VArray<VString> &searchPaths) const
	{
		VString checkPath;
		if ( GetPathIfValidPermission( toStorage, toFolder, subfolder, permission, checkPath ) )
		{
			searchPaths.append( checkPath );
		}
	}

    bool VStandardPath::GetPathIfValidPermission(StorageType toStorage, FolderType toFolder, const char * subfolder, mode_t permission, VString &outPath) const
	{
        VDir vdir;
        if ( d->storageFolderPaths[ toStorage ][ toFolder ].size() > 0 )
		{
            VPath checkPath = d->storageFolderPaths[ toStorage ][ toFolder ] + subfolder;
			if ( vdir.contains( checkPath, permission ) )
			{
				outPath = checkPath;
				return true;
			}
			else
			{
				WARN( "Failed to get permission for %s storage in %s folder ", StorageName[toStorage], FolderName[toFolder] );
			}
		}
		else
		{
			WARN( "Path not found for %s storage in %s folder ", StorageName[toStorage], FolderName[toFolder] );
		}
		return false;
	}

    bool VStandardPath::HasStoragePath(const StorageType toStorage, const FolderType toFolder) const
	{
        return ( d->storageFolderPaths[ toStorage ][ toFolder ].size() > 0 );
	}

    long long VStandardPath::GetAvailableInternalMemoryInBytes(JNIEnv * jni, jobject activityObj) const
	{
        return (long long )( jni->CallStaticLongMethod( d->vrLibClass, d->internalCacheMemoryId, activityObj ) );
	}

    VString GetFullPath(const VArray<VString> &searchPaths, const VString &relativePath)
	{
        VDir temp;
        if ( temp.exists ( relativePath ) )
        {
			return relativePath;
		}

        const int numSearchPaths = searchPaths.length();
        for ( int index = 0; index < numSearchPaths; ++index) {
            const VString fullPath = searchPaths.at(index) + relativePath;
            if ( temp.exists( fullPath ) )
			{
				return fullPath;
			}
		}

		return VString();
	}

    bool GetFullPath( const VArray<VString> &searchPaths, const VString &relativePath, char *outPath, const int outMaxLen)
	{
		OVR_ASSERT( outPath != NULL && outMaxLen >= 1 );
		VDir temp;
		if ( temp.exists (relativePath ) )
		{
            sprintf(outPath, "%s", relativePath.toCString());
			return true;
		}

        for ( int i = 0; i < searchPaths.length(); ++i )
		{
            sprintf(outPath, "%s%s", searchPaths[i].toCString(), relativePath.toCString());
            if ( temp.exists (outPath ) )
			{
				return true;	// outpath is now set to the full path
			}
		}
		// just return the relative path if we never found the file
        sprintf(outPath, "%s", relativePath.toCString());
		return false;
	}

    bool GetFullPath(const VArray<VString> &searchPaths, const VString &relativePath, VString &outPath)
	{
		char largePath[1024];
		bool result = GetFullPath( searchPaths, relativePath, largePath, sizeof( largePath ) );
		if( result )
		{
			outPath = largePath;
		}
		return result;
	}

    bool ToRelativePath( const VArray<VString>& searchPaths, char const *fullPath, char *outPath, const int outMaxLen)
	{
		// check if the path starts with any of the search paths
        const int n = searchPaths.length();
		for ( int i = 0; i < n; ++i )
		{
            char const * path = searchPaths[i].toCString();
			if ( strstr( fullPath, path ) == fullPath )
			{
				size_t len = strlen( path );
                sprintf(outPath, "%s", fullPath + len);
				return true;
			}
		}
        sprintf(outPath, "%s", fullPath);
		return false;
	}

    bool ToRelativePath(const VArray<VString>& searchPaths, char const *fullPath, VString &outPath )
	{
		char largePath[1024];
		bool result = ToRelativePath( searchPaths, fullPath, largePath, sizeof( largePath ) );
		outPath = largePath;
		return result;
	}

NV_NAMESPACE_END
