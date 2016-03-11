
#define  GFILE_CXX

// Standard C library (Captain Obvious guarantees!)
#include <stdio.h>


#include "VSysFile.h"
<<<<<<< HEAD
#include "VUnopenedFile.h"
=======
>>>>>>> dev

NV_NAMESPACE_BEGIN

// ***** System File

// System file is created to access objects on file system directly
// This file can refer directly to path

<<<<<<< HEAD
=======
VFile* VFileFILEOpen(const VString& path, int flags, int mode);

>>>>>>> dev
// ** Constructor
VSysFile::VSysFile() : VDelegatedFile(0)
{
    m_file = *new VUnopenedFile;
}

<<<<<<< HEAD
VFile* VFileFILEOpen(const VString& path, int flags, int mode);
=======
>>>>>>> dev

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
        return 0;
    }
    //pFile = *OVR_NEW DelegatedFile(pFile); // MA Testing
    if (flags & Open_Buffered)
        m_file = *new VBuffer(m_file);
    return 1;
}


// ** Overrides

int VSysFile::errorCode()
{
<<<<<<< HEAD
    return m_file ? m_file->errorCode() : FileNotFoundError;
=======
    return m_file ? m_file->errorCode() : FileNotFound;
>>>>>>> dev
}


// Overrides to provide re-open support
bool VSysFile::isValid()
{
    return m_file && m_file->isValid();
}
<<<<<<< HEAD
bool VSysFile::close()
{
    if (isValid())
    {
        VDelegatedFile::close();
=======
bool VSysFile::Close()
{
    if (isValid())
    {
        VDelegatedFile::Close();
>>>>>>> dev
        m_file = *new VUnopenedFile;
        return 1;
    }
    return 0;
}

NV_NAMESPACE_END
