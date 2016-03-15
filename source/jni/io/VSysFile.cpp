#include "VSysFile.h"

NV_NAMESPACE_BEGIN

//通过路径打开文件系统中的文件，该函数在VFileOperation.cpp中实现
VFile* VOpenFile(const VString& path, int flags);

VSysFile::VSysFile()
    : VDelegatedFile(0)
{
    m_file = nullptr;
}

VSysFile::VSysFile(const VString& path, int flags)
    : VDelegatedFile()
{
    open(path, flags);
}
VSysFile::VSysFile(VFile *pfile)
    : VDelegatedFile(pfile)
{

}

//根据路径打开一个文件
bool VSysFile::open(const VString& path, int flags)
{
    m_file = *VOpenFile(path, flags);
    if ((!m_file) || (!m_file->isOpened())) {
        return false;
    }
    return true;
}

int VSysFile::errorCode()
{
    return m_file ? m_file->errorCode() : FileNotFound;

}

bool VSysFile::isOpened()
{
    return m_file && m_file->isOpened();
}

bool VSysFile::close()
{
    if (isOpened()) {
        VDelegatedFile::close();
        m_file = nullptr;
        return true;
    }
    return false;
}

NV_NAMESPACE_END
