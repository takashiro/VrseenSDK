#include "VStandardPath.h"
#include "VDir.h"
#include "VLog.h"
#include "VFile.h"

#include "android/JniUtils.h"

#include <stdio.h>
#include <unistd.h>

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
    d->vrLibClass = JniUtils::GetGlobalClassReference( jni, "com/vrseen/VrLib" );

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

VString VStandardPath::findFolder(StorageType toStorage, FolderType toFolder, const char * subfolder) const
{
    if (d->storageFolderPaths[toStorage][toFolder].size() > 0) {
        return d->storageFolderPaths[toStorage][toFolder] + subfolder;
    } else {
        vWarn("Path not found for" << StorageName[toStorage] << "storage in" << FolderName[toFolder] << "folder");
        return VString();
    }
}

bool VStandardPath::contains(StorageType toStorage, FolderType toFolder) const
{
    return !d->storageFolderPaths[toStorage][toFolder].isEmpty();
}

VString GetFullPath(const VArray<VString> &searchPaths, const VString &relativePath)
{
    if (VFile::Exists(relativePath)) {
        return relativePath;
    }

    const int numSearchPaths = searchPaths.length();
    for ( int index = 0; index < numSearchPaths; ++index) {
        const VString fullPath = searchPaths.at(index) + relativePath;
        if (VFile::Exists(fullPath)) {
            return fullPath;
        }
    }

    return VString();
}

bool GetFullPath(const VArray<VString> &searchPaths, const VString &relativePath, VString &outPath)
{
    if (VFile::Exists(relativePath)) {
        outPath = relativePath;
        return true;
    }

    for (const VString &searchPath : searchPaths) {
        outPath = searchPath + relativePath;
        if (VFile::Exists(outPath)) {
            return true;	// outpath is now set to the full path
        }
    }
    // just return the relative path if we never found the file
    outPath = relativePath;
    return false;
}

NV_NAMESPACE_END
