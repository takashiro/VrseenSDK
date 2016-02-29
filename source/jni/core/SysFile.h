/************************************************************************************

PublicHeader:   Kernel
Filename    :   OVR_SysFile.h
Content     :   Header for all internal file management - functions and structures
                to be inherited by OS specific subclasses.
Created     :   September 19, 2012
Notes       :

Notes       :   errno may not be preserved across use of GBaseFile member functions
            :   Directories cannot be deleted while files opened from them are in use
                (For the GetFullName function)

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

************************************************************************************/

#ifndef OVR_SysFile_h
#define OVR_SysFile_h

#include "File.h"

namespace NervGear {

// ***** Declared classes
class   SysFile;

//-----------------------------------------------------------------------------------
// *** File Statistics

// This class contents are similar to _stat, providing
// creation, modify and other information about the file.
struct FileStat
{
    // No change or create time because they are not available on most systems
    SInt64  modifyTime;
    SInt64  accessTime;
    SInt64  fileSize;

    bool operator== (const FileStat& stat) const
    {
        return ( (modifyTime == stat.modifyTime) &&
                 (accessTime == stat.accessTime) &&
                 (fileSize == stat.fileSize) );
    }
};

//-----------------------------------------------------------------------------------
// *** System File

// System file is created to access objects on file system directly
// This file can refer directly to path.
// System file can be open & closed several times; however, such use is not recommended
// This class is realy a wrapper around an implementation of File interface for a
// particular platform.

class SysFile : public DelegatedFile
{
protected:
  SysFile(const SysFile &source) : DelegatedFile () { OVR_UNUSED(source); }
public:

    // ** Constructor
    SysFile();
    // Opens a file
    SysFile(const VString& path, int flags = Open_Read|Open_Buffered, int mode = ReadWrite);

    // ** Open & management
    bool  open(const VString& path, int flags = Open_Read|Open_Buffered, int mode = ReadWrite);

    OVR_FORCE_INLINE bool  Create(const VString& path, int mode = ReadWrite)
    { return open(path, Open_ReadWrite|Open_Create, mode); }

    // Helper function: obtain file statistics information. In OVR, this is used to detect file changes.
    // Return 0 if function failed, most likely because the file doesn't exist.
    static bool OVR_CDECL getFileStat(FileStat* pfileStats, const VString& path);

    // ** Overrides
    // Overridden to provide re-open support
    virtual int   errorCode();

    virtual bool  isValid();

    virtual bool  close();
};

} // Namespace OVR

#endif
