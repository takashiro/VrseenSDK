
#define  GFILE_CXX

// Standard C library (Captain Obvious guarantees!)
#include <stdio.h>


#include "VSysFile.h"
#include "VUnopenedFile.h"

NV_NAMESPACE_BEGIN

// ***** System Filebool  open(const VString& path, int flags = Open_Read | Open_Buffered, int mode = ReadWrite);

// System file is created to access objects on file system directly
// This file can refer directly to path


VFile* VFileFILEOpen(const VString& path, int flags, int mode);

// ** Constructor
VSysFile::VSysFile() : VDelegatedFile(0)
{
    m_file = *new VUnopenedFile;
}

// Opens a file
VSysFile::VSysFile(const VString& path, int flags, int mode) : VDelegatedFile(0)
{
    open(path, flags, mode);
}


// ** Open & management
// Will fail if file's already open
bool VSysFile::open(const VString& path, int flags, int mode)
{
    m_file = *VFileFILEOpen(path, flags, mode);
    if ((!m_file) || (!m_file->isValid()))
    {
        m_file = *new VUnopenedFile;
        return false;
    }
    //pFile = *OVR_NEW DelegatedFile(pFile); // MA Testing
//    if (flags & Open_Buffered)
//        m_file = *new VBuffer(m_file);
    return true;
}


// ** Overrides

int VSysFile::errorCode()
{
    return m_file ? m_file->errorCode() : FileNotFound;

}


// Overrides to provide re-open support
bool VSysFile::isValid()
{
    return m_file && m_file->isValid();
}

bool VSysFile::fileClose()
{
    if (isValid())
    {
        VDelegatedFile::fileClose();
        m_file = *new VUnopenedFile;
        return 1;
    }
    return 0;
}

NV_NAMESPACE_END
