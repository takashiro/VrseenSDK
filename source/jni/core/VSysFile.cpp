
#define  GFILE_CXX

// Standard C library (Captain Obvious guarantees!)
#include <stdio.h>


#include "VSysFile.h"

NV_NAMESPACE_BEGIN

// ***** System Filebool  open(const VString& path, int flags = Open_Read | Open_Buffered, int mode = ReadWrite);

// System file is created to access objects on file system directly
// This file can refer directly to path


VFile* VOpenFile(const VString& path, int flags);

// ** Constructor
VSysFile::VSysFile()
    : VDelegatedFile(0)
{
    m_file = *new VDefaultFile;
}

// Opens a file
VSysFile::VSysFile(const VString& path, int flags)
    : VDelegatedFile()
{
    open(path, flags);
}
VSysFile::VSysFile(VFile *pfile)
    : VDelegatedFile(pfile)
{

}


// ** Open & management
// Will fail if file's already open
bool VSysFile::open(const VString& path, int flags)
{
    m_file = *VOpenFile(path, flags);
    if ((!m_file) || (!m_file->isOpened())) {
        m_file = *new VDefaultFile;
        return false;
    }
    return true;
}


// ** Overrides

int VSysFile::errorCode()
{
    return m_file ? m_file->errorCode() : FileNotFound;

}


// Overrides to provide re-open support
bool VSysFile::isOpened()
{
    return m_file && m_file->isOpened();
}

bool VSysFile::fileClose()
{
    if (isOpened()) {
        VDelegatedFile::fileClose();
        m_file = *new VDefaultFile;
        return true;
    }
    return false;
}

NV_NAMESPACE_END
