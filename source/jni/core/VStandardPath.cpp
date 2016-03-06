#include "VStandardPath.h"

#include <stdio.h>
#include "unistd.h"
#include "Android/JniUtils.h"
#include "VrCommon.h"
#include "App.h"

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
	{
        d->vrLibClass = ovr_GetGlobalClassReference( jni, "me/takashiro/nervgear/VrLib" );

		// Internal memory
        jmethodID internalRootDirID = ovr_GetStaticMethodID( jni, d->vrLibClass, "getInternalStorageRootDir", "()Ljava/lang/String;" );
		if( internalRootDirID )
		{
            JavaUTFChars returnString( jni, (jstring)jni->CallStaticObjectMethod( d->vrLibClass, internalRootDirID ) );
            d->storageFolderPaths[InternalStorage][RootFolder] = returnString.ToStr();
		}

        jmethodID internalFilesDirID = ovr_GetStaticMethodID( jni, d->vrLibClass, "getInternalStorageFilesDir", "(Landroid/app/Activity;)Ljava/lang/String;" );
		if( internalFilesDirID )
		{
            JavaUTFChars returnString( jni, (jstring)jni->CallStaticObjectMethod( d->vrLibClass, internalFilesDirID, activityObj ) );
            d->storageFolderPaths[InternalStorage][FilesFolder] = returnString.ToStr();
		}

        jmethodID internalCacheDirID = ovr_GetStaticMethodID( jni, d->vrLibClass, "getInternalStorageCacheDir", "(Landroid/app/Activity;)Ljava/lang/String;" );
		if( internalCacheDirID )
		{
            JavaUTFChars returnString( jni, (jstring)jni->CallStaticObjectMethod( d->vrLibClass, internalCacheDirID, activityObj ) );
            d->storageFolderPaths[InternalStorage][CacheFolder] = returnString.ToStr();
		}

        d->internalCacheMemoryId = ovr_GetStaticMethodID( jni, d->vrLibClass, "getInternalCacheMemoryInBytes", "(Landroid/app/Activity;)J" );

		// Primary external memory
        jmethodID primaryRootDirID = ovr_GetStaticMethodID( jni, d->vrLibClass, "getPrimaryExternalStorageRootDir", "(Landroid/app/Activity;)Ljava/lang/String;" );
		if( primaryRootDirID )
		{
            JavaUTFChars returnString( jni, (jstring)jni->CallStaticObjectMethod( d->vrLibClass, primaryRootDirID, activityObj ) );
            d->storageFolderPaths[PrimaryExternalStorage][RootFolder] = returnString.ToStr();
		}

        jmethodID primaryFilesDirID = ovr_GetStaticMethodID( jni, d->vrLibClass, "getPrimaryExternalStorageFilesDir", "(Landroid/app/Activity;)Ljava/lang/String;" );
		if( primaryFilesDirID )
		{
            JavaUTFChars returnString( jni, (jstring)jni->CallStaticObjectMethod( d->vrLibClass, primaryFilesDirID, activityObj ) );
            d->storageFolderPaths[PrimaryExternalStorage][FilesFolder] = returnString.ToStr();
		}

        jmethodID primaryCacheDirID = ovr_GetStaticMethodID( jni, d->vrLibClass, "getPrimaryExternalStorageCacheDir", "(Landroid/app/Activity;)Ljava/lang/String;" );
		if( primaryCacheDirID )
		{
            JavaUTFChars returnString( jni, (jstring)jni->CallStaticObjectMethod( d->vrLibClass, primaryCacheDirID, activityObj ) );
            d->storageFolderPaths[PrimaryExternalStorage][CacheFolder] = returnString.ToStr();
		}

		// secondary external memory
        jmethodID secondaryRootDirID = ovr_GetStaticMethodID( jni, d->vrLibClass, "getSecondaryExternalStorageRootDir", "()Ljava/lang/String;" );
		if( secondaryRootDirID )
		{
            JavaUTFChars returnString( jni, (jstring)jni->CallStaticObjectMethod( d->vrLibClass, secondaryRootDirID ) );
            d->storageFolderPaths[SecondaryExternalStorage][RootFolder] = returnString.ToStr();
		}

        jmethodID secondaryFilesDirID = ovr_GetStaticMethodID( jni, d->vrLibClass, "getSecondaryExternalStorageFilesDir", "(Landroid/app/Activity;)Ljava/lang/String;" );
		if( secondaryFilesDirID )
		{
            JavaUTFChars returnString( jni, (jstring)jni->CallStaticObjectMethod( d->vrLibClass, secondaryFilesDirID, activityObj ) );
            d->storageFolderPaths[SecondaryExternalStorage][FilesFolder] = returnString.ToStr();
		}

        jmethodID secondaryCacheDirID = ovr_GetStaticMethodID( jni, d->vrLibClass, "getSecondaryExternalStorageCacheDir", "(Landroid/app/Activity;)Ljava/lang/String;" );
		if( secondaryCacheDirID )
		{
            JavaUTFChars returnString( jni, (jstring)jni->CallStaticObjectMethod( d->vrLibClass, secondaryCacheDirID, activityObj ) );
            d->storageFolderPaths[SecondaryExternalStorage][CacheFolder] = returnString.ToStr();
		}
	}

    void VStandardPath::PushBackSearchPathIfValid( StorageType toStorage, FolderType toFolder, const char * subfolder, Array<VString> & searchPaths ) const
	{
		PushBackSearchPathIfValidPermission( toStorage, toFolder, subfolder, R_OK, searchPaths );
	}

	void VStandardPath::PushBackSearchPathIfValidPermission( StorageType toStorage, FolderType toFolder, const char * subfolder, mode_t permission, Array<VString> & searchPaths ) const
	{
		VString checkPath;
		if ( GetPathIfValidPermission( toStorage, toFolder, subfolder, permission, checkPath ) )
		{
			searchPaths.append( checkPath );
		}
	}

	bool VStandardPath::GetPathIfValidPermission( StorageType toStorage, FolderType toFolder, const char * subfolder, mode_t permission, VString & outPath ) const
	{
        if ( d->storageFolderPaths[ toStorage ][ toFolder ].size() > 0 )
		{
            VString checkPath = d->storageFolderPaths[ toStorage ][ toFolder ] + subfolder;
			if ( HasPermission( checkPath, permission ) )
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

	bool VStandardPath::HasStoragePath( const StorageType toStorage, const FolderType toFolder ) const
	{
        return ( d->storageFolderPaths[ toStorage ][ toFolder ].size() > 0 );
	}

	long long VStandardPath::GetAvailableInternalMemoryInBytes( JNIEnv * jni, jobject activityObj ) const
	{
        return (long long )( jni->CallStaticLongMethod( d->vrLibClass, d->internalCacheMemoryId, activityObj ) );
	}

	VString GetFullPath( const Array<VString>& searchPaths, const VString & relativePath )
	{
		if ( FileExists( relativePath ) )
		{
			return relativePath;
		}

		const int numSearchPaths = searchPaths.sizeInt();
		for ( int index = 0; index < numSearchPaths; ++index )
		{
			const VString fullPath = searchPaths.at( index ) + VString( relativePath );
			if ( FileExists( fullPath ) )
			{
				return fullPath;
			}
		}

		return VString();
	}

    bool GetFullPath( const Array<VString>& searchPaths, const VString &relativePath, char * outPath, const int outMaxLen )
	{
		OVR_ASSERT( outPath != NULL && outMaxLen >= 1 );

		if ( FileExists( relativePath ) )
		{
            OVR_sprintf( outPath, relativePath.size() + 1, "%s", relativePath.toCString() );
			return true;
		}

		for ( int i = 0; i < searchPaths.sizeInt(); ++i )
		{
            OVR_sprintf( outPath, outMaxLen, "%s%s", searchPaths[i].toCString(), relativePath.toCString() );
			if ( FileExists( outPath ) )
			{
				return true;	// outpath is now set to the full path
			}
		}
		// just return the relative path if we never found the file
        OVR_sprintf( outPath, outMaxLen, "%s", relativePath.toCString() );
		return false;
	}

    bool GetFullPath( const Array<VString>& searchPaths, const VString &relativePath, VString & outPath )
	{
		char largePath[1024];
		bool result = GetFullPath( searchPaths, relativePath, largePath, sizeof( largePath ) );
		if( result )
		{
			outPath = largePath;
		}
		return result;
	}

	bool ToRelativePath( const Array<VString>& searchPaths, char const * fullPath, char * outPath, const int outMaxLen )
	{
		// check if the path starts with any of the search paths
		const int n = searchPaths.sizeInt();
		for ( int i = 0; i < n; ++i )
		{
            char const * path = searchPaths[i].toCString();
			if ( strstr( fullPath, path ) == fullPath )
			{
				size_t len = strlen( path );
				OVR_sprintf( outPath, outMaxLen, "%s", fullPath + len );
				return true;
			}
		}
		OVR_sprintf( outPath, outMaxLen, "%s", fullPath );
		return false;
	}

	bool ToRelativePath( const Array<VString>& searchPaths, char const * fullPath, VString & outPath )
	{
		char largePath[1024];
		bool result = ToRelativePath( searchPaths, fullPath, largePath, sizeof( largePath ) );
		outPath = largePath;
		return result;
	}

NV_NAMESPACE_END
