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

    struct Info
    {
        StorageType storageType;
        FolderType folderType;
        const char *subfolder;
    };

    VStandardPath(JNIEnv *jni, jobject activityObj);
    ~VStandardPath();

    bool contains(StorageType toStorage, FolderType toFolder) const;

    VString findFolder(StorageType toStorage, FolderType toFolder, const char *subfolder) const;
    VString findFolder(const Info &info) const { return findFolder(info.storageType, info.folderType, info.subfolder); }

private:
    NV_DECLARE_PRIVATE
    NV_DISABLE_COPY(VStandardPath)
};

VString	GetFullPath(const VArray< VString > & searchPaths, const VString &relativePath);
bool GetFullPath(const VArray<VString> &searchPaths, const VString &relativePath, VString &outPath);

NV_NAMESPACE_END


