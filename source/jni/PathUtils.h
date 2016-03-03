#pragma once

#include "vglobal.h"

#include <jni.h>
#include "VString.h"
#include "Array.h"

#pragma once

NV_NAMESPACE_BEGIN
	class App;

	enum EStorageType
	{
		// By default data here is private and other apps shouldn't be able to access data from here
		// Path => "/data/data/", in Note 4 this is 24.67GB
		EST_INTERNAL_STORAGE = 0,

		// Also known as emulated internal storage, as this is part of phone memory( that can't be removed ) which is emulated as external storage
		// in Note 4 this is = 24.64GB, with WRITE_EXTERNAL_STORAGE permission can write anywhere in this storage
		// Path => "/storage/emulated/0" or "/sdcard",
		EST_PRIMARY_EXTERNAL_STORAGE,

		// Path => "/storage/extSdCard"
		// Can only write to app specific folder - /storage/extSdCard/Android/obb/<app>
		EST_SECONDARY_EXTERNAL_STORAGE,

		EST_COUNT
	};

	enum EFolderType
	{
		// Root folder, for example:
		//		internal 			=> "/data"
		//		primary external 	=> "/storage/emulated/0"
		//		secondary external 	=> "/storage/extSdCard"
		EFT_ROOT = 0,

		// Files folder
		EFT_FILES,

		// Cache folder, data in this folder can be flushed by OS when it needs more memory.
		EFT_CACHE,

		EFT_COUNT
	};

	class OvrStoragePaths
	{
	public:
					OvrStoragePaths( JNIEnv * jni, jobject activityObj );
		void		PushBackSearchPathIfValid( EStorageType toStorage, EFolderType toFolder, const char * subfolder, Array<VString> & searchPaths ) const;
		void		PushBackSearchPathIfValidPermission( EStorageType toStorage, EFolderType toFolder, const char * subfolder, mode_t permission, Array<VString> & searchPaths ) const;
		bool		GetPathIfValidPermission( EStorageType toStorage, EFolderType toFolder, const char * subfolder, mode_t permission, VString & outPath ) const;
		bool		HasStoragePath( const EStorageType toStorage, const EFolderType toFolder ) const;
		long long 	GetAvailableInternalMemoryInBytes( JNIEnv * jni, jobject activityObj ) const;

	private:
		// contains all the folder paths for future reference
		VString 			StorageFolderPaths[EST_COUNT][EFT_COUNT];
		jclass			VrLibClass;
		jmethodID		InternalCacheMemoryID;
	};

	VString	GetFullPath		( const Array< VString > & searchPaths, const VString & relativePath );

	// Return false if it fails to find the relativePath in any of the search locations
	bool	GetFullPath		(const Array< VString > & searchPaths, const VString &relativePath, 	char * outPath, 	const int outMaxLen );
	bool	GetFullPath		(const Array< VString > & searchPaths, const VString &relativePath, 	VString & outPath 						);

	bool	ToRelativePath	( const Array< VString > & searchPaths, char const * fullPath, 		char * outPath, 	const int outMaxLen );
	bool	ToRelativePath	( const Array< VString > & searchPaths, char const * fullPath, 		VString & outPath 						);

NV_NAMESPACE_END


