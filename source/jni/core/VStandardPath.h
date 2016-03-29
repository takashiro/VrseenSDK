#pragma once

#include "../vglobal.h"
#include "VArray.h"
#include "VString.h"

#include <jni.h>

NV_NAMESPACE_BEGIN

class VStandardPath
{
public:

    enum StorageType
    {
        // By default data here is private and other apps shouldn't be able to access data from here
        // Path => "/data/data/", in Note 4 this is 24.67GB
        InternalStorage = 0,

        // Also known as emulated internal storage, as this is part of phone memory( that can't be removed ) which is emulated as external storage
        // in Note 4 this is = 24.64GB, with WRITE_EXTERNAL_STORAGE permission can write anywhere in this storage
        // Path => "/storage/emulated/0" or "/sdcard",
        PrimaryExternalStorage,

        // Path => "/storage/extSdCard"
        // Can only write to app specific folder - /storage/extSdCard/Android/obb/<app>
        SecondaryExternalStorage,

        StorageTypeCount
    };

    enum FolderType
    {
        // Root folder, for example:
        //		internal 			=> "/data"
        //		primary external 	=> "/storage/emulated/0"
        //		secondary external 	=> "/storage/extSdCard"
        RootFolder = 0,

        // Files folder
        FilesFolder,

        // Cache folder, data in this folder can be flushed by OS when it needs more memory.
        CacheFolder,

        FolderTypeCount
    };

    VStandardPath( JNIEnv * jni, jobject activityObj );
    ~VStandardPath();

    void PushBackSearchPathIfValid(StorageType toStorage, FolderType toFolder, const char * subfolder, VArray<VString> & searchPaths ) const;
    void PushBackSearchPathIfValidPermission(StorageType toStorage, FolderType toFolder, const char * subfolder, mode_t permission, VArray<VString> &searchPaths) const;
    bool GetPathIfValidPermission(StorageType toStorage, FolderType toFolder, const char * subfolder, mode_t permission, VString & outPath) const;
    bool HasStoragePath(const StorageType toStorage, const FolderType toFolder) const;
    long long GetAvailableInternalMemoryInBytes(JNIEnv * jni, jobject activityObj) const;

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VStandardPath)
};

VString	GetFullPath(const VArray< VString > & searchPaths, const VString &relativePath);

// Return false if it fails to find the relativePath in any of the search locations
bool GetFullPath(const VArray<VString> &searchPaths, const VString &relativePath, char *outPath, const int outMaxLen);
bool GetFullPath(const VArray<VString> &searchPaths, const VString &relativePath, VString & outPath);

bool ToRelativePath(const VArray<VString> &searchPaths, char const *fullPath, char * outPath, const int outMaxLen);
bool ToRelativePath(const VArray<VString> &searchPaths, char const *fullPath, VString & outPath);

NV_NAMESPACE_END


