#pragma once

#include "vglobal.h"

#include "VDelegatedFile.h"
#include "VFileState.h"
#include "VDefaultFile.h"

NV_NAMESPACE_BEGIN

// ***** Declared classes
class   VSysFile;

//-----------------------------------------------------------------------------------
// *** File Statistics

// This class contents are similar to _stat, providing
// creation, modify and other information about the file.

//-----------------------------------------------------------------------------------
// *** System File

// System file is created to access objects on file system directly
// This file can refer directly to path.
// System file can be open & closed several times; however, such use is not recommended
// This class is realy a wrapper around an implementation of File interface for a
// particular platform.

class VSysFile : public VDelegatedFile
{
protected:
//    VSysFile(const VSysFile &source) : VDelegatedFile () {  }
public:

    // ** Constructor
    VSysFile();
    VSysFile(VFile *pfile);
    // Opens a file
     VSysFile(const VString& path, int flags = Open_Read);

    // ** Open & management
     bool  open(const VString& path, int flags = Open_Read);

    inline bool  Create(const VString& path)
    { return open(path, Open_ReadWrite | Open_Create); }

    // Helper function: obtain file statistics information. In OVR, this is used to detect file changes.
    // Return 0 if function failed, most likely because the file doesn't exist.
    static bool  getFileStat(VFileStat* pfileStats, const VString& path);

    // ** Overrides
    // Overridden to provide re-open support
    virtual int   errorCode() override;

    virtual bool  isOpened() override;

    virtual bool  fileClose() override;

};

NV_NAMESPACE_END
